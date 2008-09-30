/*	$Id: aldap.c,v 1.10 2008/08/28 17:15:14 alex Exp $ */
/*	$OpenBSD: src/usr.sbin/ypldap/aldap.c,v 1.1 2008/09/30 16:24:16 aschrijver Exp $ */

/*
 * Copyright (c) 2008 Alexander Schrijver <aschrijver@openbsd.org>
 * Copyright (c) 2006, 2007 Marc Balmer <mbalmer@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "aldap.h"

//#define DEBUG
#define VERSION 3

static struct ber_element	*ldap_parse_search_filter(struct ber_element*, char *);
static struct ber_element	*ldap_do_parse_search_filter(struct ber_element*, char **);
char				**aldap_get_stringset(struct ber_element *);
char				*utoa(char *u);

#ifdef DEBUG
void			 ldap_debug_elements(struct ber_element *);
#endif

#ifdef DEBUG
#define DPRINTF(x...)	printf(x)
#define LDAP_DEBUG(x, y)	do { fprintf(stderr, "*** " x "\n"); ldap_debug_elements(y); } while (0)
//#define LDAP_DEBUG(x, y)	do { printf("*** " x "\n"); } while (0)
#else
#define DPRINTF(x...)	do { } while (0)
#define LDAP_DEBUG(x, y)	do { } while (0)
#endif

struct aldap *
aldap_init(int fd)
{
	struct aldap *a;

	if((a = malloc(sizeof(*a))) == NULL)
		return NULL;
	bzero(a, sizeof(*a));
	a->ber.fd = fd;

	return a;
}

int
aldap_bind(struct aldap *ldap, char *binddn, char *bindcred)
{
	struct ber_element *root;

	root = ber_add_sequence(NULL);
	ber_printf_elements(root, "d{tdsst", ++ldap->msgid, BER_CLASS_APP,
	    (unsigned long)LDAP_REQ_BIND, VERSION, binddn, bindcred,
	    BER_CLASS_CONTEXT, (unsigned long)LDAP_AUTH_SIMPLE);

	LDAP_DEBUG("aldap_bind", root);

	if(ber_write_elements(&ldap->ber, root) == -1)
		return (-1);

	return (ldap->msgid);
}

int
aldap_unbind(struct aldap *ldap)
{
	struct ber_element *root;

	root = ber_add_sequence(NULL);
	ber_printf_elements(root, "d{t", ++ldap->msgid, BER_CLASS_APP,
	    LDAP_REQ_UNBIND_30);

	LDAP_DEBUG("aldap_unbind", root);

	if(ber_write_elements(&ldap->ber, root) == -1)
		return (-1);

	return (ldap->msgid);
}

int
aldap_search(struct aldap *ldap, char *basedn, enum scope scope, char *filter,
    char **attrs, int typesonly, int sizelimit, int timelimit)
{
	struct ber_element *root, *ber;
	int i;

	root = ber_add_sequence(NULL);
	ber = ber_printf_elements(root, "d{tsEEddb", ++ldap->msgid, BER_CLASS_APP,
	    (unsigned long)LDAP_REQ_SEARCH, basedn, (long long)scope,
	    (long long)LDAP_DEREF_NEVER, sizelimit, timelimit, typesonly);

	ber = ldap_parse_search_filter(ber, filter);
	ber = ber_add_sequence(ber);

	if(attrs != NULL)
		for(i = 0; i >= 0 && attrs[i] != NULL; i++)
			ber = ber_add_string(ber, attrs[i]);

	LDAP_DEBUG("aldap_search", root);

	if(ber_write_elements(&ldap->ber, root) == -1)
		return (-1);

	return (ldap->msgid);
}

struct aldap_message *
aldap_parse(struct aldap *ldap)
{
	int			 class = 0;
	long long		 msgid = 0;
	unsigned long		 type  = 0;
	struct aldap_message	*m;
	struct ber_element	*a = NULL;

	if((m = malloc(sizeof(struct aldap_message))) == NULL)
		return NULL;
	bzero(m, sizeof(struct aldap_message));

	if((m->msg = ber_read_elements(&ldap->ber, NULL)) == NULL)
		return NULL;

	LDAP_DEBUG("message", m->msg);

	if(ber_scanf_elements(m->msg, "{ite", &msgid, &class, &type, &a) != 0)
		goto parsefail;
	m->msgid = msgid;
	m->message_type = type;
	m->protocol_op = a;

	switch(m->message_type) {
	case LDAP_RES_BIND:
	case LDAP_RES_MODIFY:
	case LDAP_RES_ADD:
	case LDAP_RES_DELETE:
	case LDAP_RES_MODRDN:
	case LDAP_RES_COMPARE:
	case LDAP_RES_SEARCH_RESULT:
		if(ber_scanf_elements(m->protocol_op, "{EeSeSe",
			    &m->body.res.rescode, &m->dn, &m->body.res.diagmsg, &a) != 0)
			goto parsefail;
		if(m->body.res.rescode == LDAP_REFERRAL)
			if(ber_scanf_elements(a, "{e", &m->body.references) != 0)
				goto parsefail;
		break;
	case LDAP_RES_SEARCH_ENTRY:
		if(ber_scanf_elements(m->protocol_op, "{eS{e", &m->dn,
			    &m->body.search.entries) != 0)
			goto parsefail;
		break;
	case LDAP_RES_SEARCH_REFERENCE:
		if(ber_scanf_elements(m->protocol_op, "{e", &m->body.references) != 0)
			goto parsefail;
		break;
	}

	return m;
parsefail:
	return NULL;
}

void
aldap_freemsg(struct aldap_message *msg)
{
	ber_free_elements(msg->msg);
	free(msg);
}

int
aldap_get_resultcode(struct aldap_message *msg)
{
	return msg->body.res.rescode;
}

char *
aldap_get_dn(struct aldap_message *msg)
{
	char *dn;

	if(msg->dn == NULL)
		return NULL;

	if(ber_get_string(msg->dn, &dn) == -1)
		return NULL;

	return utoa(dn);
}

char **
aldap_get_references(struct aldap_message *msg)
{
	 return aldap_get_stringset(msg->body.references);
}

void
aldap_free_references(char **values)
{
	int i;

	if(values == NULL)
		return;

	for(i = 0; i >= 0 && values[i] != NULL; i++)
		free(values[i]);

	free(values);
}

char *
aldap_get_diagmsg(struct aldap_message *msg)
{
	char *s;

	if(msg->body.res.diagmsg == NULL)
		return NULL;

	if(ber_get_string(msg->body.res.diagmsg, &s) == -1)
		return NULL;

	return utoa(s);
}

int
aldap_count_entries(struct aldap_message *msg)
{
	int i;
	struct ber_element *a;

	if(msg->body.search.entries == NULL)
		return (-1);

	for(i = 0, a = msg->body.search.entries; i >= 0 && a != NULL && ber_get_eoc(a) != 0;
	    i++, a = a->be_next)
		;

	return i;
}

int
aldap_first_entry(struct aldap_message *msg, char **outkey, char ***outvalues)
{
	struct ber_element *b, *c;
	char *key;

	if(msg->body.search.entries == NULL)
		return (-1);

	if(ber_scanf_elements(msg->body.search.entries, "{s(e)}e", &key, &b,
		    &c) != 0)
		return (-1);

	msg->body.search.iter = msg->body.search.entries->be_next;

	(*outvalues) = aldap_get_stringset(b);
	(*outkey) = utoa(key);

	return (1);
}

int
aldap_next_entry(struct aldap_message *msg, char **outkey, char ***outvalues)
{
	struct ber_element *a, *b;
	char *key;

	LDAP_DEBUG("entry", msg->body.search.iter);

	if(msg->body.search.iter == NULL)
		goto notfound;
	if(ber_get_eoc(msg->body.search.iter) == 0)
		goto notfound;

	if(ber_scanf_elements(msg->body.search.iter, "{s(e)}e", &key, &a, &b)
	    != 0)
		goto fail;

	msg->body.search.iter = msg->body.search.iter->be_next;

	(*outvalues) = aldap_get_stringset(a);
	(*outkey) = utoa(key);

	return (1);
fail:
notfound:
	(*outkey) = NULL;
	(*outvalues) = NULL;
	return (-1);
}

int
aldap_match_entry(struct aldap_message *msg, char *inkey, char ***outvalues)
{
	struct ber_element *a, *b;
	char *descr = NULL;

	LDAP_DEBUG("entry", msg->search_entries);

	if(msg->body.search.entries == NULL)
		return (-1);

	for(a = msg->body.search.entries;;) {
		if(a == NULL)
			goto notfound;
		if(ber_get_eoc(a) == 0)
			goto notfound;
		if(ber_scanf_elements(a, "{s(e", &descr, &b) != 0)
			goto fail;
		if(strcasecmp(descr, inkey) == 0)
			goto attrfound;
		a = a->be_next;
	}

attrfound:
	(*outvalues) = aldap_get_stringset(b);

	return (1);
fail:
notfound:
	(*outvalues) = NULL;
	return (-1);
}

int
aldap_free_entry(char **values)
{
	int i;

	if(values == NULL)
		return -1;

	for(i = 0; i >= 0 && values[i] != NULL; i++)
		free(values[i]);

	free(values);

	return (1);
}

void
aldap_free_url(struct aldap_url *lu)
{
	int i;
	free(lu->host);
	free(lu->dn);
	for(i = 0; i < MAXATTR && lu->attributes[i] != NULL; i++) {
		free(lu->attributes[i]);
	}
	free(lu->filter);
}

int
aldap_parse_url(char *url, struct aldap_url *lu)
{
	char	*dup, *p, *forward, *forward2;
	int	 i;

	p = dup = strdup(url);

	/* protocol */
	if(strncasecmp(LDAP_URL, p, strlen(LDAP_URL)) != 0)
		goto fail;
	lu->protocol = LDAP;
	p += strlen(LDAP_URL);

	/* host and optional port */
	if((forward = strchr(p, '/')) != NULL)
		*forward = '\0';
	/* find the optional port */
	if((forward2 = strchr(p, ':')) != NULL) {
		*forward2 = '\0';
		/* if a port is given */
		if(*(forward2+1) != '\0')
			lu->port = atoi(++forward2);
	}
	/* fail if no host is given */
	if(strlen(p) == 0)
		goto fail;
	lu->host = strdup(p);
	if(forward == NULL)
		goto done;
	/* p is assigned either a pointer to a character or to '\0' */
	p = ++forward;
	if(strlen(p) == 0)
		goto done;

	/* dn */
	if((forward = strchr(p, '?')) != NULL)
		*forward = '\0';
	lu->dn = strdup(p);
	if(forward == NULL)
		goto done;
	/* p is assigned either a pointer to a character or to '\0' */
	p = ++forward;
	if(strlen(p) == 0)
		goto done;

	/* attributes */
	if((forward = strchr(p, '?')) != NULL)
		*forward = '\0';
	for(i = 0; i < MAXATTR; i++) {
		if((forward2 = strchr(p, ',')) == NULL) {
			if(strlen(p) == 0)
				break;
			lu->attributes[i] = strdup(p);
			break;
		}
		*forward2 = '\0';
		lu->attributes[i] = strdup(p);
		p = ++forward2;
	}
	if(forward == NULL)
		goto done;
	/* p is assigned either a pointer to a character or to '\0' */
	p = ++forward;
	if(strlen(p) == 0)
		goto done;

	/* scope */
	if((forward = strchr(p, '?')) != NULL)
		*forward = '\0';
	if(strcmp(p, "base") == 0)
		lu->scope = LDAP_SCOPE_BASE;
	else if(strcmp(p, "one") == 0)
		lu->scope = LDAP_SCOPE_ONELEVEL;
	else if(strcmp(p, "sub") == 0)
		lu->scope = LDAP_SCOPE_SUBTREE;
	if(forward == NULL)
		goto done;
	p = ++forward;
	if(strlen(p) == 0)
		goto done;

	/* filter */
	if(p == NULL)
		goto done;
	lu->filter = strdup(p);

done:
	free(dup);
	return (1);
fail:
	free(dup);
	return (-1);
}

/*
 * internal functions
 */

char **
aldap_get_stringset(struct ber_element *elm)
{
	struct ber_element *a;
	int i;
	char **ret;
	char *s;

	if(elm->be_type != BER_TYPE_OCTETSTRING)
		return NULL;

	for(a = elm, i = 1; i > 0 && a->be_type == BER_TYPE_OCTETSTRING;
	    a = a->be_next, i++)
		;

	if((ret = calloc(i + 1, sizeof(char *))) == NULL)
		return NULL;

	for(a = elm, i = 0; i >= 0 && a->be_type == BER_TYPE_OCTETSTRING;
	    a = a->be_next, i++) {

		ber_get_string(a, &s);
		ret[i] = utoa(s);
	}
	ret[i + 1] = NULL;

	return ret;
}

/*
 * Base case for __ldap_do_parse_search_filter
 *
 * returns:
 *	struct ber_element *, ber_element tree
 *	NULL, parse failed
 */
static struct ber_element *
ldap_parse_search_filter(struct ber_element *ber, char *filter)
{
	struct ber_element *elm;
	char *cp;

	cp = filter;

	if (cp == NULL || *cp == '\0') {
		errno = EINVAL;
		return (NULL);
	}

	if ((elm = ldap_do_parse_search_filter(ber, &cp)) == NULL)
		return (NULL);

	if (*cp != '\0') {
		ber_free_elements(elm);
		errno = EINVAL;
		return (NULL);
	}

	return (elm);
}

/*
 * Translate RFC2254 search filter string into ber_element tree
 *
 * returns:
 *	struct ber_element *, ber_element tree
 *	NULL, parse failed
 *
 * notes:
 *	when cp is passed to a recursive invocation, it is updated
 *	    to point one character beyond the filter that was passed
 *	    i.e., cp jumps to "(filter)"
 *	                               ^
 *	goto's used to discriminate error-handling based on error type
 *	doesn't handle extended filters (yet)
 *
 *	escaped characters aren't supported (yet).
 */
static struct ber_element *
ldap_do_parse_search_filter(struct ber_element *prev, char **cpp)
{
	struct ber_element *elm, *root;
	char *attr_desc, *attr_val, *cp;
	size_t len;
	unsigned long type;

	root = NULL;

	/* cpp should pass in pointer to opening parenthesis of "(filter)" */
	cp = *cpp;
	if (*cp != '(')
		goto syntaxfail;

	switch (*++cp) {
	case '&':		/* AND */
	case '|':		/* OR */
		if (*cp == '&')
			type = LDAP_FILT_AND;
		else
			type = LDAP_FILT_OR;

		if ((elm = ber_add_set(prev)) == NULL)
			goto callfail;
		root = elm;
		ber_set_header(elm, BER_CLASS_APP, type);

		if (*++cp != '(')		/* opening `(` of filter */
			goto syntaxfail;

		while(*cp == '(') {
			if ((elm =
			    ldap_do_parse_search_filter(elm, &cp)) == NULL)
				goto bad;
		}

		if (*cp != ')')			/* trailing `)` of filter */
			goto syntaxfail;
		break;

	case '!':		/* NOT */
		if ((root = ber_add_sequence(prev)) == NULL)
			goto callfail;
		ber_set_header(root, BER_CLASS_APP, LDAP_FILT_NOT);

		cp++;				/* now points to sub-filter */
		if ((elm = ldap_do_parse_search_filter(root, &cp)) == NULL)
			goto bad;

		if (*cp != ')')			/* trailing `)` of filter */
			goto syntaxfail;
		break;

	default:	/* SIMPLE || PRESENCE */
		attr_desc = cp;

		len = strcspn(cp, "()<>~=");
		cp += len;
		switch(*cp) {
		case '~':
			type = LDAP_FILT_APPR;
			cp++;
			break;
		case '<':
			type = LDAP_FILT_LE;
			cp++;
			break;
		case '>':
			type = LDAP_FILT_GE;
			cp++;
			break;
		case '=':
			type = LDAP_FILT_EQ;	/* assume EQ until disproven */
			break;
		case '(':
		case ')':
		default:
			goto syntaxfail;
		}
		attr_val = ++cp;

		/* presence filter */
		if (strncmp(attr_val, "*)", 2) == 0) {
			cp++;			/* point to trailing `)` */
			if ((root =
			    ber_add_nstring(prev, attr_desc, len)) == NULL)
				goto bad;

			ber_set_header(root, BER_CLASS_CONTEXT, LDAP_FILT_PRES);
			break;
		}

		if ((root = ber_add_sequence(prev)) == NULL)
			goto callfail;
		ber_set_header(root, BER_CLASS_CONTEXT, type);

		if ((elm = ber_add_nstring(root, attr_desc, len)) == NULL)
			goto callfail;

		len = strcspn(attr_val, "*)");
		if (len == 0 && *cp != '*')
			goto syntaxfail;
		cp += len;
		if (*cp == '\0')
			goto syntaxfail;

		if (*cp == '*') {	/* substring filter */
			int initial;

			cp = attr_val;

			ber_set_header(root, BER_CLASS_CONTEXT, LDAP_FILT_SUBS);

			if ((elm = ber_add_sequence(elm)) == NULL)
				goto callfail;

			for (initial = 1;; cp++, initial = 0) {
				attr_val = cp;

				len = strcspn(attr_val, "*)");
				if (len == 0) {
					if (*cp == ')')
						break;
					else
						continue;
				}
				cp += len;
				if (*cp == '\0')
					goto syntaxfail;

				if (initial)
					type = LDAP_FILT_SUBS_INIT;
				else if (*cp == ')')
					type = LDAP_FILT_SUBS_FIN;
				else
					type = LDAP_FILT_SUBS_ANY;

				if ((elm =
				    ber_add_nstring(elm, attr_val, len)) == NULL)
					goto callfail;
				ber_set_header(elm, BER_CLASS_CONTEXT, type);
				if (type == LDAP_FILT_SUBS_FIN)
					break;
			}
			break;
		}

		if ((elm = ber_add_nstring(elm, attr_val, len)) == NULL)
			goto callfail;
		break;
	}

	cp++;		/* now points one char beyond the trailing `)` */

	*cpp = cp;
	return (root);

syntaxfail:		/* XXX -- error reporting */
callfail:
bad:
	ber_free_elements(root);
	return (NULL);
}

#ifdef DEBUG
/*
 * Display a list of ber elements.
 *
 */
void
ldap_debug_elements(struct ber_element *root)
{
	static int	 indent = 0;
	long long	 v;
	int		 d;
	char		*buf;
	size_t		 len;
	u_int		 i;
	int		 constructed;
	struct ber_oid	 o;

	/* calculate lengths */
	ber_calc_len(root);

	switch (root->be_encoding) {
	case BER_TYPE_SEQUENCE:
	case BER_TYPE_SET:
		constructed = root->be_encoding;
		break;
	default:
		constructed = 0;
		break;
	}

	fprintf(stderr, "%*slen %lu ", indent, "", root->be_len);
	switch (root->be_class) {
	case BER_CLASS_UNIVERSAL:
		fprintf(stderr, "class: universal(%u) type: ", root->be_class);
		switch (root->be_type) {
		case BER_TYPE_EOC:
			fprintf(stderr, "end-of-content");
			break;
		case BER_TYPE_BOOLEAN:
			fprintf(stderr, "boolean");
			break;
		case BER_TYPE_INTEGER:
			fprintf(stderr, "integer");
			break;
		case BER_TYPE_BITSTRING:
			fprintf(stderr, "bit-string");
			break;
		case BER_TYPE_OCTETSTRING:
			fprintf(stderr, "octet-string");
			break;
		case BER_TYPE_NULL:
			fprintf(stderr, "null");
			break;
		case BER_TYPE_OBJECT:
			fprintf(stderr, "object");
			break;
		case BER_TYPE_ENUMERATED:
			fprintf(stderr, "enumerated");
			break;
		case BER_TYPE_SEQUENCE:
			fprintf(stderr, "sequence");
			break;
		case BER_TYPE_SET:
			fprintf(stderr, "set");
			break;
		}
		break;
	case BER_CLASS_APPLICATION:
		fprintf(stderr, "class: application(%u) type: ",
		    root->be_class);
		switch (root->be_type) {
		case LDAP_REQ_BIND:
			fprintf(stderr, "bind");
			break;
		case LDAP_RES_BIND:
			fprintf(stderr, "bind");
			break;
		case LDAP_REQ_UNBIND_30:
			break;
		case LDAP_REQ_SEARCH:
			fprintf(stderr, "search");
			break;
		case LDAP_RES_SEARCH_ENTRY:
			fprintf(stderr, "search_entry");
			break;
		case LDAP_RES_SEARCH_RESULT:
			fprintf(stderr, "search_result");
			break;
		case LDAP_REQ_MODIFY:
			fprintf(stderr, "modify");
			break;
		case LDAP_RES_MODIFY:
			fprintf(stderr, "modify");
			break;
		case LDAP_REQ_ADD:
			fprintf(stderr, "add");
			break;
		case LDAP_RES_ADD:
			fprintf(stderr, "add");
			break;
		case LDAP_REQ_DELETE_30:
			fprintf(stderr, "delete");
			break;
		case LDAP_RES_DELETE:
			fprintf(stderr, "delete");
			break;
		case LDAP_REQ_MODRDN:
			fprintf(stderr, "modrdn");
			break;
		case LDAP_RES_MODRDN:
			fprintf(stderr, "modrdn");
			break;
		case LDAP_REQ_COMPARE:
			fprintf(stderr, "compare");
			break;
		case LDAP_RES_COMPARE:
			fprintf(stderr, "compare");
			break;
		case LDAP_REQ_ABANDON_30:
			fprintf(stderr, "abandon");
			break;
		}
		break;
	case BER_CLASS_PRIVATE:
		fprintf(stderr, "class: private(%u) type: ", root->be_class);
		fprintf(stderr, "encoding (%lu) type: ", root->be_encoding);
		break;
	case BER_CLASS_CONTEXT:
		/* XXX: this is not correct */
		fprintf(stderr, "class: context(%u) type: ", root->be_class);
		switch(root->be_type) {
		case LDAP_AUTH_SIMPLE:
			fprintf(stderr, "auth simple");
			break;
		}
		break;
	default:
		fprintf(stderr, "class: <INVALID>(%u) type: ", root->be_class);
		break;
	}
	fprintf(stderr, "(%lu) encoding %lu ",
	    root->be_type, root->be_encoding);

	if (constructed)
		root->be_encoding = constructed;

	switch (root->be_encoding) {
	case BER_TYPE_BOOLEAN:
		if (ber_get_boolean(root, &d) == -1) {
			fprintf(stderr, "<INVALID>\n");
			break;
		}
		fprintf(stderr, "%s(%d)\n", d ? "true" : "false", d);
		break;
	case BER_TYPE_INTEGER:
		if (ber_get_integer(root, &v) == -1) {
			fprintf(stderr, "<INVALID>\n");
			break;
		}
		fprintf(stderr, "value %lld\n", v);
		break;
	case BER_TYPE_ENUMERATED:
		if (ber_get_enumerated(root, &v) == -1) {
			fprintf(stderr, "<INVALID>\n");
			break;
		}
		fprintf(stderr, "value %lld\n", v);
		break;
	case BER_TYPE_BITSTRING:
		if (ber_get_bitstring(root, (void *)&buf, &len) == -1) {
			fprintf(stderr, "<INVALID>\n");
			break;
		}
		fprintf(stderr, "hexdump ");
		for (i = 0; i < len; i++)
			fprintf(stderr, "%02x", buf[i]);
		fprintf(stderr, "\n");
		break;
	case BER_TYPE_OBJECT:
		if (ber_get_oid(root, &o) == -1) {
			fprintf(stderr, "<INVALID>\n");
			break;
		}
		fprintf(stderr, "\n");
		break;
	case BER_TYPE_OCTETSTRING:
		if (ber_get_nstring(root, (void *)&buf, &len) == -1) {
			fprintf(stderr, "<INVALID>\n");
			break;
		}
		fprintf(stderr, "string \"%.*s\"\n",  len, buf);
		break;
	case BER_TYPE_NULL:	/* no payload */
	case BER_TYPE_EOC:
	case BER_TYPE_SEQUENCE:
	case BER_TYPE_SET:
	default:
		fprintf(stderr, "\n");
		break;
	}

	if (constructed && root->be_sub) {
		indent += 2;
		ldap_debug_elements(root->be_sub);
		indent -= 2;
	}
	if (root->be_next)
		ldap_debug_elements(root->be_next);
}
#endif

/*
 * Convert UTF-8 to ASCII.
 * notes:
 *	non-ASCII characters are displayed as '?'
 *	the argument u should be a NULL terminated sequence of UTF-8 bytes.
 */
char *
utoa(char *u)
{
	int	 len, i, j;
	char	*str;

	/* calculate the length to allocate */
	for(len = 0, i = 0; u[i] != NULL; ) {
		if((u[i] & 0xF0) == 0xF0)
			i += 4;
		else if((u[i] & 0xE0) == 0xE0)
			i += 3;
		else if((u[i] & 0xC0) == 0xC0)
			i += 2;
		else
			i += 1;
		len++;
	}

	str = calloc(len + 1, sizeof(char));
	bzero(str, (len + 1) * sizeof(char));

	/* copy the ASCII characters to the newly allocated string */
	for(i = 0, j = 0; u[i] != NULL; j++) {
		if((u[i] & 0xF0) == 0xF0) {
			str[j] = '?';
			i += 4;
		} else if((u[i] & 0xE0) == 0xE0) {
			str[j] = '?';
			i += 3;
		} else if((u[i] & 0xC0) == 0xC0) {
			str[j] = '?';
			i += 2;
		} else {
			str[j] =  u[i];
			i += 1;
		}
	}

	return str;
}

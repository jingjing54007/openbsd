%{
/*	$OpenBSD: src/usr.bin/bc/bc.y,v 1.12 2003/10/18 20:35:36 otto Exp $	*/

/*
 * Copyright (c) 2003, Otto Moerbeek <otto@drijf.net>
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

/*
 * This implementation of bc(1) uses concepts from the original 4.4
 * BSD bc(1). The code itself is a complete rewrite, based on the
 * Posix defined bc(1) grammar. Other differences include type safe
 * usage of pointers to build the tree of emitted code, typed yacc
 * rule values, dynamic allocation of all data structures and a
 * completely rewritten lexical analyzer using lex(1).
 *
 * Some effort has been made to make sure that the generated code is
 * the same as the code generated by the older version, to provide
 * easy regression testing.
 */

#ifndef lint
static const char rcsid[] = "$OpenBSD: src/usr.bin/bc/bc.y,v 1.12 2003/10/18 20:35:36 otto Exp $";
#endif /* not lint */

#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include "extern.h"
#include "pathnames.h"

#define END_NODE	((ssize_t) -1)
#define CONST_STRING	((ssize_t) -2)
#define ALLOC_STRING	((ssize_t) -3)

struct tree {
	ssize_t			index;
	union {
		char		*astr;
		const char	*cstr;
	} u;
};

int			yyparse(void);
int			yywrap(void);

static void		grow(void);
static ssize_t		cs(const char *);
static ssize_t		as(const char *);
static ssize_t		node(ssize_t, ...);
static void		emit(ssize_t);
static void		emit_macro(int, ssize_t);
static void		free_tree(void);
static ssize_t		numnode(int);
static void		add_par(ssize_t);
static void		add_local(ssize_t);
static void		warning(const char *);
static void		init(void);
static __dead void	usage(void);

static size_t		instr_sz = 0;
static struct tree	*instructions = NULL;
static ssize_t		current = 0;
static int		macro_char = '0';
static int		reset_macro_char = '0';
static int		nesting = 0;
static int		breakstack[16];
static int		breaksp = 0;
static ssize_t		prologue;
static ssize_t		epilogue;
static bool		st_has_continue;
static char		str_table[UCHAR_MAX][2];
static int		sargc;
static char		**sargv;
static char		*filename;
static bool		do_fork = true;

extern char *__progname;

#define BREAKSTACK_SZ	(sizeof(breakstack)/sizeof(breakstack[0]))

/* These values are 4.4BSD dc compatible */
#define FUNC_CHAR	0x01
#define ARRAY_CHAR	0xa1

#define LETTER_NODE(str)	(cs(str_table[(int)str[0]]))
#define ARRAY_NODE(str)		(cs(str_table[(int)str[0] - 'a' + ARRAY_CHAR]))
#define FUNCTION_NODE(str)	(cs(str_table[(int)str[0] - 'a' + FUNC_CHAR]))

%}

%start program

%union {
	ssize_t		node;
	struct lvalue	lvalue;
	const char	*str;
}

%token COMMA SEMICOLON LPAR RPAR LBRACE RBRACE LBRACKET RBRACKET DOT
%token NEWLINE
%token <str> LETTER NUMBER STRING
%token DEFINE BREAK QUIT LENGTH
%token RETURN FOR IF WHILE SQRT
%token SCALE IBASE OBASE AUTO
%token CONTINUE ELSE

%nonassoc EQUALS LESS_EQ GREATER_EQ UNEQUALS LESS GREATER
%right <str> ASSIGN_OP
%left PLUS MINUS
%left MULTIPLY DIVIDE REMAINDER
%left EXPONENT
%nonassoc UMINUS
%nonassoc INCR DECR

%type <lvalue>	named_expression
%type <node>	argument_list
%type <node>	alloc_macro
%type <node>	expression
%type <node>	function
%type <node>	function_header
%type <node>	input_item
%type <node>	opt_argument_list
%type <node>	opt_statement
%type <node>	relational_expression
%type <node>	return_expression
%type <node>	semicolon_list
%type <node>	statement
%type <node>	statement_list

%%

program		: /* empty */
			{
				putchar('q');
				fflush(stdout);
				exit(0);
			}
		| input_item program
		;

input_item	: semicolon_list NEWLINE
			{
				emit($1);
				macro_char = reset_macro_char;
				putchar('\n');
				free_tree();
				st_has_continue = false;
			}
		| function
			{
				putchar('\n');
				free_tree();
				st_has_continue = false;
			}
		| error NEWLINE
			{
				yyerrok;
			}
		;

semicolon_list	: /* empty */
			{
				$$ = cs("");
			}
		| statement
		| semicolon_list SEMICOLON statement
			{
				$$ = node($1, $3, END_NODE);
			}
		| semicolon_list SEMICOLON
		;

statement_list	: /* empty */
			{
				$$ = cs("");
			}
		| statement
		| statement_list NEWLINE
		| statement_list NEWLINE statement
			{
				$$ = node($1, $3, END_NODE);
			}
		| statement_list SEMICOLON
		| statement_list SEMICOLON statement
			{
				$$ = node($1, $3, END_NODE);
			}
		;


opt_statement	: /* empty */
			{
				$$ = cs("");
			}
		| statement
		;

statement	: expression
			{
				$$ = node($1, cs("ps."), END_NODE);
			}
		| named_expression ASSIGN_OP expression
			{
				if ($2[0] == '\0')
					$$ = node($3, cs($2), $1.store,
					    END_NODE);
				else
					$$ = node($1.load, $3, cs($2), $1.store,
					    END_NODE);
			}
		| STRING
			{
				$$ = node(cs("["), as($1),
				    cs("]P"), END_NODE);
			}
		| BREAK
			{
				if (breaksp == 0) {
					warning("break not in for or while");
					YYERROR;
				} else {
					$$ = node(
					    numnode(nesting -
						breakstack[breaksp-1]),
					    cs("Q"), END_NODE);
				}
			}
		| CONTINUE
			{
				if (breaksp == 0) {
					warning("continue not in for or while");
					YYERROR;
				} else {
					st_has_continue = true;
					$$ = node(numnode(nesting -
					    breakstack[breaksp-1] - 1),
					    cs("J"), END_NODE);
				}
			}
		| QUIT
			{
				putchar('q');
				fflush(stdout);
				exit(0);
			}
		| RETURN
			{
				if (nesting == 0) {
					warning("return must be in a function");
					YYERROR;
				}
				$$ = node(cs("0"), epilogue,
				    numnode(nesting), cs("Q"), END_NODE);
			}
		| RETURN LPAR return_expression RPAR
			{
				if (nesting == 0) {
					warning("return must be in a function");
					YYERROR;
				}
				$$ = $3;
			}
		| FOR LPAR alloc_macro expression SEMICOLON
		     relational_expression SEMICOLON
		     expression RPAR opt_statement pop_nesting
			{
				ssize_t n;

				if (st_has_continue)
					n = node($10, cs("M"), $8, cs("s."),
					    $6, $3, END_NODE);
				else
					n = node($10, $8, cs("s."), $6, $3,
					    END_NODE);

				emit_macro($3, n);
				$$ = node($4, cs("s."), $6, $3, cs(" "),
				    END_NODE);
			}
		| IF LPAR alloc_macro pop_nesting relational_expression RPAR
		      opt_statement
			{
				emit_macro($3, $7);
				$$ = node($5, $3, cs(" "), END_NODE);
			}
		| WHILE LPAR alloc_macro relational_expression RPAR
		      opt_statement pop_nesting
			{
				ssize_t n;

				if (st_has_continue)
					n = node($6, cs("M"), $4, $3, END_NODE);
				else
					n = node($6, $4, $3, END_NODE);
				emit_macro($3, n);
				$$ = node($4, $3, cs(" "), END_NODE);
			}
		| LBRACE statement_list RBRACE
			{
				$$ = $2;
			}
		;

alloc_macro	: /* empty */
			{
				$$ = cs(str_table[macro_char]);
				macro_char++;
				/* Do not use [, \ and ] */
				if (macro_char == '[')
					macro_char += 3;
				/* skip letters */
				else if (macro_char == 'a')
					macro_char = '{';
				else if (macro_char == ARRAY_CHAR)
					macro_char += 26;
				else if (macro_char == 256)
					fatal("program too big");
				if (breaksp == BREAKSTACK_SZ)
					fatal("nesting too deep");
				breakstack[breaksp++] = nesting++;
			}
		;

pop_nesting	: /* empty */
			{
				breaksp--;
			}
		;

function	: function_header opt_parameter_list RPAR
		  LBRACE NEWLINE opt_auto_define_list
		  statement_list RBRACE
			{
				int n = node(prologue, $7, epilogue,
				    cs("0"), numnode(nesting),
				    cs("Q"), END_NODE);
				emit_macro($1, n);
				reset_macro_char = macro_char;
				nesting = 0;
				breaksp = 0;
			}
		;

function_header : DEFINE LETTER LPAR
			{
				$$ = FUNCTION_NODE($2);
				prologue = cs("");
				epilogue = cs("");
				nesting = 1;
				breaksp = 0;
				breakstack[breaksp] = 0;
			}
		;

opt_parameter_list
		: /* empty */
		| parameter_list
		;


parameter_list	: LETTER
			{
				add_par(LETTER_NODE($1));
			}
		| LETTER LBRACKET RBRACKET
			{
				add_par(ARRAY_NODE($1));
			}
		| parameter_list COMMA LETTER
			{
				add_par(LETTER_NODE($3));
			}
		| parameter_list COMMA LETTER LBRACKET RBRACKET
			{
				add_par(ARRAY_NODE($3));
			}
		;



opt_auto_define_list
		: /* empty */
		| AUTO define_list NEWLINE
		| AUTO define_list SEMICOLON
		;


define_list	: LETTER
			{
				add_local(LETTER_NODE($1));
			}
		| LETTER LBRACKET RBRACKET
			{
				add_local(ARRAY_NODE($1));
			}
		| define_list COMMA LETTER
			{
				add_local(LETTER_NODE($3));
			}
		| define_list COMMA LETTER LBRACKET RBRACKET
			{
				add_local(ARRAY_NODE($3));
			}
		;


opt_argument_list
		: /* empty */
			{
				$$ = cs("");
			}
		| argument_list
		;


argument_list	: expression
		| argument_list COMMA expression
			{
				$$ = node($1, $3, END_NODE);
			}
		| argument_list COMMA LETTER LBRACKET RBRACKET
			{
				$$ = node($1, cs("l"), ARRAY_NODE($3),
				    END_NODE);
			}
		;


relational_expression
		: expression
			{
				$$ = node($1, cs(" 0!="), END_NODE);
			}
		| expression EQUALS expression
			{
				$$ = node($1, $3, cs("="), END_NODE);
			}
		| expression UNEQUALS expression
			{
				$$ = node($1, $3, cs("!="), END_NODE);
			}
		| expression LESS expression
			{
				$$ = node($1, $3, cs(">"), END_NODE);
			}
		| expression LESS_EQ expression
			{
				$$ = node($1, $3, cs("!<"), END_NODE);
			}
		| expression GREATER expression
			{
				$$ = node($1, $3, cs("<"), END_NODE);
			}
		| expression GREATER_EQ expression
			{
				$$ = node($1, $3, cs("!>"), END_NODE);
			}
		;


return_expression
		: /* empty */
			{
				$$ = node(cs("0"), epilogue,
				    numnode(nesting), cs("Q"), END_NODE);
			}
		| expression
			{
				$$ = node($1, epilogue,
				    numnode(nesting), cs("Q"), END_NODE);
			}
		;


expression	: named_expression
			{
				$$ = node($1.load, END_NODE);
			}
		| DOT	{
				$$ = node(cs("l."), END_NODE);
			}
		| NUMBER
			{
				$$ = node(cs(" "), as($1), END_NODE);
			}
		| LPAR expression RPAR
			{
				$$ = $2;
			}
		| LETTER LPAR opt_argument_list RPAR
			{
				$$ = node($3, cs("l"),
				    FUNCTION_NODE($1), cs("x"),
				    END_NODE);
			}
		| MINUS expression %prec UMINUS
			{
				$$ = node(cs(" 0"), $2, cs("-"),
				    END_NODE);
			}
		| expression PLUS expression
			{
				$$ = node($1, $3, cs("+"), END_NODE);
			}
		| expression MINUS expression
			{
				$$ = node($1, $3, cs("-"), END_NODE);
			}
		| expression MULTIPLY expression
			{
				$$ = node($1, $3, cs("*"), END_NODE);
			}
		| expression DIVIDE expression
			{
				$$ = node($1, $3, cs("/"), END_NODE);
			}
		| expression REMAINDER expression
			{
				$$ = node($1, $3, cs("%"), END_NODE);
			}
		| expression EXPONENT expression
			{
				$$ = node($1, $3, cs("^"), END_NODE);
			}
		| INCR named_expression
			{
				$$ = node($2.load, cs("1+d"), $2.store,
				    END_NODE);
			}
		| DECR named_expression
			{
				$$ = node($2.load, cs("1-d"),
				    $2.store, END_NODE);
			}
		| named_expression INCR
			{
				$$ = node($1.load, cs("d1+"),
				    $1.store, END_NODE);
			}
		| named_expression DECR
			{
				$$ = node($1.load, cs("d1-"),
				    $1.store, END_NODE);
			}
		| named_expression ASSIGN_OP expression
			{
				if ($2[0] == '\0')
					$$ = node($3, cs($2), cs("d"), $1.store,
					    END_NODE);
				else
					$$ = node($1.load, $3, cs($2), cs("d"),
					    $1.store, END_NODE);
			}
		| LENGTH LPAR expression RPAR
			{
				$$ = node($3, cs("Z"), END_NODE);
			}
		| SQRT LPAR expression RPAR
			{
				$$ = node($3, cs("v"), END_NODE);
			}
		| SCALE LPAR expression RPAR
			{
				$$ = node($3, cs("X"), END_NODE);
			}
		;

named_expression
		: LETTER
			{
				$$.load = node(cs("l"), LETTER_NODE($1),
				    END_NODE);
				$$.store = node(cs("s"), LETTER_NODE($1),
				    END_NODE);
			}
		| LETTER LBRACKET expression RBRACKET
			{
				$$.load = node($3, cs(";"),
				    ARRAY_NODE($1), END_NODE);
				$$.store = node($3, cs(":"),
				    ARRAY_NODE($1), END_NODE);
			}
		| SCALE
			{
				$$.load = cs("K");
				$$.store = cs("k");
			}
		| IBASE
			{
				$$.load = cs("I");
				$$.store = cs("i");
			}
		| OBASE
			{
				$$.load = cs("O");
				$$.store = cs("o");
			}
		;
%%


static void
grow(void)
{
	struct tree	*p;
	int		newsize;

	if (current == instr_sz) {
		newsize = instr_sz * 2 + 1;
		p = realloc(instructions, newsize * sizeof(*p));
		if (p == NULL) {
			free(instructions);
			err(1, NULL);
		}
		instructions = p;
		instr_sz = newsize;
	}
}

static ssize_t
cs(const char *str)
{
	grow();
	instructions[current].index = CONST_STRING;
	instructions[current].u.cstr = str;
	return current++;
}

static ssize_t
as(const char *str)
{
	grow();
	instructions[current].index = ALLOC_STRING;
	instructions[current].u.astr = strdup(str);
	if (instructions[current].u.astr == NULL)
		err(1, NULL);
	return current++;
}

static ssize_t
node(ssize_t arg, ...)
{
	va_list		ap;
	ssize_t		ret;

	va_start(ap, arg);

	ret = current;
	grow();
	instructions[current++].index = arg;

	do {
		arg = va_arg(ap, ssize_t);
		grow();
		instructions[current++].index = arg;
	} while (arg != END_NODE);

	va_end(ap);
	return ret;
}

static void
emit(ssize_t i)
{
	if (instructions[i].index >= 0)
		while (instructions[i].index != END_NODE)
			emit(instructions[i++].index);
	else
		fputs(instructions[i].u.cstr, stdout);
}

static void
emit_macro(int node, ssize_t code)
{
	putchar('[');
	emit(code);
	printf("]s%s\n", instructions[node].u.cstr);
	nesting--;
}

static void
free_tree(void)
{
	size_t i;

	for (i = 0; i < current; i++)
		if (instructions[i].index == ALLOC_STRING)
			free(instructions[i].u.astr);
	current = 0;
}

static ssize_t
numnode(int num)
{
	const char *p;

	if (num < 10)
		p = str_table['0' + num];
	else if (num < 16)
		p = str_table['A' - 10 + num];
	else
		err(1, "internal error: break num > 15");
	return node(cs(" "), cs(p), END_NODE);
}

static void
add_par(ssize_t n)
{
	prologue = node(cs("S"), n, prologue, END_NODE);
	epilogue = node(epilogue, cs("L"), n, cs("s."), END_NODE);
}

static void
add_local(ssize_t n)
{
	prologue = node(cs("0S"), n, prologue, END_NODE);
	epilogue = node(epilogue, cs("L"), n, cs("s."), END_NODE);
}

int
yywrap(void)
{
	if (optind < sargc) {
		filename = sargv[optind++];
		yyin = fopen(filename, "r");
		lineno = 1;
		if (yyin == NULL)
			err(1, "cannot open %s", filename);
		return 0;
	} else if (optind == sargc) {
		optind++;
		yyin = stdin;
		lineno = 1;
		filename = "stdin";
		return 0;
	}
	return 1;
}

void
yyerror(char *s)
{
	char	*str, *p;

	if (isspace(yytext[0]) || !isprint(yytext[0]))
		asprintf(&str, "%s: %s:%d: %s: ascii char 0x%x unexpected",
		    __progname, filename, lineno, s, yytext[0]);
	else
		asprintf(&str, "%s: %s:%d: %s: %s unexpected",
		    __progname, filename, lineno, s, yytext);
	if (str == NULL)
		err(1, NULL);

	fputs("c[", stdout);
	for (p = str; *p != '\0'; p++) {
		if (*p == '[' || *p == ']' || *p =='\\')
			putchar('\\');
		putchar(*p);
	}
	fputs("]pc\n", stdout);
	free(str);
}

void
fatal(const char *s)
{
	errx(1, "%s:%d: %s", filename, lineno, s);
}

static void
warning(const char *s)
{
	warnx("%s:%d: %s", filename, lineno, s);
}

static void
init(void)
{
	int i;

	for (i = 0; i < UCHAR_MAX; i++) {
		str_table[i][0] = i;
		str_table[i][1] = '\0';
	}
}


static __dead void
usage(void)
{
	fprintf(stderr, "%s: usage: [-cl] [file ...]\n", __progname);
	exit(1);
}


int
main(int argc, char *argv[])
{
	int	ch, ret;
	int	p[2];

	init();
	setlinebuf(stdout);

	/* The d debug option is 4.4 BSD dc(1) compatible */
	while ((ch = getopt(argc, argv, "cdl")) != -1) {
		switch (ch) {
		case 'c':
		case 'd':
			do_fork = false;
			break;
		case 'l':
			argv[1] = _PATH_LIBB;
			optind = 1;
			break;
		default:
			usage();
		}
	}

	sargc = argc;
	sargv = argv;

	if (do_fork) {
		if (pipe(p) == -1)
			err(1, "cannot create pipe");
		ret = fork();
		if (ret == -1)
			err(1, "cannot fork");
		else if (ret == 0) {
			close(STDOUT_FILENO);
			dup(p[1]);
			close(p[0]);
			close(p[1]);
		} else {
			signal(SIGINT, SIG_IGN);
			close(STDIN_FILENO);
			dup(p[0]);
			close(p[0]);
			close(p[1]);
			execl(_PATH_DC, "dc", "-", (char *)NULL);
			err(1, "cannot find dc");
		}
	}
	signal(SIGINT, abort_line);
	yywrap();
	return yyparse();
}

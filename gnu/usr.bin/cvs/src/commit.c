/*
 * Copyright (c) 1992, Brian Berliner and Jeff Polk
 * Copyright (c) 1989-1992, Brian Berliner
 * 
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS 1.4 kit.
 * 
 * Commit Files
 * 
 * "commit" commits the present version to the RCS repository, AFTER
 * having done a test on conflicts.
 *
 * The call is: cvs commit [options] files...
 * 
 */

#include <assert.h>
#include "cvs.h"
#include "getline.h"
#include "edit.h"
#include "fileattr.h"

static Dtype check_direntproc PROTO ((void *callerdat, char *dir,
				      char *repos, char *update_dir,
				      List *entries));
static int check_fileproc PROTO ((void *callerdat, struct file_info *finfo));
static int check_filesdoneproc PROTO ((void *callerdat, int err,
				       char *repos, char *update_dir,
				       List *entries));
static int checkaddfile PROTO((char *file, char *repository, char *tag,
			       char *options, RCSNode **rcsnode)); 
static Dtype commit_direntproc PROTO ((void *callerdat, char *dir,
				       char *repos, char *update_dir,
				       List *entries));
static int commit_dirleaveproc PROTO ((void *callerdat, char *dir,
				       int err, char *update_dir,
				       List *entries));
static int commit_fileproc PROTO ((void *callerdat, struct file_info *finfo));
static int commit_filesdoneproc PROTO ((void *callerdat, int err,
					char *repository, char *update_dir,
					List *entries));
static int finaladd PROTO((struct file_info *finfo, char *revision, char *tag,
			   char *options));
static int findmaxrev PROTO((Node * p, void *closure));
static int lock_RCS PROTO((char *user, RCSNode *rcs, char *rev,
			   char *repository));
static int precommit_list_proc PROTO((Node * p, void *closure));
static int precommit_proc PROTO((char *repository, char *filter));
static int remove_file PROTO ((struct file_info *finfo, char *tag,
			       char *message));
static void fix_rcs_modes PROTO((char *rcs, char *user));
static void fixaddfile PROTO((char *file, char *repository));
static void fixbranch PROTO((RCSNode *, char *branch));
static void unlockrcs PROTO((RCSNode *rcs));
static void ci_delproc PROTO((Node *p));
static void masterlist_delproc PROTO((Node *p));
static char *locate_rcs PROTO((char *file, char *repository));

struct commit_info
{
    Ctype status;			/* as returned from Classify_File() */
    char *rev;				/* a numeric rev, if we know it */
    char *tag;				/* any sticky tag, or -r option */
    char *options;			/* Any sticky -k option */
};
struct master_lists
{
    List *ulist;			/* list for Update_Logfile */
    List *cilist;			/* list with commit_info structs */
};

static int force_ci = 0;
static int got_message;
static int run_module_prog = 1;
static int aflag;
static char *tag;
static char *write_dirtag;
static char *logfile;
static List *mulist;
static char *message;
static time_t last_register_time;


static const char *const commit_usage[] =
{
    "Usage: %s %s [-nRlf] [-m msg | -F logfile] [-r rev] files...\n",
    "\t-n\tDo not run the module program (if any).\n",
    "\t-R\tProcess directories recursively.\n",
    "\t-l\tLocal directory only (not recursive).\n",
    "\t-f\tForce the file to be committed; disables recursion.\n",
    "\t-F file\tRead the log message from file.\n",
    "\t-m msg\tLog message.\n",
    "\t-r rev\tCommit to this branch or trunk revision.\n",
    NULL
};

#ifdef CLIENT_SUPPORT
/* Identify a file which needs "? foo" or a Questionable request.  */
struct question {
    /* The two fields for the Directory request.  */
    char *dir;
    char *repos;

    /* The file name.  */
    char *file;

    struct question *next;
};

struct find_data {
    List *ulist;
    int argc;
    char **argv;

    /* This is used from dirent to filesdone time, for each directory,
       to make a list of files we have already seen.  */
    List *ignlist;

    /* Linked list of files which need "? foo" or a Questionable request.  */
    struct question *questionables;

    /* Only good within functions called from the filesdoneproc.  Stores
       the repository (pointer into storage managed by the recursion
       processor.  */
    char *repository;
};

static Dtype find_dirent_proc PROTO ((void *callerdat, char *dir,
				      char *repository, char *update_dir,
				      List *entries));

static Dtype
find_dirent_proc (callerdat, dir, repository, update_dir, entries)
    void *callerdat;
    char *dir;
    char *repository;
    char *update_dir;
    List *entries;
{
    struct find_data *find_data = (struct find_data *)callerdat;

    /* initialize the ignore list for this directory */
    find_data->ignlist = getlist ();
    return R_PROCESS;
}

/* Here as a static until we get around to fixing ignore_files to pass
   it along as an argument.  */
static struct find_data *find_data_static;

static void find_ignproc PROTO ((char *, char *));

static void
find_ignproc (file, dir)
    char *file;
    char *dir;
{
    struct question *p;

    p = (struct question *) xmalloc (sizeof (struct question));
    p->dir = xstrdup (dir);
    p->repos = xstrdup (find_data_static->repository);
    p->file = xstrdup (file);
    p->next = find_data_static->questionables;
    find_data_static->questionables = p;
}

static int find_filesdoneproc PROTO ((void *callerdat, int err,
				      char *repository, char *update_dir,
				      List *entries));

static int
find_filesdoneproc (callerdat, err, repository, update_dir, entries)
    void *callerdat;
    int err;
    char *repository;
    char *update_dir;
    List *entries;
{
    struct find_data *find_data = (struct find_data *)callerdat;
    find_data->repository = repository;

    /* if this directory has an ignore list, process it then free it */
    if (find_data->ignlist)
    {
	find_data_static = find_data;
	ignore_files (find_data->ignlist, entries, update_dir, find_ignproc);
	dellist (&find_data->ignlist);
    }

    find_data->repository = NULL;

    return err;
}

static int find_fileproc PROTO ((void *callerdat, struct file_info *finfo));

/* Machinery to find out what is modified, added, and removed.  It is
   possible this should be broken out into a new client_classify function;
   merging it with classify_file is almost sure to be a mess, though,
   because classify_file has all kinds of repository processing.  */
static int
find_fileproc (callerdat, finfo)
    void *callerdat;
    struct file_info *finfo;
{
    Vers_TS *vers;
    enum classify_type status;
    Node *node;
    struct find_data *args = (struct find_data *)callerdat;
    struct logfile_info *data;
    struct file_info xfinfo;

    /* if this directory has an ignore list, add this file to it */
    if (args->ignlist)
    {
	Node *p;

	p = getnode ();
	p->type = FILES;
	p->key = xstrdup (finfo->file);
	if (addnode (args->ignlist, p) != 0)
	    freenode (p);
    }

    xfinfo = *finfo;
    xfinfo.repository = NULL;
    xfinfo.rcs = NULL;

    vers = Version_TS (&xfinfo, NULL, NULL, NULL, 0, 0);
    if (vers->ts_user == NULL
	&& vers->vn_user != NULL
	&& vers->vn_user[0] == '-')
	/* FIXME: If vn_user is starts with "-" but ts_user is
	   non-NULL, what classify_file does is print "%s should be
	   removed and is still there".  I'm not sure what it does
	   then.  We probably should do the same.  */
	status = T_REMOVED;
    else if (vers->vn_user == NULL)
    {
	if (vers->ts_user == NULL)
	    error (0, 0, "nothing known about `%s'", finfo->fullname);
	else
	    error (0, 0, "use `cvs add' to create an entry for %s",
		   finfo->fullname);
	return 1;
    }
    else if (vers->ts_user != NULL
	     && vers->vn_user != NULL
	     && vers->vn_user[0] == '0')
	/* FIXME: If vn_user is "0" but ts_user is NULL, what classify_file
	   does is print "new-born %s has disappeared" and removes the entry.
	   We probably should do the same.  */
	status = T_ADDED;
    else if (vers->ts_user != NULL
	     && vers->ts_rcs != NULL
	     && strcmp (vers->ts_user, vers->ts_rcs) != 0)
	status = T_MODIFIED;
    else
    {
	/* This covers unmodified files, as well as a variety of other
	   cases.  FIXME: we probably should be printing a message and
	   returning 1 for many of those cases (but I'm not sure
	   exactly which ones).  */
	return 0;
    }

    node = getnode ();
    node->key = xstrdup (finfo->fullname);

    data = (struct logfile_info *) xmalloc (sizeof (struct logfile_info));
    data->type = status;
    data->tag = xstrdup (vers->tag);
    data->rev_old = data->rev_new = NULL;

    node->type = UPDATE;
    node->delproc = update_delproc;
    node->data = (char *) data;
    (void)addnode (args->ulist, node);

    ++args->argc;

    freevers_ts (&vers);
    return 0;
}

static int copy_ulist PROTO ((Node *, void *));

static int
copy_ulist (node, data)
    Node *node;
    void *data;
{
    struct find_data *args = (struct find_data *)data;
    args->argv[args->argc++] = node->key;
    return 0;
}
#endif /* CLIENT_SUPPORT */

int
commit (argc, argv)
    int argc;
    char **argv;
{
    int c;
    int err = 0;
    int local = 0;

    if (argc == -1)
	usage (commit_usage);

#ifdef CVS_BADROOT
    /*
     * For log purposes, do not allow "root" to commit files.  If you look
     * like root, but are really logged in as a non-root user, it's OK.
     */
    if (geteuid () == (uid_t) 0)
    {
	struct passwd *pw;

	if ((pw = (struct passwd *) getpwnam (getcaller ())) == NULL)
	    error (1, 0, "you are unknown to this system");
	if (pw->pw_uid == (uid_t) 0)
	    error (1, 0, "cannot commit files as 'root'");
    }
#endif /* CVS_BADROOT */

    optind = 1;
    while ((c = getopt (argc, argv, "nlRm:fF:r:")) != -1)
    {
	switch (c)
	{
	    case 'n':
		run_module_prog = 0;
		break;
	    case 'm':
#ifdef FORCE_USE_EDITOR
		use_editor = TRUE;
#else
		use_editor = FALSE;
#endif
		if (message)
		{
		    free (message);
		    message = NULL;
		}

		message = xstrdup(optarg);
		break;
	    case 'r':
		if (tag)
		    free (tag);
		tag = xstrdup (optarg);
		break;
	    case 'l':
		local = 1;
		break;
	    case 'R':
		local = 0;
		break;
	    case 'f':
		force_ci = 1;
		local = 1;		/* also disable recursion */
		break;
	    case 'F':
#ifdef FORCE_USE_EDITOR
		use_editor = TRUE;
#else
		use_editor = FALSE;
#endif
		logfile = optarg;
		break;
	    case '?':
	    default:
		usage (commit_usage);
		break;
	}
    }
    argc -= optind;
    argv += optind;

    /* numeric specified revision means we ignore sticky tags... */
    if (tag && isdigit (*tag))
    {
	aflag = 1;
	/* strip trailing dots */
	while (tag[strlen (tag) - 1] == '.')
	    tag[strlen (tag) - 1] = '\0';
    }

    /* some checks related to the "-F logfile" option */
    if (logfile)
    {
	int n, logfd;
	struct stat statbuf;

	if (message)
	    error (1, 0, "cannot specify both a message and a log file");

	/* FIXME: Why is this binary?  Needs more investigation.  */
	if ((logfd = CVS_OPEN (logfile, O_RDONLY | OPEN_BINARY)) < 0)
	    error (1, errno, "cannot open log file %s", logfile);

	if (fstat(logfd, &statbuf) < 0)
	    error (1, errno, "cannot find size of log file %s", logfile);

	message = xmalloc (statbuf.st_size + 1);

	/* FIXME: Should keep reading until EOF, rather than assuming the
	   first read gets the whole thing.  */
	if ((n = read (logfd, message, statbuf.st_size + 1)) < 0)
	    error (1, errno, "cannot read log message from %s", logfile);

	(void) close (logfd);
	message[n] = '\0';
    }

#ifdef CLIENT_SUPPORT
    if (client_active) 
    {
	struct find_data find_args;

	ign_setup ();

	find_args.ulist = getlist ();
	find_args.argc = 0;
	find_args.questionables = NULL;
	find_args.ignlist = NULL;
	find_args.repository = NULL;

	err = start_recursion (find_fileproc, find_filesdoneproc,
			       find_dirent_proc, (DIRLEAVEPROC) NULL,
			       (void *)&find_args,
			       argc, argv, local, W_LOCAL, 0, 0,
			       (char *)NULL, 0);
	if (err)
	    error (1, 0, "correct above errors first!");

	if (find_args.argc == 0)
	    /* Nothing to commit.  Exit now without contacting the
	       server (note that this means that we won't print "?
	       foo" for files which merit it, because we don't know
	       what is in the CVSROOT/cvsignore file).  */
	    return 0;

	/* Now we keep track of which files we actually are going to
	   operate on, and only work with those files in the future.
	   This saves time--we don't want to search the file system
	   of the working directory twice.  */
	find_args.argv = (char **) xmalloc (find_args.argc * sizeof (char **));
	find_args.argc = 0;
	walklist (find_args.ulist, copy_ulist, &find_args);

	/* Do this before calling do_editor; don't ask for a log
	   message if we can't talk to the server.  But do it after we
	   have made the checks that we can locally (to more quickly
	   catch syntax errors, the case where no files are modified,
	   added or removed, etc.).

	   On the other hand, calling start_server before do_editor
	   means that we chew up server resources the whole time that
	   the user has the editor open (hours or days if the user
	   forgets about it), which seems dubious.  */
	start_server ();

	/*
	 * We do this once, not once for each directory as in normal CVS.
	 * The protocol is designed this way.  This is a feature.
	 */
	if (use_editor)
	    do_editor (".", &message, (char *)NULL, find_args.ulist);

	/* Run the user-defined script to verify/check information in
	 *the log message
	 */
	do_verify (message, (char *)NULL);  	

	/* We always send some sort of message, even if empty.  */
	option_with_arg ("-m", message);

	/* OK, now process all the questionable files we have been saving
	   up.  */
	{
	    struct question *p;
	    struct question *q;

	    p = find_args.questionables;
	    while (p != NULL)
	    {
		if (ign_inhibit_server || !supported_request ("Questionable"))
		{
		    cvs_output ("? ", 2);
		    if (p->dir[0] != '\0')
		    {
			cvs_output (p->dir, 0);
			cvs_output ("/", 1);
		    }
		    cvs_output (p->file, 0);
		    cvs_output ("\n", 1);
		}
		else
		{
		    send_to_server ("Directory ", 0);
		    send_to_server (p->dir[0] == '\0' ? "." : p->dir, 0);
		    send_to_server ("\012", 1);
		    send_to_server (p->repos, 0);
		    send_to_server ("\012", 1);

		    send_to_server ("Questionable ", 0);
		    send_to_server (p->file, 0);
		    send_to_server ("\012", 1);
		}
		free (p->dir);
		free (p->repos);
		free (p->file);
		q = p->next;
		free (p);
		p = q;
	    }
	}

	if (local)
	    send_arg("-l");
	if (force_ci)
	    send_arg("-f");
	if (!run_module_prog)
	    send_arg("-n");
	option_with_arg ("-r", tag);

	/* Sending only the names of the files which were modified, added,
	   or removed means that the server will only do an up-to-date
	   check on those files.  This is different from local CVS and
	   previous versions of client/server CVS, but it probably is a Good
	   Thing, or at least Not Such A Bad Thing.  */
	send_file_names (find_args.argc, find_args.argv, 0);
	send_files (find_args.argc, find_args.argv, local, 0, 0);

	send_to_server ("ci\012", 0);
	return get_responses_and_close ();
    }
#endif

    if (tag != NULL)
	tag_check_valid (tag, argc, argv, local, aflag, "");

    /* XXX - this is not the perfect check for this */
    if (argc <= 0)
	write_dirtag = tag;

    wrap_setup ();

    lock_tree_for_write (argc, argv, local, aflag);

    /*
     * Set up the master update list
     */
    mulist = getlist ();

    /*
     * Run the recursion processor to verify the files are all up-to-date
     */
    err = start_recursion (check_fileproc, check_filesdoneproc,
			   check_direntproc, (DIRLEAVEPROC) NULL, NULL, argc,
			   argv, local, W_LOCAL, aflag, 0, (char *) NULL, 1);
    if (err)
    {
	Lock_Cleanup ();
	error (1, 0, "correct above errors first!");
    }

    /*
     * Run the recursion processor to commit the files
     */
    if (noexec == 0)
	err = start_recursion (commit_fileproc, commit_filesdoneproc,
			       commit_direntproc, commit_dirleaveproc, NULL,
			       argc, argv, local, W_LOCAL, aflag, 0,
			       (char *) NULL, 1);

    /*
     * Unlock all the dirs and clean up
     */
    Lock_Cleanup ();
    dellist (&mulist);

    if (last_register_time)
    {
	time_t now;

	(void) time (&now);
	if (now == last_register_time) 
	{
	    sleep (1);			/* to avoid time-stamp races */
	}
    }

    return (err);
}

/* This routine determines the status of a given file and retrieves
   the version information that is associated with that file. */

static
Ctype
classify_file_internal (finfo, vers)
    struct file_info *finfo;
    Vers_TS **vers;
{
    int save_noexec, save_quiet, save_really_quiet;
    Ctype status;
    
    save_noexec = noexec;
    save_quiet = quiet;
    save_really_quiet = really_quiet;
    noexec = quiet = really_quiet = 1;

    /* handle specified numeric revision specially */
    if (tag && isdigit (*tag))
    {
	/* If the tag is for the trunk, make sure we're at the head */
	if (numdots (tag) < 2)
	{
	    status = Classify_File (finfo, (char *) NULL, (char *) NULL,
				    (char *) NULL, 1, aflag, vers, 0);
	    if (status == T_UPTODATE || status == T_MODIFIED ||
		status == T_ADDED)
	    {
		Ctype xstatus;

		freevers_ts (vers);
		xstatus = Classify_File (finfo, tag, (char *) NULL,
					 (char *) NULL, 1, aflag, vers, 0);
		if (xstatus == T_REMOVE_ENTRY)
		    status = T_MODIFIED;
		else if (status == T_MODIFIED && xstatus == T_CONFLICT)
		    status = T_MODIFIED;
		else
		    status = xstatus;
	    }
	}
	else
	{
	    char *xtag, *cp;

	    /*
	     * The revision is off the main trunk; make sure we're
	     * up-to-date with the head of the specified branch.
	     */
	    xtag = xstrdup (tag);
	    if ((numdots (xtag) & 1) != 0)
	    {
		cp = strrchr (xtag, '.');
		*cp = '\0';
	    }
	    status = Classify_File (finfo, xtag, (char *) NULL,
				    (char *) NULL, 1, aflag, vers, 0);
	    if ((status == T_REMOVE_ENTRY || status == T_CONFLICT)
		&& (cp = strrchr (xtag, '.')) != NULL)
	    {
		/* pluck one more dot off the revision */
		*cp = '\0';
		freevers_ts (vers);
		status = Classify_File (finfo, xtag, (char *) NULL,
					(char *) NULL, 1, aflag, vers, 0);
		if (status == T_UPTODATE || status == T_REMOVE_ENTRY)
		    status = T_MODIFIED;
	    }
	    /* now, muck with vers to make the tag correct */
	    free ((*vers)->tag);
	    (*vers)->tag = xstrdup (tag);
	    free (xtag);
	}
    }
    else
	status = Classify_File (finfo, tag, (char *) NULL, (char *) NULL,
				1, 0, vers, 0);
    noexec = save_noexec;
    quiet = save_quiet;
    really_quiet = save_really_quiet;

    return status;
}

/*
 * Check to see if a file is ok to commit and make sure all files are
 * up-to-date
 */
/* ARGSUSED */
static int
check_fileproc (callerdat, finfo)
    void *callerdat;
    struct file_info *finfo;
{
    Ctype status;
    char *xdir;
    Node *p;
    List *ulist, *cilist;
    Vers_TS *vers;
    struct commit_info *ci;
    struct logfile_info *li;
    
    status = classify_file_internal (finfo, &vers);

    /*
     * If the force-commit option is enabled, and the file in question
     * appears to be up-to-date, just make it look modified so that
     * it will be committed.
     */
    if (force_ci && status == T_UPTODATE)
	status = T_MODIFIED;

    switch (status)
    {
	case T_CHECKOUT:
#ifdef SERVER_SUPPORT
	case T_PATCH:
#endif
	case T_NEEDS_MERGE:
	case T_CONFLICT:
	case T_REMOVE_ENTRY:
	    error (0, 0, "Up-to-date check failed for `%s'", finfo->fullname);
	    freevers_ts (&vers);
	    return (1);
	case T_MODIFIED:
	case T_ADDED:
	case T_REMOVED:
	    /*
	     * some quick sanity checks; if no numeric -r option specified:
	     *	- can't have a sticky date
	     *	- can't have a sticky tag that is not a branch
	     * Also,
	     *	- if status is T_REMOVED, can't have a numeric tag
	     *	- if status is T_ADDED, rcs file must not exist unless on
	     *    a branch
	     *	- if status is T_ADDED, can't have a non-trunk numeric rev
	     *	- if status is T_MODIFIED and a Conflict marker exists, don't
	     *    allow the commit if timestamp is identical or if we find
	     *    an RCS_MERGE_PAT in the file.
	     */
	    if (!tag || !isdigit (*tag))
	    {
		if (vers->date)
		{
		    error (0, 0,
			   "cannot commit with sticky date for file `%s'",
			   finfo->fullname);
		    freevers_ts (&vers);
		    return (1);
		}
		if (status == T_MODIFIED && vers->tag &&
		    !RCS_isbranch (finfo->rcs, vers->tag))
		{
		    error (0, 0,
			   "sticky tag `%s' for file `%s' is not a branch",
			   vers->tag, finfo->fullname);
		    freevers_ts (&vers);
		    return (1);
		}
	    }
	    if (status == T_MODIFIED && !force_ci && vers->ts_conflict)
	    {
		char *filestamp;
		int retcode;

		/*
		 * We found a "conflict" marker.
		 *
		 * If the timestamp on the file is the same as the
		 * timestamp stored in the Entries file, we block the commit.
		 */
#ifdef SERVER_SUPPORT
		if (server_active)
		    retcode = vers->ts_conflict[0] != '=';
		else {
		    filestamp = time_stamp (finfo->file);
		    retcode = strcmp (vers->ts_conflict, filestamp);
		    free (filestamp);
		}
#else
		filestamp = time_stamp (finfo->file);
		retcode = strcmp (vers->ts_conflict, filestamp);
		free (filestamp);
#endif
		if (retcode == 0)
		{
		    error (0, 0,
			  "file `%s' had a conflict and has not been modified",
			   finfo->fullname);
		    freevers_ts (&vers);
		    return (1);
		}

		if (file_has_markers (finfo))
		{
		    error (0, 0,
			   "file `%s' still contains conflict indicators",
			   finfo->fullname);d673 2
		    freevers_ts (&vers);
		    return (1);
		}
	    }

	    if (status == T_REMOVED && vers->tag && isdigit (*vers->tag))
	    {
		if (finfo->update_dir[0] == '\0')
		    error (0, 0,
		error (0, 0,
	"cannot remove file `%s' which has a numeric sticky tag of `%s'",
			   finfo->fullname, vers->tag);
		freevers_ts (&vers);
		return (1);
	    }
	        if (vers->tag == NULL)
		sprintf(rcs, "%s/%s%s", finfo->repository, finfo->file, RCSEXT);
		    char *rcs;

		    rcs = xmalloc (strlen (finfo->repository)
				   + strlen (finfo->file)
				   + sizeof RCSEXT
				   + 5);

		    /* Don't look in the attic; if it exists there we
		       will move it back out in checkaddfile.  */
		    sprintf(rcs, "%s/%s%s", finfo->repository, finfo->file,
			    RCSEXT);
		    if (isreadable (rcs))
		    {
			error (0, 0,
		    "cannot add file `%s' when RCS file `%s' already exists",
			       finfo->fullname, rcs);
			freevers_ts (&vers);
			free (rcs);
			return (1);
		    }
		    free (rcs);
		    freevers_ts (&vers);
		    return (1);
		}
		if (vers->tag && isdigit (*vers->tag) &&
		    numdots (vers->tag) > 1)
		{
		    error (0, 0,
		"cannot add file `%s' with revision `%s'; must be on trunk",
			       finfo->fullname, vers->tag);
		    freevers_ts (&vers);
		    return (1);
		}
	    }

	    /* done with consistency checks; now, to get on with the commit */
	    if (finfo->update_dir[0] == '\0')
		xdir = ".";
	    else
		xdir = finfo->update_dir;
	    if ((p = findnode (mulist, xdir)) != NULL)
	    {
		ulist = ((struct master_lists *) p->data)->ulist;
		cilist = ((struct master_lists *) p->data)->cilist;
	    }
	    else
	    {
		struct master_lists *ml;

		ulist = getlist ();
		cilist = getlist ();
		p = getnode ();
		p->key = xstrdup (xdir);
		p->type = UPDATE;
		ml = (struct master_lists *)
		    xmalloc (sizeof (struct master_lists));
		ml->ulist = ulist;
		ml->cilist = cilist;
		p->data = (char *) ml;
		p->delproc = masterlist_delproc;
		(void) addnode (mulist, p);
	    }

	    /* first do ulist, then cilist */
	    p = getnode ();
	    p->key = xstrdup (finfo->file);
	    li = ((struct logfile_info *)
		  xmalloc (sizeof (struct logfile_info)));
	    li->type = status;
	    li->tag = xstrdup (vers->tag);
	    li->rev_old = xstrdup (vers->vn_rcs);
	    li->rev_new = NULL;
	    p->data = (char *) li;
	    p->delproc = update_delproc;
	    p->data = (char *) status;
	    (void) addnode (ulist, p);

	    p = getnode ();
	    p->key = xstrdup (finfo->file);
	    p->type = UPDATE;
	    p->delproc = ci_delproc;
	    ci = (struct commit_info *) xmalloc (sizeof (struct commit_info));
	    ci->status = status;
	    if (vers->tag)
		if (isdigit (*vers->tag))
		    ci->rev = xstrdup (vers->tag);
		else
		    ci->rev = RCS_whatbranch (finfo->rcs, vers->tag);
	    else
		ci->rev = (char *) NULL;
	    ci->tag = xstrdup (vers->tag);
	    ci->options = xstrdup(vers->options);
	    p->data = (char *) ci;
	    (void) addnode (cilist, p);
	    break;
	case T_UNKNOWN:
	    error (0, 0, "nothing known about `%s'", finfo->fullname);
	    freevers_ts (&vers);
	    return (1);
	case T_UPTODATE:
	    break;
	default:
	    error (0, 0, "CVS internal error: unknown status %d", status);
	    break;
    }

    freevers_ts (&vers);
    return (0);
}

/*
 * Print warm fuzzies while examining the dirs
 */
check_direntproc (callerdat, dir, repos, update_dir, entries)
    void *callerdat;
static Dtype
check_direntproc (dir, repos, update_dir)
    char *dir;
    List *entries;
    char *repos;
    char *update_dir;
{
    if (!quiet)
	error (0, 0, "Examining %s", update_dir);

    return (R_PROCESS);
}

/*
 * Walklist proc to run pre-commit checks
 */
static int
precommit_list_proc (p, closure)
    Node *p;
    struct logfile_info *li;

    li = (struct logfile_info *) p->data;
    if (li->type == T_ADDED
	|| li->type == T_MODIFIED
	|| li->type == T_REMOVED)
    if (p->data == (char *) T_ADDED || p->data == (char *) T_MODIFIED ||
	p->data == (char *) T_REMOVED)
    {
	run_arg (p->key);
    }
    return (0);
}

/*
 * Callback proc for pre-commit checking
 */
static List *ulist;
static int
precommit_proc (repository, filter)
    char *repository;
    char *filter;
{
    /* see if the filter is there, only if it's a full path */
    if (isabsolute (filter))
    {
    	char *s, *cp;
	
	s = xstrdup (filter);
	for (cp = s; *cp; cp++)
	    if (isspace (*cp))
	    {
		*cp = '\0';
		break;
	    }
	if (!isfile (s))
	{
	    error (0, errno, "cannot find pre-commit filter `%s'", s);
	    free (s);
	    return (1);			/* so it fails! */
	}
	free (s);
    }

    run_setup ("%s %s", filter, repository);
    (void) walklist (ulist, precommit_list_proc, NULL);
    return (run_exec (RUN_TTY, RUN_TTY, RUN_TTY, RUN_NORMAL|RUN_REALLY));
}

/*
 * Run the pre-commit checks for the dir
 */
check_filesdoneproc (callerdat, err, repos, update_dir, entries)
    void *callerdat;
static int
check_filesdoneproc (err, repos, update_dir)
    int err;
    List *entries;
    char *repos;
    char *update_dir;
{
    int n;
    Node *p;

    /* find the update list for this dir */
    p = findnode (mulist, update_dir);
    if (p != NULL)
	ulist = ((struct master_lists *) p->data)->ulist;
    else
	ulist = (List *) NULL;

    /* skip the checks if there's nothing to do */
    if (ulist == NULL || ulist->list->next == ulist->list)
	return (err);

    /* run any pre-commit checks */
    if ((n = Parse_Info (CVSROOTADM_COMMITINFO, repos, precommit_proc, 1)) > 0)
    {
	error (0, 0, "Pre-commit check failed");
	err += n;
    }

    return (err);
}

/*
 * Do the work of committing a file
static char *sbranch;
static int maxrev;
static char sbranch[PATH_MAX];

commit_fileproc (callerdat, finfo)
    void *callerdat;
static int
commit_fileproc (finfo)
    struct file_info *finfo;
{
    Node *p;
    int err = 0;
    struct commit_info *ci;
    char rcs[PATH_MAX];

    if (finfo->update_dir[0] == '\0')
	p = findnode (mulist, ".");
    else
	p = findnode (mulist, finfo->update_dir);

    /*
     * if p is null, there were file type command line args which were
     * all up-to-date so nothing really needs to be done
     */
    if (p == NULL)
	return (0);
    ulist = ((struct master_lists *) p->data)->ulist;
    cilist = ((struct master_lists *) p->data)->cilist;

    /*
     * At this point, we should have the commit message unless we were called
     * with files as args from the command line.  In that latter case, we
    if (!(got_message))
    {
    if (use_editor && !got_message)
	if (use_editor)
	    do_editor (finfo->update_dir, &message, finfo->repository, ulist);
	do_verify (message, finfo->repository);  
    }
	do_editor (finfo->update_dir, &message, finfo->repository, ulist);
      }

    p = findnode (cilist, finfo->file);
    if (p == NULL)
	return (0);

    ci = (struct commit_info *) p->data;
	if (finfo->rcs == NULL)
	    error (1, 0, "internal error: no parsed RCS file");
	if (lock_RCS (finfo->file, finfo->rcs, ci->rev,
		      finfo->repository) != 0)
    {
	    unlockrcs (finfo->rcs);
	{
	    unlockrcs (finfo->file, finfo->repository);
	    err = 1;
	    goto out;
	}
    }
    else if (ci->status == T_ADDED)
    {
	if (checkaddfile (finfo->file, finfo->repository, ci->tag, ci->options,
			  &finfo->rcs) != 0)
	{
	    fixaddfile (finfo->file, finfo->repository);
	    err = 1;
	    goto out;
	}

	/* adding files with a tag, now means adding them on a branch.
	   Since the branch test was done in check_fileproc for
	if (ci->tag)
	{
	    if (finfo->rcs == NULL)
		error (1, 0, "internal error: no parsed RCS file");
	if (ci->tag) {
	    err = Checkin ('A', finfo, finfo->rcs->path, ci->rev,
			   ci->tag, ci->options, message);
	    err = Checkin ('A', finfo->file, finfo->update_dir, finfo->repository, rcs, ci->rev,
			   ci->tag, ci->options, message, finfo->entries);
		unlockrcs (finfo->rcs);
		fixbranch (finfo->rcs, sbranch);
		unlockrcs (finfo->file, finfo->repository);
		fixbranch (finfo->file, finfo->repository, sbranch);
	    }

	    (void) time (&last_register_time);

	    ci->status = T_UPTODATE;
	}
    }

    /*
     * Add the file for real
     */
    if (ci->status == T_ADDED)
    {
	char *xrev = (char *) NULL;

	if (ci->rev == NULL)
	{
	    /* find the max major rev number in this directory */
	    maxrev = 0;
	    (void) walklist (finfo->entries, findmaxrev, NULL);
	    if (maxrev == 0)
		maxrev = 1;
	    xrev = xmalloc (20);
	    (void) sprintf (xrev, "%d", maxrev);
	}
	err = finaladd (finfo, ci->rev ? ci->rev : xrev, ci->tag, ci->options);
	err = finaladd (finfo->file, ci->rev ? ci->rev : xrev, ci->tag, ci->options,
			finfo->update_dir, finfo->repository, finfo->entries);
	if (xrev)
	    free (xrev);
    }
	err = Checkin ('M', finfo,
		       finfo->rcs->path, ci->rev, ci->tag,
		       ci->options, message);
		       rcs, ci->rev, ci->tag,
		       ci->options, message, finfo->entries);

	(void) time (&last_register_time);

	    unlockrcs (finfo->rcs);
	    fixbranch (finfo->rcs, sbranch);
	    unlockrcs (finfo->file, finfo->repository);
	    fixbranch (finfo->file, finfo->repository, sbranch);
	}
    }
	err = remove_file (finfo, ci->tag, message);
	err = remove_file (finfo->file, finfo->repository, ci->tag, message,
			   finfo->entries, finfo->rcs);
#ifdef SERVER_SUPPORT
	    server_updated (finfo,
			    NULL,

	    server_scratch_entry_only ();
			    SERVER_UPDATED,

			    (struct stat *) NULL,
			    /* Doesn't matter, it won't get checked.  */
			    SERVER_UPDATED, (struct stat *) NULL,
			    (unsigned char *) NULL);
	}
#endif
    }

    /* Clearly this is right for T_MODIFIED.  I haven't thought so much
       about T_ADDED or T_REMOVED.  */
    notify_do ('C', finfo->file, getcaller (), NULL, NULL, finfo->repository);

out:
    if (err != 0)
    {
	/* on failure, remove the file from ulist */
	p = findnode (ulist, finfo->file);
	if (p)
    else
    {
	/* On success, retrieve the new version number of the file and
           copy it into the log information (see logmsg.c
           (logfile_write) for more details).  We should only update
           the version number for files that have been added or
           modified but not removed.  Why?  classify_file_internal
           will return the version number of a file even after it has
           been removed from the archive, which is not the behavior we
           want for our commitlog messages; we want the old version
           number and then "NONE." */
	
	if (ci->status != T_REMOVED)
	{
	    p = findnode (ulist, finfo->file);
	    if (p)
	    {
		Vers_TS *vers;
		struct logfile_info *li;
	    
		(void) classify_file_internal (finfo, &vers);
		li = (struct logfile_info *) p->data;
		li->rev_new = xstrdup (vers->vn_rcs);
		freevers_ts (&vers);
	    }
	}
    }
	    delnode (p);
    }

    return (err);
}

/*
 * Log the commit and clean up the update list
 */
commit_filesdoneproc (callerdat, err, repository, update_dir, entries)
    void *callerdat;
static int
commit_filesdoneproc (err, repository, update_dir)
    int err;
    List *entries;
    char *repository;
{
    char *xtag = (char *) NULL;
    Node *p;
    List *ulist;

    p = findnode (mulist, update_dir);
    if (p == NULL)
	return (err);

    ulist = ((struct master_lists *) p->data)->ulist;


    Update_Logfile (repository, message, (FILE *) 0, ulist);
    if (xtag)
	free (xtag);

    /* Build the administrative files if necessary.  */
    {
	if (strncmp (CVSroot_directory, repository,
		     strlen (CVSroot_directory)) != 0)
	    error (0, 0, "internal error: repository (%s) doesn't begin with root (%s)", repository, CVSroot_directory);
	p = repository + strlen (CVSroot_directory);
	    error (0, 0, "internal error: repository doesn't begin with root");
	p = repository + strlen (CVSroot);
	if (*p == '/')
	    ++p;
	if (strcmp ("CVSROOT", p) == 0)
	{
	    /* "Database" might a little bit grandiose and/or vague,
	       but "checked-out copies of administrative files, unless
	       in the case of modules and you are using ndbm in which
	       case modules.{pag,dir,db}" is verbose and excessively
	       focused on how the database is implemented.  */

	    cvs_output (program_name, 0);
	    cvs_output (" ", 1);
	    cvs_output (command_name, 0);
	    cvs_output (": Rebuilding administrative file database\n", 0);
	    mkmodules (repository);
	}
    }

    if (err == 0 && run_module_prog)
    {
	if ((fp = CVS_FOPEN (CVSADM_CIPROG, "r")) != NULL)

	if ((fp = fopen (CVSADM_CIPROG, "r")) != NULL)
	{
	    char *line;
	    int line_length;
	    size_t line_chars_allocated;
	    char *repository;

	    line = NULL;
	    line_chars_allocated = 0;
	    line_length = getline (&line, &line_chars_allocated, fp);
	    if (line_length > 0)
	    {
		/* Remove any trailing newline.  */
		if (line[line_length - 1] == '\n')
		    line[--line_length] = '\0';
		cvs_output (program_name, 0);
		cvs_output (" ", 1);
		cvs_output (command_name, 0);
		cvs_output (": Executing '", 0);
		(void) printf ("%s %s: Executing '", program_name,
		cvs_output ("'\n", 0);
		run_print (stdout);
		(void) printf ("'\n");
		(void) run_exec (RUN_TTY, RUN_TTY, RUN_TTY, RUN_NORMAL);
		free (repository);
	    }
	    else
	    {
		if (ferror (fp))
		    error (0, errno, "warning: error reading %s",
			   CVSADM_CIPROG);
	    }
	    if (line != NULL)
		free (line);
	    if (fclose (fp) < 0)
		error (0, errno, "warning: cannot close %s", CVSADM_CIPROG);
	}
	else
	{
	    if (! existence_error (errno))
		error (0, errno, "warning: cannot open %s", CVSADM_CIPROG);
	}
    }

    return (err);
}

/*
 * Get the log message for a dir and print a warm fuzzy
 */
commit_direntproc (callerdat, dir, repos, update_dir, entries)
    void *callerdat;
static Dtype
commit_direntproc (dir, repos, update_dir)
    char *dir;
    List *entries;
    char *repos;
    char *update_dir;
{
    Node *p;
    List *ulist;
    char *real_repos;

    /* find the update list for this dir */
    p = findnode (mulist, update_dir);
    if (p != NULL)
	ulist = ((struct master_lists *) p->data)->ulist;
    else
	ulist = (List *) NULL;

    /* skip the files as an optimization */
    if (ulist == NULL || ulist->list->next == ulist->list)
	return (R_SKIP_FILES);

    /* print the warm fuzzy */
    if (!quiet)
	error (0, 0, "Committing %s", update_dir);
    real_repos = Name_Repository (dir, update_dir);
    got_message = 1;

	got_message = 1;
    do_verify (message, real_repos);  
    free (real_repos);
	free (real_repos);
    }
    return (R_PROCESS);
}

/*
 * Process the post-commit proc if necessary
 */
commit_dirleaveproc (callerdat, dir, err, update_dir, entries)
    void *callerdat;
static int
commit_dirleaveproc (dir, err, update_dir)
    char *dir;
    List *entries;
    int err;
    char *update_dir;
{
    /* update the per-directory tag info */
    if (err == 0 && write_dirtag != NULL)
    {
	WriteTag ((char *) NULL, write_dirtag, (char *) NULL);
#ifdef SERVER_SUPPORT
	if (server_active)
	    server_set_sticky (update_dir, Name_Repository (dir, update_dir),
			       write_dirtag, (char *) NULL);
#endif
    }

    return (err);
}

/*
 * find the maximum major rev number in an entries file
 */
static int
findmaxrev (p, closure)
    Node *p;
    void *closure;
{
    char *cp;
    int thisrev;
    Entnode *entdata;
    if (entdata->type != ENT_FILE)
	return (0);

    entdata = (Entnode *) p->data;
    cp = strchr (entdata->version, '.');
    if (cp != NULL)
	*cp = '\0';
    thisrev = atoi (entdata->version);
    if (cp != NULL)
	*cp = '.';
    if (thisrev > maxrev)
	maxrev = thisrev;
    return (0);
}

/*
 * Actually remove a file by moving it to the attic
 * XXX - if removing a ,v file that is a relative symbolic link to
 * another ,v file, we probably should add a ".." component to the
 * link to keep it relative after we move it into the attic.
remove_file (finfo, tag, message)
    struct file_info *finfo;
    char *file;
    char *repository;
    List *entries;
    RCSNode *rcsnode;
{
    int retcode;
    char rcs[PATH_MAX];
    char *tmp;

    int branch;
    int lockflag;
    char *corev;
    char *old_path;
    char *rev;
    char *prev_rev;

    corev = NULL;
    rev = NULL;
    prev_rev = NULL;

    if (finfo->rcs == NULL)
	error (1, 0, "internal error: no parsed RCS file");

    locate_rcs (file, repository, rcs);
    if (tag && !(branch = RCS_isbranch (finfo->rcs, tag)))
    branch = 0;
    if (tag && !(branch = RCS_isbranch (rcsnode, tag)))
	if ((retcode = RCS_deltag (finfo->rcs, tag, 1)) != 0) 
	/* a symbolic tag is specified; just remove the tag from the file */
	if ((retcode = RCS_deltag (rcs, tag, 1)) != 0) 
	{
		       "failed to remove tag `%s' from `%s'", tag,
		       finfo->fullname);
		error (0, retcode == -1 ? errno : 0,
		       "failed to remove tag `%s' from `%s'", tag, rcs);
	Scratch_Entry (finfo->entries, finfo->file);
	}
	Scratch_Entry (entries, file);
	return (0);
    }

    /* we are removing the file from either the head or a branch */
    /* commit a new, dead revision. */
    cvs_output ("Removing ", 0);
    cvs_output (finfo->fullname, 0);
    cvs_output (";\n", 0);
    /* Print message indicating that file is going to be removed. */
    (void) printf ("Removing %s;\n", file);
    lockflag = 1;
    rev = NULL;
    lockflag = RCS_FLAGS_LOCK;
    if (branch)
    {
	rev = RCS_whatbranch (finfo->rcs, tag);

	rev = RCS_whatbranch (rcsnode, tag);
	if (rev == NULL)
	{
	    error (0, 0, "cannot find branch \"%s\".", tag);
	    return (1);
	branchname = RCS_getbranch (finfo->rcs, rev, 1);
	}
	branchname = RCS_getbranch (rcsnode, rev, 1);
	if (branchname == NULL)
	{
	    corev = RCS_gettag (finfo->rcs, tag, 1, (int *) NULL);
	       revision but do not lock. */
	    corev = RCS_gettag (rcsnode, tag, 1, 0);
	    prev_rev = xstrdup(rev);
	    lockflag = 0;
	} else
	{
	    corev = xstrdup (rev);
	    prev_rev = xstrdup(branchname);
	    free (branchname);
	}

    {
	prev_rev = RCS_head (finfo->rcs);
	}
	prev_rev = RCS_head (rcsnode);
    }
    
    /* if removing without a tag or a branch, then make sure the default
       branch is the trunk. */
        if (RCS_setbranch (finfo->rcs, NULL) != 0) 
    {
        if (RCS_setbranch (rcs, NULL) != 0) 
		   finfo->fullname);
	    error (0, 0, "cannot change branch to default for %s",
		   rcs);
	    return (1);
	}
    }

#ifdef SERVER_SUPPORT
    if (server_active) {
	/* If this is the server, there will be a file sitting in the
	   temp directory which is the kludgy way in which server.c
	unlink_file (finfo->file);
	   it so we can create temp files with that name (ignore errors).  */
	unlink_file (file);
    }
#endif
       particular rev, then name it.  */
    retcode = RCS_checkout (finfo->rcs, finfo->file, rev ? corev : NULL,
			    (char *) NULL, (char *) NULL, RUN_TTY);
    retcode = RCS_checkout (rcs, "", rev ? corev : NULL, NULL, RUN_TTY,
                            lockflag, 1);
    if (retcode != 0)
    {
		   "failed to check out `%s'", finfo->fullname);
	    error (0, retcode == -1 ? errno : 0,
		   "failed to check out `%s'", rcs);
	return (1);
    /* Except when we are creating a branch, lock the revision so that
       we can check in the new revision.  */
    if (lockflag)
	RCS_lock (finfo->rcs, rev ? corev : NULL, 0);

    }

    if (corev != NULL)
    retcode = RCS_checkin (finfo->rcs->path, finfo->file, message, rev,
			   RCS_FLAGS_DEAD | RCS_FLAGS_QUIET);

    retcode = RCS_checkin (rcs, NULL, message, rev, RCS_FLAGS_DEAD, 1);
    if (retcode	!= 0)
    {
		   "failed to commit dead revision for `%s'", finfo->fullname);
	    error (0, retcode == -1 ? errno : 0,
		   "failed to commit dead revision for `%s'", rcs);
	return (1);
    }

    if (rev != NULL)
    old_path = finfo->rcs->path;
	free (rev);

    if (!branch)
	tmp = xmalloc(strlen(finfo->repository) + 
	/* this was the head; really move it into the Attic */
	tmp = xmalloc(strlen(repository) + 
		      sizeof('/') +
		      strlen(finfo->file) +
		      sizeof('/') +
	(void) sprintf (tmp, "%s/%s", finfo->repository, CVSATTIC);
		      sizeof(RCSEXT) + 1);
	(void) sprintf (tmp, "%s/%s", repository, CVSATTIC);
	omask = umask (cvsumask);
	(void) sprintf (tmp, "%s/%s/%s%s", finfo->repository, CVSATTIC, finfo->file, RCSEXT);
	(void) umask (omask);
	if (strcmp (finfo->rcs->path, tmp) != 0
	    && CVS_RENAME (finfo->rcs->path, tmp) == -1
	    && (isreadable (finfo->rcs->path) || !isreadable (tmp)))
	    && rename (rcs, tmp) == -1
	    && (isreadable (rcs) || !isreadable (tmp)))
	{
	    free(tmp);
	/* The old value of finfo->rcs->path is in old_path, and is
           freed below.  */
	finfo->rcs->path = tmp;
	}
	free(tmp);
    }
    cvs_output (old_path, 0);
    cvs_output ("  <--  ", 0);
    cvs_output (finfo->file, 0);
    cvs_output ("\nnew revision: delete; previous revision: ", 0);
    cvs_output (prev_rev, 0);
    cvs_output ("\ndone\n", 0);
    (void) printf ("previous revision: %s\n", prev_rev);
    (void) printf ("done\n");
    if (old_path != finfo->rcs->path)
	free (old_path);

    Scratch_Entry (finfo->entries, finfo->file);

    Scratch_Entry (entries, file);
    return (0);
}

/*
 * Do the actual checkin for added files
finaladd (finfo, rev, tag, options)
    struct file_info *finfo;
finaladd (file, rev, tag, options, update_dir, repository, entries)
    char *file;
    char *rev;
    char *repository;
    List *entries;
    char *rcs;
    char tmp[PATH_MAX];
    rcs = locate_rcs (finfo->file, finfo->repository);
    ret = Checkin ('A', finfo, rcs, rev, tag, options, message);
    ret = Checkin ('A', file, update_dir, repository, rcs, rev, tag, options,
		   message, entries);
	char *tmp = xmalloc (strlen (finfo->file) + sizeof (CVSADM)
			     + sizeof (CVSEXT_LOG) + 10);
	(void) sprintf (tmp, "%s/%s%s", CVSADM, finfo->file, CVSEXT_LOG);
    {
	free (tmp);
	(void) sprintf (tmp, "%s/%s%s", CVSADM, file, CVSEXT_LOG);
	(void) unlink_file (tmp);
	fixaddfile (finfo->file, finfo->repository);
    else
	fixaddfile (file, repository);
    free (rcs);

    (void) time (&last_register_time);

    return (ret);
}

/*
 * Unlock an rcs file
unlockrcs (rcs)
    RCSNode *rcs;
    char *file;
    int retcode;

    locate_rcs (file, repository, rcs);

	       "could not unlock %s", rcs->path);
	error (retcode == -1 ? 1 : 0, retcode == -1 ? errno : 0,
	       "could not unlock %s", rcs);
}

/*
 * remove a partially added file.  if we can parse it, leave it alone.
 */
static void
fixaddfile (file, repository)
    char *file;
    char *repository;
    char *rcs;
    RCSNode *rcsfile;
    char rcs[PATH_MAX];
    rcs = locate_rcs (file, repository);

    locate_rcs (file, repository, rcs);
    save_really_quiet = really_quiet;
    really_quiet = 1;
    if ((rcsfile = RCS_parsercsfile (rcs)) == NULL)
	(void) unlink_file (rcs);
    else
    free (rcs);
	freercsnode (&rcsfile);
    really_quiet = save_really_quiet;
}

/*
 * put the branch back on an rcs file
fixbranch (rcs, branch)
    RCSNode *rcs;
    char *file;
    char *repository;
    int retcode;
    char rcs[PATH_MAX];
    if (branch != NULL)

    {
	locate_rcs (file, repository, rcs);
		   "cannot restore branch to %s for %s", branch, rcs->path);
	    error (retcode == -1 ? 1 : 0, retcode == -1 ? errno : 0,
		   "cannot restore branch to %s for %s", branch, rcs);
    }
}

/*
 * do the initial part of a file add for the named file.  if adding
 * with a tag, put the file in the Attic and point the symbolic tag
 * at the committed revision.
 */

static int
checkaddfile (file, repository, tag, options, rcsnode)
    char *file;
    char *repository;
    char *tag;
    char *options;
    char *rcs;
    char *fname;
    char rcs[PATH_MAX];
    char fname[PATH_MAX];
    mode_t omask;
    RCSNode *rcsfile = NULL;
    int retval;
    int retcode = 0;
    int newfile = 0;

	rcs = xmalloc (strlen (repository) + strlen (file)
		       + sizeof (RCSEXT) + sizeof (CVSATTIC) + 10);
        (void) sprintf (rcs, "%s/%s%s", repository, file, RCSEXT);
	if (! isreadable (rcs))
	{
	    (void) sprintf(rcs, "%s/%s", repository, CVSATTIC);
	    omask = umask (cvsumask);
	    if (CVS_MKDIR (rcs, 0777) != 0 && errno != EEXIST)
		error (1, errno, "cannot make directory `%s'", rcs);;
	    (void) umask (omask);
	    (void) sprintf (rcs, "%s/%s/%s%s", repository, CVSATTIC, file,
			    RCSEXT);
	}
	(void) umask (omask);
	(void) sprintf (rcs, "%s/%s/%s%s", repository, CVSATTIC, file, RCSEXT);
	rcs = locate_rcs (file, repository);
    else
    if (isreadable (rcs))

    if (isreadable(rcs))
	/* file has existed in the past.  Prepare to resurrect. */

	if ((rcsfile = *rcsnode) == NULL)
	{
	    error (0, 0, "could not find parsed rcsfile %s", file);
	    retval = 1;
	    goto out;
	}
	char *rev;
	RCSNode *rcsfile;

	    char *oldfile;

	if (tag == NULL)
	{
	    oldfile = xstrdup (rcs);
	       Attic. */
	    strcpy (oldfile, rcs);
	    if (strcmp (oldfile, rcs) == 0)
	    {
		error (0, 0, "internal error: confused about attic for %s",
		       oldfile);
	    out1:
		free (oldfile);
		retval = 1;
		goto out;
	    }
	    if (CVS_RENAME (oldfile, rcs) != 0)
	    {
		error (0, errno, "failed to move `%s' out of the attic",
		       oldfile);
		goto out1;
	    }
	    if (isreadable (oldfile)
		|| rename (oldfile, rcs) != 0
		|| isreadable (oldfile)
		error (0, 0, "\
internal error: `%s' didn't move out of the attic",
		       oldfile);
		goto out1;
		       file);
	    free (oldfile);
	    free (rcsfile->path);
	    rcsfile->path = xstrdup (rcs);
		return (1);
	    }
	rev = RCS_getversion (rcsfile, tag, NULL, 1, (int *) NULL);

	if (lock_RCS (file, rcsfile, rev, repository))
	{
	/* and lock it */
	    if (rev != NULL)
		free (rev);
	    retval = 1;
	    goto out;
	    free (rev);
	    return (1);
	if (rev != NULL)
	    free (rev);
    }
    else
    {
	free (rev);
    } else {
	/* this is the first time we have ever seen this file; create
	   an rcs file.  */
	fname = xmalloc (strlen (file) + sizeof (CVSADM)
			 + sizeof (CVSEXT_LOG) + 10);
	run_setup ("%s%s -x,v/ -i", Rcsbin, RCS);

	(void) sprintf (fname, "%s/%s%s", CVSADM, file, CVSEXT_LOG);
	/* If the file does not exist, no big deal.  In particular, the
	   server does not (yet at least) create CVSEXT_LOG files.  */
	free (fname);
	if (isfile (fname))
	    run_args ("-t%s/%s%s", CVSADM, file, CVSEXT_LOG);

	/* Set RCS keyword expansion options.  */
	if (options && options[0] == '-' && options[1] == 'k')
	    run_arg (options);
	run_arg (rcs);
	if ((retcode = run_exec (RUN_TTY, RUN_TTY, RUN_TTY, RUN_NORMAL)) != 0)
	{
	    retval = 1;
	    goto out;
		   "could not create %s", rcs);
	    return (1);
	}
	newfile = 1;
    }

    /* when adding a file for the first time, and using a tag, we need
       to create a dead revision on the trunk.  */
    if (tag && newfile)
    {
	char *tmp;
	fname = xmalloc (strlen (file) + sizeof (CVSADM)
			 + sizeof (CVSPREFIX) + 10);

	/* move the new file out of the way. */
	(void) sprintf (fname, "%s/%s%s", CVSADM, CVSPREFIX, file);
	rename_file (file, fname);
	copy_file (DEVNULL, file);

	tmp = xmalloc (strlen (file) + strlen (tag) + 80);
	/* commit a dead revision. */
	(void) sprintf (tmp, "file %s was initially added on branch %s.",
			       RCS_FLAGS_DEAD | RCS_FLAGS_QUIET);
	retcode = RCS_checkin (rcs, NULL, tmp, NULL,
			       RCS_FLAGS_DEAD | RCS_FLAGS_QUIET, 0);
	free (tmp);
	if (retcode != 0)
	{
	    retval = 1;
	    goto out;
		   "could not create initial dead revision %s", rcs);
	    return (1);
	}

	free (fname);

	assert (rcsfile == NULL);
	rcsfile = RCS_parse (file, repository);
	if (rcsfile == NULL)
	{
	    error (0, 0, "could not read %s", rcs);
	    retval = 1;
	    goto out;
	}
	if (rcsnode != NULL)
	{
	    assert (*rcsnode == NULL);
	    *rcsnode = rcsfile;
	}

	rename_file (fname, file);
	if (lock_RCS (file, rcsfile, NULL, repository))
	{
	/* and lock it once again. */
	    retval = 1;
	    goto out;
	    error (0, 0, "cannot lock `%s'.", rcs);
	    return (1);
	}
    }

    if (tag != NULL)
    {
	   doesn't already exist.  */

	rcsfile = RCS_parse (file, repository);
	    if (rcsnode != NULL && *rcsnode != NULL)
		rcsfile = *rcsnode;
	    else
	    {
		rcsfile = RCS_parse (file, repository);
		if (rcsfile == NULL)
		{
		    error (0, 0, "could not read %s", rcs);
		    retval = 1;
		    goto out;
		}
	    }
	    error (0, 0, "could not read %s", rcs);

	if (!RCS_nodeisbranch (rcsfile, tag))
	{
	
	if (!RCS_nodeisbranch (rcsfile, tag)) {
	    /* branch does not exist.  Stub it.  */

	    head = RCS_getversion (rcsfile, NULL, NULL, 0, (int *) NULL);
	    

	    retcode = RCS_settag (rcsfile, tag, magicrev);

	    free (head);
	    free (magicrev);

	    if (retcode != 0)
	    magicrev = RCS_magicrev (rcsfile, head);
	    if ((retcode = RCS_settag(rcs, tag, magicrev)) != 0)
	    {
		retval = 1;
		goto out;
		       "could not stub branch %s for %s", tag, rcs);
	    free (head);
	    free (magicrev);
	}
	else
	    if (lock_RCS (file, rcsfile, NULL, repository))
	    {
	    /* lock the branch. (stubbed branches need not be locked.)  */
		retval = 1;
		goto out;
		error (0, 0, "cannot lock `%s'.", rcs);
		return (1);
	    }
	if (rcsnode && *rcsnode != rcsfile)
	{
	    freercsnode(rcsnode);
	    *rcsnode = rcsfile;
	}
		freercsnode(rcsnode);
	*rcsnode = rcsfile;
    }

    fileattr_newfile (file);
    retval = 0;

 out:
    free (rcs);
    return retval;
    fix_rcs_modes (rcs, file);
    return (0);
}

/*
 * Attempt to place a lock on the RCS file; returns 0 if it could and 1 if it
 * couldn't.  If the RCS file currently has a branch as the head, we must
 * move the head back to the trunk before locking the file, and be sure to
 * put the branch back as the head if there are any errors.
 */
static int
    RCSNode *rcs;
    char *user;
    char *rcs;
    char *rev;
{
    RCSNode *rcsfile;
    char *branch = NULL;
    int err = 0;

    /*
     * For a specified, numeric revision of the form "1" or "1.1", (or when
     * no revision is specified ""), definitely move the branch to the trunk
     * before locking the RCS file.
     * 
     * The assumption is that if there is more than one revision on the trunk,
     * the head points to the trunk, not a branch... and as such, it's not
     * necessary to move the head in this case.
     */
	branch = xstrdup (rcs->branch);
	if (branch != NULL)
	}
	    if (RCS_setbranch (rcs, NULL) != 0)
	    freercsnode (&rcsfile);
		error (0, 0, "cannot change branch to default for %s",
		       rcs->path);
		if (branch)
		    free (branch);
		return (1);
		    return (1);
	    }
	err = RCS_lock(rcs, NULL, 0);
	    err = RCS_lock(rcs, NULL, 0);
	}
    }
    else
    {
	(void) RCS_lock(rcs, rev, 1);
    }

	if (sbranch != NULL)
	    free (sbranch);
    if (err == 0)
    {
	    sbranch = branch;
	    (void) strcpy (sbranch, branch);
	    free (branch);
	    sbranch = NULL;
	else
	    sbranch[0] = '\0';
	return (0);
    }

	fixbranch (rcs, branch);
    if (branch != NULL)
	fixbranch (user, repository, branch);

    if (branch)
	free (branch);
    return (1);
}

/*
 * Called when "add"ing files to the RCS respository, as it is necessary to
 * preserve the file modes in the same fashion that RCS does.  This would be
 * automatic except that we are placing the RCS ,v file very far away from
 * the user file, and I can't seem to convince RCS of the location of the
 * user file.  So we munge it here, after the ,v file has been successfully
 * initialized with "rcs -i".
 */
static void
fix_rcs_modes (rcs, user)
    char *rcs;
    char *user;
{
    if ( CVS_STAT (user, &sb) != -1)

    if (stat (user, &sb) != -1)
	(void) chmod (rcs, (int) sb.st_mode & ~0222);
}
 * free an UPDATE node's data
/*
 * free an UPDATE node's data (really nothing to do)
 */
void
update_delproc (p)
    struct logfile_info *li;

    li = (struct logfile_info *) p->data;
    if (li->tag)
	free (li->tag);
    if (li->rev_old)
	free (li->rev_old);
    if (li->rev_new)
	free (li->rev_new);
    free (li);
{
    p->data = (char *) NULL;
}

/*
 * Free the commit_info structure in p.
 */
static void
ci_delproc (p)
    Node *p;
{
    struct commit_info *ci;

    ci = (struct commit_info *) p->data;
    if (ci->rev)
	free (ci->rev);
    if (ci->tag)
	free (ci->tag);
    if (ci->options)
	free (ci->options);
    free (ci);
}

/*
 * Free the commit_info structure in p.
 */
static void
masterlist_delproc (p)
    Node *p;
{
    struct master_lists *ml;

    ml = (struct master_lists *) p->data;
    dellist (&ml->ulist);
    dellist (&ml->cilist);
    free (ml);
/* Find an RCS file in the repository.  Most parts of CVS will want to
   rely instead on RCS_parse which performs a similar operation and is
   called by recurse.c which then puts the result in useful places
   like the rcs field of struct file_info.

   REPOSITORY is the repository (including the directory) and FILE is
   the filename within that directory (without RCSEXT).  Returns a
   newly-malloc'd array containing the absolute pathname of the RCS
   file that was found.  */
static char *
locate_rcs (file, repository)
static void
locate_rcs (file, repository, rcs)
    char *repository;
    char *rcs;

    rcs = xmalloc (strlen (repository) + strlen (file) + sizeof (RCSEXT) + 10);
    char *rcs;
{
    (void) sprintf (rcs, "%s/%s%s", repository, file, RCSEXT);
    if (!isreadable (rcs))
    {
	(void) sprintf (rcs, "%s/%s/%s%s", repository, CVSATTIC, file, RCSEXT);
	if (!isreadable (rcs))
    return rcs;
	    (void) sprintf (rcs, "%s/%s%s", repository, file, RCSEXT);
    }
}

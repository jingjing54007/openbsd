/*	$OpenBSD: src/gnu/usr.bin/gas/opcode/Attic/tahoe.h,v 1.2 1998/02/15 18:50:09 niklas Exp $	*/

/* tahoe-opcode.h - tahoe-specific
 * Not part of GAS yet
 *
 * Ported by the State University of New York at Buffalo by the Distributed
 * Computer Systems Lab, Department of Computer Science, 1991. (And by Dale
 * Wiles who was unemployed at the time.)
 */

struct tot_wot                  /* tahoe opcode table: wot to do with this */
                                /* particular opcode */
{
  char *args;			/* how to compile said opcode */
  tahoe_opcodeT code;		/* The opcode. */
};

struct tot                      /* tahoe opcode text */
{
  char *name;			/* opcode name: lowercase string  [key]  */
  struct tot_wot detail;	/* rest of opcode table          [datum] */
};

static struct tot
totstrs[] =
{
{    "halt",	{"",			0x00	} },
{    "sinf",    {"",                    0x05    } },
{    "ldf",     {"rl",                  0x06    } },
{    "ldd",     {"rq",                  0x07    } },
{    "addb2",	{"rbmb",		0x08	} },
{    "movb",	{"rbwb",		0x09	} },
{    "addw2",	{"rwmw",		0x0a	} },
{    "movw",	{"rwww",		0x0b	} },
{    "addl2",	{"rlml",		0x0c	} },
{    "movl",	{"rlwl",		0x0d	} },
{    "bbs",	{"rlvlbw",		0x0e	} },
{    "nop",     {"",                    0x10    } },
{    "brb",	{"bb",			0x11	} },
{    "brw",	{"bw",			0x13	} },
{    "cosf",    {"",                    0x15    } },
{    "lnf",     {"rl",                  0x16    } },
{    "lnd",     {"rq",                  0x17    } },
{    "addb3",	{"rbrbwb",		0x18	} },
  /* cmpb is wrong in the "offical" (what a joke!) text. It's not "rbwb" */
{    "cmpb",	{"rbrb",		0x19	} }, 
{    "addw3",	{"rwrwww",		0x1a	} },
  /* cmpw is wrong in the "offical" text. It's not "rwww" */
{    "cmpw",	{"rwrw",		0x1b	} },
{    "addl3",	{"rlrlwl",		0x1c	} },
  /* cmpl is wrong in the "offical" text. It's not "rlwl" */
{    "cmpl",	{"rlrl",		0x1d	} },
{    "bbc",	{"rlvlbw",		0x1e	} },
{    "rei",	{"",			0x20	} },
{    "bneq",	{"bb",			0x21	} },
{    "bnequ",	{"bb",			0x21	} },
{    "cvtwl",	{"rwwl",		0x23	} },
{    "stf",     {"wl",                  0x26    } },
{    "std",     {"wq",                  0x27    } },
{    "subb2",	{"rbmb",		0x28	} },
{    "mcomb",	{"rbwb",		0x29	} },
{    "subw2",	{"rwmw",		0x2a	} },
{    "mcomw",	{"rwww",		0x2b	} },
{    "subl2",   {"rlml",                0x2c    } },
{    "mcoml",   {"rlwl",                0x2d    } },
{    "emul",	{"rlrlrlwq",		0x2e	} },
{    "aoblss",	{"rlmlbw",		0x2f	} },
{    "bpt",	{"",			0x30	} },
{    "beql",	{"bb",			0x31	} },
{    "beqlu",	{"bb",			0x31	} },
{    "cvtwb",	{"rwwb",		0x33	} },
{    "logf",    {"",                    0x35    } },
{    "cmpf",    {"rl",                  0x36    } },
{    "cmpd",    {"rq",                  0x37    } },
{    "subb3",	{"rbrbwb",		0x38	} },
{    "bitb",	{"rbrb",		0x39	} },
{    "subw3",	{"rwrwww",		0x3a	} },
{    "bitw",	{"rwrw",		0x3b	} },
{    "subl3",	{"rlrlwl",		0x3c	} },
{    "bitl",	{"rlrl",		0x3d	} },
{    "ediv",	{"rlrqwlwl",		0x3e	} },
{    "aobleq",	{"rlmlbw",		0x3f	} },
{    "ret",	{"",			0x40	} },
{    "bgtr",	{"bb",			0x41	} },
{    "sqrtf",   {"",                    0x45    } },
{    "cmpf2",   {"rlrl",                0x46    } },
{    "cmpd2",   {"rqrq",                0x47    } },
{    "shll",    {"rbrlwl",              0x48    } },
{    "clrb",	{"wb",			0x49	} },
{    "shlq",	{"rbrqwq",		0x4a	} },
{    "clrw",	{"ww",			0x4b	} },
{    "mull2",	{"rlml",		0x4c	} },
{    "clrl",	{"wl",			0x4d	} },
{    "shal",    {"rbrlwl",		0x4e	} },
{    "bleq",	{"bb",			0x51	} },
{    "expf",    {"",                    0x55    } },
{    "tstf",    {"",                    0x56    } },
{    "tstd",    {"",                    0x57    } },
{    "shrl",    {"rbrlwl",		0x58	} },
{    "tstb",	{"rb",			0x59	} },
{    "shrq",    {"rbrqwq",		0x5a	} },
{    "tstw",	{"rw",			0x5b	} },
{    "mull3",	{"rlrlwl",		0x5c	} },
{    "tstl",	{"rl",			0x5d	} },
{    "shar",	{"rbrlwl",		0x5e	} },
{    "bbssi",	{"rlmlbw",		0x5f	} },
{    "ldpctx",	{"",			0x60	} },
{    "pushd",   {"",                    0x67    } },
{    "incb",	{"mb",			0x69	} },
{    "incw",	{"mw",			0x6b	} },
{    "divl2",	{"rlml",		0x6c	} },
{    "incl",	{"ml",			0x6d	} },
{    "cvtlb",	{"rlwb",		0x6f	} },
{    "svpctx",	{"",			0x70	} },
{    "jmp",	{"ab",			0x71	} },
{    "cvlf",    {"rl",                  0x76    } },
{    "cvld",    {"rl",                  0x77    } },
{    "decb",	{"mb",			0x79	} },
{    "decw",	{"mw",			0x7b	} },
{    "divl3",	{"rlrlwl",		0x7c	} },
{    "decl",	{"ml",			0x7d	} },
{    "cvtlw",	{"rlww",		0x7f	} },
{    "bgeq",	{"bb",			0x81	} },
{    "movs2",	{"abab",		0x82	} },
{    "cvfl",    {"wl",                  0x86    } },
{    "cvdl",    {"wl",                  0x87    } },
{    "orb2",	{"rbmb",		0x88	} },
{    "cvtbl",	{"rbwl",		0x89	} },
{    "orw2",	{"rwmw",		0x8a	} },
{    "bispsw",	{"rw",			0x8b	} },
{    "orl2",    {"rlml",                0x8c    } },
{    "adwc",	{"rlml",		0x8d	} },
{    "adda", 	{"rlml",		0x8e	} },
{    "blss",	{"bb",			0x91	} },
{    "cmps2",   {"abab",		0x92	} },
{    "ldfd",    {"rl",                  0x97    } },
{    "orb3",	{"rbrbwb",		0x98	} },
{    "cvtbw",	{"rbww",		0x99	} },
{    "orw3",   	{"rwrwww",		0x9a	} },
{    "bicpsw",	{"rw",			0x9b	} },
{    "orl3",    {"rlrlwl",              0x9c    } },
{    "sbwc",	{"rlml",		0x9d	} },
{    "suba",    {"rlml",                0x9e    } },
{    "bgtru",	{"bb",			0xa1	} },
{    "cvdf",    {"",                    0xa6    } },
{    "andb2",   {"rbmb",		0xa8    } },
{    "movzbl",	{"rbwl",		0xa9	} },
{    "andw2",   {"rwmw",		0xaa    } },
{    "loadr",   {"rwal",		0xab	} },
{    "andl2",   {"rlml",		0xac    } },
{    "mtpr",	{"rlrl",		0xad	} },
{    "ffs",	{"rlwl",		0xae	} },
{    "blequ",	{"bb",			0xb1	} },
{    "negf",    {"",                    0xb6    } },
{    "negd",    {"",                    0xb7    } },
{    "andb3",   {"rbrbwb",              0xb8    } },
{    "movzbw",	{"rbww",		0xb9	} },
{    "andw3",   {"rwrwww",		0xba    } },
{    "storer",  {"rwal",                0xbb    } },
{    "andl3",   {"rlrlwl",		0xbc    } },
{    "mfpr",	{"rlwl",		0xbd	} },
{    "ffc",	{"rlwl",		0xbe	} },
{    "calls",	{"rbab",		0xbf	} },
{    "prober",	{"rbabrl",		0xc0	} },
{    "bvc",	{"bb",			0xc1	} },
{    "movs3",	{"ababrw",		0xc2	} },
{    "movzwl",	{"rwwl",		0xc3	} },
{    "addf",    {"rl",                  0xc6    } },
{    "addd",    {"rq",                  0xc7    } },
{    "xorb2",   {"rbmb",                0xc8    } },
{    "movob",   {"rbwb",                0xc9    } },
{    "xorw2",   {"rwmw",                0xca    } },
{    "movow",   {"rwww",                0xcb	} },
{    "xorl2",	{"rlml",                0xcc    } },
{    "movpsl",  {"wl",                  0xcd    } },
{    "kcall",   {"rw",			0xcf	} },
{    "probew",  {"rbabrl",		0xd0	} },
{    "bvs",     {"bb",			0xd1	} },
{    "cmps3",   {"ababrw",		0xd2	} },
{    "subf",    {"rq",                  0xd6    } },
{    "subd",    {"rq",                  0xd7    } },
{    "xorb3",	{"rbrbwb",		0xd8	} },
{    "pushb",   {"rb",			0xd9	} },
{    "xorw3",	{"rwrwww",		0xda	} },
{    "pushw",   {"rw", 			0xdb	} },
{    "xorl3",	{"rlrlwl",		0xdc	} },
{    "pushl",	{"rl",			0xdd	} },
{    "insque",	{"abab",		0xe0	} },
{    "bcs",	{"bb",			0xe1	} },
{    "bgequ",	{"bb",			0xe1	} },
{    "mulf",    {"rq",                  0xe6    } },
{    "muld",    {"rq",                  0xe7    } },
{    "mnegb",	{"rbwb",		0xe8	} },
{    "movab",	{"abwl",		0xe9	} },
{    "mnegw",	{"rwww",		0xea	} },
{    "movaw",	{"awwl",		0xeb	} },
{    "mnegl",	{"rlwl",		0xec	} },
{    "moval",	{"alwl",		0xed	} },
{    "remque",	{"ab",  		0xf0	} },
{    "bcc",	{"bb",			0xf1	} },
{    "blssu",	{"bb",			0xf1	} },
{    "divf",    {"rq",                  0xf6    } },
{    "divd",    {"rq",                  0xf7    } },
  /* movblk is really "alalrw" but 'as' won't accept it,
     'cc' and 'gcc' also produce code this way. */
{    "movblk",  {"",                    0xf8	} },
{    "pushab",	{"ab",			0xf9	} },
{    "pushaw",	{"aw",			0xfb	} },
{    "casel",	{"rlrlrl",		0xfc	} },
{    "pushal",	{"al",			0xfd	} },
{    "callf",	{"rbab",		0xfe	} },
{      "",   ""          } /* empty is end sentinel */
};

/* These are synthetic instructions, where the assembler will munge
   the addressings modes for you. */
static struct tot
synthetic_totstrs[] =
{
{    "jr",	{"b-",			0x11	} },
{    "jbr",	{"b-",			0x11	} },

{    "jneq",	{"b?",			0x21	} },
{    "jnequ",	{"b?",			0x21	} },
{    "jeql",	{"b?",			0x31	} },
{    "jeqlu",	{"b?",			0x31	} },
{    "jgtr",	{"b?",			0x41	} },
{    "jleq",	{"b?",			0x51	} },
{    "jgeq",	{"b?",			0x81	} },
{    "jlss",	{"b?",			0x91	} },
{    "jgtru",	{"b?",			0xa1	} },
{    "jlequ",	{"b?",			0xb1	} },
{    "jvc",	{"b?",			0xc1	} },
{    "jvs",     {"b?",			0xd1	} },
{    "jcs",	{"b?",			0xe1	} },
{    "jgequ",	{"b?",			0xe1	} },
{    "jcc",	{"b?",			0xf1	} },
{    "jlssu",	{"b?",			0xf1	} },

{    "jbs",	{"rlvlb!",		0x0e	} },
{    "jbc",	{"rlvlb!",		0x1e	} },

{    "aojlss",	{"rlmlb:",		0x2f	} },
{    "jaoblss",	{"rlmlb:",		0x2f	} },
{    "aojleq",	{"rlmlb:",		0x3f	} },
{    "jaobleq",	{"rlmlb:",		0x3f	} },
{    "jbssi",	{"rlmlb:",		0x5f	} },
  {      "",   ""          } /* empty is end sentinel */
};

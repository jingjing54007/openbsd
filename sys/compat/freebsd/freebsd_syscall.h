/*
 * System call numbers.
 *
 * DO NOT EDIT-- this file is automatically generated.
 * created from	OpenBSD: syscalls.master,v 1.8 1997/11/13 18:35:23 deraadt Exp 
 */

#define	FREEBSD_SYS_syscall	0
#define	FREEBSD_SYS_exit	1
#define	FREEBSD_SYS_fork	2
#define	FREEBSD_SYS_read	3
#define	FREEBSD_SYS_write	4
#define	FREEBSD_SYS_open	5
#define	FREEBSD_SYS_close	6
#define	FREEBSD_SYS_wait4	7
#define	FREEBSD_SYS_ocreat	8
#define	FREEBSD_SYS_link	9
#define	FREEBSD_SYS_unlink	10
				/* 11 is obsolete execv */
#define	FREEBSD_SYS_chdir	12
#define	FREEBSD_SYS_fchdir	13
#define	FREEBSD_SYS_mknod	14
#define	FREEBSD_SYS_chmod	15
#define	FREEBSD_SYS_chown	16
#define	FREEBSD_SYS_break	17
#define	FREEBSD_SYS_getfsstat	18
#define	FREEBSD_SYS_olseek	19
#define	FREEBSD_SYS_getpid	20
#define	FREEBSD_SYS_mount	21
#define	FREEBSD_SYS_unmount	22
#define	FREEBSD_SYS_setuid	23
#define	FREEBSD_SYS_getuid	24
#define	FREEBSD_SYS_geteuid	25
#define	FREEBSD_SYS_ptrace	26
#define	FREEBSD_SYS_recvmsg	27
#define	FREEBSD_SYS_sendmsg	28
#define	FREEBSD_SYS_recvfrom	29
#define	FREEBSD_SYS_accept	30
#define	FREEBSD_SYS_getpeername	31
#define	FREEBSD_SYS_getsockname	32
#define	FREEBSD_SYS_access	33
#define	FREEBSD_SYS_chflags	34
#define	FREEBSD_SYS_fchflags	35
#define	FREEBSD_SYS_sync	36
#define	FREEBSD_SYS_kill	37
#define	FREEBSD_SYS_ostat	38
#define	FREEBSD_SYS_getppid	39
#define	FREEBSD_SYS_olstat	40
#define	FREEBSD_SYS_dup	41
#define	FREEBSD_SYS_pipe	42
#define	FREEBSD_SYS_getegid	43
#define	FREEBSD_SYS_profil	44
#define	FREEBSD_SYS_ktrace	45
#define	FREEBSD_SYS_sigaction	46
#define	FREEBSD_SYS_getgid	47
#define	FREEBSD_SYS_sigprocmask	48
#define	FREEBSD_SYS_getlogin	49
#define	FREEBSD_SYS_setlogin	50
#define	FREEBSD_SYS_acct	51
#define	FREEBSD_SYS_sigpending	52
#define	FREEBSD_SYS_sigaltstack	53
#define	FREEBSD_SYS_ioctl	54
#define	FREEBSD_SYS_reboot	55
#define	FREEBSD_SYS_revoke	56
#define	FREEBSD_SYS_symlink	57
#define	FREEBSD_SYS_readlink	58
#define	FREEBSD_SYS_execve	59
#define	FREEBSD_SYS_umask	60
#define	FREEBSD_SYS_chroot	61
#define	FREEBSD_SYS_ofstat	62
#define	FREEBSD_SYS_ogetkerninfo	63
#define	FREEBSD_SYS_ogetpagesize	64
#define	FREEBSD_SYS_msync	65
#define	FREEBSD_SYS_vfork	66
				/* 67 is obsolete vread */
				/* 68 is obsolete vwrite */
#define	FREEBSD_SYS_sbrk	69
#define	FREEBSD_SYS_sstk	70
#define	FREEBSD_SYS_ommap	71
#define	FREEBSD_SYS_vadvise	72
#define	FREEBSD_SYS_munmap	73
#define	FREEBSD_SYS_mprotect	74
#define	FREEBSD_SYS_madvise	75
				/* 76 is obsolete vhangup */
				/* 77 is obsolete vlimit */
#define	FREEBSD_SYS_mincore	78
#define	FREEBSD_SYS_getgroups	79
#define	FREEBSD_SYS_setgroups	80
#define	FREEBSD_SYS_getpgrp	81
#define	FREEBSD_SYS_setpgid	82
#define	FREEBSD_SYS_setitimer	83
#define	FREEBSD_SYS_owait	84
#define	FREEBSD_SYS_swapon	85
#define	FREEBSD_SYS_getitimer	86
#define	FREEBSD_SYS_ogethostname	87
#define	FREEBSD_SYS_osethostname	88
#define	FREEBSD_SYS_ogetdtablesize	89
#define	FREEBSD_SYS_dup2	90
#define	FREEBSD_SYS_fcntl	92
#define	FREEBSD_SYS_select	93
#define	FREEBSD_SYS_fsync	95
#define	FREEBSD_SYS_setpriority	96
#define	FREEBSD_SYS_socket	97
#define	FREEBSD_SYS_connect	98
#define	FREEBSD_SYS_oaccept	99
#define	FREEBSD_SYS_getpriority	100
#define	FREEBSD_SYS_osend	101
#define	FREEBSD_SYS_orecv	102
#define	FREEBSD_SYS_sigreturn	103
#define	FREEBSD_SYS_bind	104
#define	FREEBSD_SYS_setsockopt	105
#define	FREEBSD_SYS_listen	106
				/* 107 is obsolete vtimes */
#define	FREEBSD_SYS_osigvec	108
#define	FREEBSD_SYS_osigblock	109
#define	FREEBSD_SYS_osigsetmask	110
#define	FREEBSD_SYS_sigsuspend	111
#define	FREEBSD_SYS_osigstack	112
#define	FREEBSD_SYS_orecvmsg	113
#define	FREEBSD_SYS_osendmsg	114
#define	FREEBSD_SYS_vtrace	115
				/* 115 is obsolete vtrace */
#define	FREEBSD_SYS_gettimeofday	116
#define	FREEBSD_SYS_getrusage	117
#define	FREEBSD_SYS_getsockopt	118
				/* 119 is obsolete resuba */
#define	FREEBSD_SYS_readv	120
#define	FREEBSD_SYS_writev	121
#define	FREEBSD_SYS_settimeofday	122
#define	FREEBSD_SYS_fchown	123
#define	FREEBSD_SYS_fchmod	124
#define	FREEBSD_SYS_orecvfrom	125
#define	FREEBSD_SYS_osetreuid	126
#define	FREEBSD_SYS_osetregid	127
#define	FREEBSD_SYS_rename	128
#define	FREEBSD_SYS_otruncate	129
#define	FREEBSD_SYS_oftruncate	130
#define	FREEBSD_SYS_flock	131
#define	FREEBSD_SYS_mkfifo	132
#define	FREEBSD_SYS_sendto	133
#define	FREEBSD_SYS_shutdown	134
#define	FREEBSD_SYS_socketpair	135
#define	FREEBSD_SYS_mkdir	136
#define	FREEBSD_SYS_rmdir	137
#define	FREEBSD_SYS_utimes	138
				/* 139 is obsolete 4.2 sigreturn */
#define	FREEBSD_SYS_adjtime	140
#define	FREEBSD_SYS_ogetpeername	141
#define	FREEBSD_SYS_ogethostid	142
#define	FREEBSD_SYS_osethostid	143
#define	FREEBSD_SYS_ogetrlimit	144
#define	FREEBSD_SYS_osetrlimit	145
#define	FREEBSD_SYS_okillpg	146
#define	FREEBSD_SYS_setsid	147
#define	FREEBSD_SYS_quotactl	148
#define	FREEBSD_SYS_oquota	149
#define	FREEBSD_SYS_ogetsockname	150
#define	FREEBSD_SYS_nfssvc	155
#define	FREEBSD_SYS_ogetdirentries	156
#define	FREEBSD_SYS_statfs	157
#define	FREEBSD_SYS_fstatfs	158
#define	FREEBSD_SYS_getfh	161
#define	FREEBSD_SYS_getdomainname	162
#define	FREEBSD_SYS_setdomainname	163
#define	FREEBSD_SYS_uname	164
#define	FREEBSD_SYS_sysarch	165
#define	FREEBSD_SYS_rtprio	166
#define	FREEBSD_SYS_semsys	169
#define	FREEBSD_SYS_msgsys	170
#define	FREEBSD_SYS_shmsys	171
#define	FREEBSD_SYS_freebsd_ntp_adjtime	176
#define	FREEBSD_SYS_setgid	181
#define	FREEBSD_SYS_setegid	182
#define	FREEBSD_SYS_seteuid	183
#define	FREEBSD_SYS_lfs_bmapv	184
#define	FREEBSD_SYS_lfs_markv	185
#define	FREEBSD_SYS_lfs_segclean	186
#define	FREEBSD_SYS_lfs_segwait	187
#define	FREEBSD_SYS_stat	188
#define	FREEBSD_SYS_fstat	189
#define	FREEBSD_SYS_lstat	190
#define	FREEBSD_SYS_pathconf	191
#define	FREEBSD_SYS_fpathconf	192
#define	FREEBSD_SYS_getrlimit	194
#define	FREEBSD_SYS_setrlimit	195
#define	FREEBSD_SYS_getdirentries	196
#define	FREEBSD_SYS_mmap	197
#define	FREEBSD_SYS___syscall	198
#define	FREEBSD_SYS_lseek	199
#define	FREEBSD_SYS_truncate	200
#define	FREEBSD_SYS_ftruncate	201
#define	FREEBSD_SYS___sysctl	202
#define	FREEBSD_SYS_mlock	203
#define	FREEBSD_SYS_munlock	204
#define	FREEBSD_SYS_undelete	205
#define	FREEBSD_SYS_getpgid	207
#define	FREEBSD_SYS_poll	209
#define	FREEBSD_SYS___semctl	220
#define	FREEBSD_SYS_semget	221
#define	FREEBSD_SYS_semop	222
#define	FREEBSD_SYS_semconfig	223
#define	FREEBSD_SYS_msgctl	224
#define	FREEBSD_SYS_msgget	225
#define	FREEBSD_SYS_msgsnd	226
#define	FREEBSD_SYS_msgrcv	227
#define	FREEBSD_SYS_shmat	228
#define	FREEBSD_SYS_shmctl	229
#define	FREEBSD_SYS_shmdt	230
#define	FREEBSD_SYS_shmget	231
#define	FREEBSD_SYS_nanosleep	240
#define	FREEBSD_SYS_minherit	250
#define	FREEBSD_SYS_rfork	251
#define	FREEBSD_SYS_poll2	252
#define	FREEBSD_SYS_issetugid	253
#define	FREEBSD_SYS_lchown	254
#define	FREEBSD_SYS_MAXSYSCALL	255

/* Run the C pre-processor over this file with one of the following defined
 * ELF - elf object files,
 * OUT - a.out object files,
 * BSDI - BSDI style a.out object files
 * SOL - Solaris style elf
 */

#define TYPE(a,b)       .type   a,b
#define SIZE(a,b)       .size   a,b

#if defined(OUT) || defined(BSDI)
#define des_SPtrans _des_SPtrans
#define fcrypt_body _fcrypt_body

#endif

#ifdef OUT
#define OK	1
#define ALIGN	4
#endif

#ifdef BSDI
#define OK              1
#define ALIGN           4
#undef SIZE
#undef TYPE
#define SIZE(a,b)
#define TYPE(a,b)
#endif

#if defined(ELF) || defined(SOL)
#define OK              1
#define ALIGN           16
#endif

#ifndef OK
You need to define one of
ELF - elf systems - linux-elf, NetBSD and DG-UX
OUT - a.out systems - linux-a.out and FreeBSD
SOL - solaris systems, which are elf with strange comment lines
BSDI - a.out with a very primative version of as.
#endif

/* Let the Assembler begin :-) */
	/* Don't even think of reading this code */
	/* It was automatically generated by crypt586.pl */
	/* Which is a perl program used to generate the x86 assember for */
	/* any of elf, a.out, BSDI,Win32, or Solaris */
	/* eric <eay@cryptsoft.com> */

	.file	"crypt586.s"
	.version	"01.01"
gcc2_compiled.:
.text
	.align ALIGN
.globl fcrypt_body
	TYPE(fcrypt_body,@function)
fcrypt_body:
	pushl	%ebp
	pushl	%ebx
	pushl	%esi
	pushl	%edi


	/* Load the 2 words */
	xorl	%edi,		%edi
	xorl	%esi,		%esi
	movl	24(%esp),	%ebp
	pushl	$25
.L000start:

	/* Round 0 */
	movl	32(%esp),	%eax
	movl	%esi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%esi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	(%ebp),		%ebx
	xorl	%ebx,		%eax
	movl	4(%ebp),	%ecx
	xorl	%esi,		%eax
	xorl	%esi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%edi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%edi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%edi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%edi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%edi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%edi

	/* Round 1 */
	movl	32(%esp),	%eax
	movl	%edi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%edi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	8(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	12(%ebp),	%ecx
	xorl	%edi,		%eax
	xorl	%edi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%esi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%esi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%esi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%esi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%esi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%esi

	/* Round 2 */
	movl	32(%esp),	%eax
	movl	%esi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%esi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	16(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	20(%ebp),	%ecx
	xorl	%esi,		%eax
	xorl	%esi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%edi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%edi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%edi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%edi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%edi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%edi

	/* Round 3 */
	movl	32(%esp),	%eax
	movl	%edi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%edi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	24(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	28(%ebp),	%ecx
	xorl	%edi,		%eax
	xorl	%edi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%esi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%esi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%esi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%esi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%esi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%esi

	/* Round 4 */
	movl	32(%esp),	%eax
	movl	%esi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%esi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	32(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	36(%ebp),	%ecx
	xorl	%esi,		%eax
	xorl	%esi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%edi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%edi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%edi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%edi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%edi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%edi

	/* Round 5 */
	movl	32(%esp),	%eax
	movl	%edi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%edi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	40(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	44(%ebp),	%ecx
	xorl	%edi,		%eax
	xorl	%edi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%esi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%esi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%esi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%esi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%esi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%esi

	/* Round 6 */
	movl	32(%esp),	%eax
	movl	%esi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%esi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	48(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	52(%ebp),	%ecx
	xorl	%esi,		%eax
	xorl	%esi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%edi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%edi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%edi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%edi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%edi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%edi

	/* Round 7 */
	movl	32(%esp),	%eax
	movl	%edi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%edi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	56(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	60(%ebp),	%ecx
	xorl	%edi,		%eax
	xorl	%edi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%esi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%esi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%esi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%esi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%esi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%esi

	/* Round 8 */
	movl	32(%esp),	%eax
	movl	%esi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%esi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	64(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	68(%ebp),	%ecx
	xorl	%esi,		%eax
	xorl	%esi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%edi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%edi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%edi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%edi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%edi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%edi

	/* Round 9 */
	movl	32(%esp),	%eax
	movl	%edi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%edi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	72(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	76(%ebp),	%ecx
	xorl	%edi,		%eax
	xorl	%edi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%esi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%esi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%esi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%esi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%esi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%esi

	/* Round 10 */
	movl	32(%esp),	%eax
	movl	%esi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%esi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	80(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	84(%ebp),	%ecx
	xorl	%esi,		%eax
	xorl	%esi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%edi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%edi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%edi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%edi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%edi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%edi

	/* Round 11 */
	movl	32(%esp),	%eax
	movl	%edi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%edi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	88(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	92(%ebp),	%ecx
	xorl	%edi,		%eax
	xorl	%edi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%esi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%esi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%esi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%esi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%esi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%esi

	/* Round 12 */
	movl	32(%esp),	%eax
	movl	%esi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%esi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	96(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	100(%ebp),	%ecx
	xorl	%esi,		%eax
	xorl	%esi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%edi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%edi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%edi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%edi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%edi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%edi

	/* Round 13 */
	movl	32(%esp),	%eax
	movl	%edi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%edi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	104(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	108(%ebp),	%ecx
	xorl	%edi,		%eax
	xorl	%edi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%esi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%esi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%esi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%esi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%esi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%esi

	/* Round 14 */
	movl	32(%esp),	%eax
	movl	%esi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%esi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	112(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	116(%ebp),	%ecx
	xorl	%esi,		%eax
	xorl	%esi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%edi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%edi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%edi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%edi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%edi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%edi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%edi

	/* Round 15 */
	movl	32(%esp),	%eax
	movl	%edi,		%edx
	shrl	$16,		%edx
	movl	36(%esp),	%ecx
	xorl	%edi,		%edx
	andl	%edx,		%eax
	andl	%ecx,		%edx
	movl	%eax,		%ebx
	sall	$16,		%ebx
	movl	%edx,		%ecx
	sall	$16,		%ecx
	xorl	%ebx,		%eax
	xorl	%ecx,		%edx
	movl	120(%ebp),	%ebx
	xorl	%ebx,		%eax
	movl	124(%ebp),	%ecx
	xorl	%edi,		%eax
	xorl	%edi,		%edx
	xorl	%ecx,		%edx
	andl	$0xfcfcfcfc,	%eax
	xorl	%ebx,		%ebx
	andl	$0xcfcfcfcf,	%edx
	xorl	%ecx,		%ecx
	movb	%al,		%bl
	movb	%ah,		%cl
	rorl	$4,		%edx
	movl	      des_SPtrans(%ebx),%ebp
	movb	%dl,		%bl
	xorl	%ebp,		%esi
	movl	0x200+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movb	%dh,		%cl
	shrl	$16,		%eax
	movl	0x100+des_SPtrans(%ebx),%ebp
	xorl	%ebp,		%esi
	movb	%ah,		%bl
	shrl	$16,		%edx
	movl	0x300+des_SPtrans(%ecx),%ebp
	xorl	%ebp,		%esi
	movl	28(%esp),	%ebp
	movb	%dh,		%cl
	andl	$0xff,		%eax
	andl	$0xff,		%edx
	movl	0x600+des_SPtrans(%ebx),%ebx
	xorl	%ebx,		%esi
	movl	0x700+des_SPtrans(%ecx),%ebx
	xorl	%ebx,		%esi
	movl	0x400+des_SPtrans(%eax),%ebx
	xorl	%ebx,		%esi
	movl	0x500+des_SPtrans(%edx),%ebx
	xorl	%ebx,		%esi
	movl	(%esp),		%ebx
	movl	%edi,		%eax
	decl	%ebx
	movl	%esi,		%edi
	movl	%eax,		%esi
	movl	%ebx,		(%esp)
	jnz	.L000start

	/* FP */
	movl	24(%esp),	%edx
.byte 209
.byte 207		/* rorl $1 %edi */
	movl	%esi,		%eax
	xorl	%edi,		%esi
	andl	$0xaaaaaaaa,	%esi
	xorl	%esi,		%eax
	xorl	%esi,		%edi

	roll	$23,		%eax
	movl	%eax,		%esi
	xorl	%edi,		%eax
	andl	$0x03fc03fc,	%eax
	xorl	%eax,		%esi
	xorl	%eax,		%edi

	roll	$10,		%esi
	movl	%esi,		%eax
	xorl	%edi,		%esi
	andl	$0x33333333,	%esi
	xorl	%esi,		%eax
	xorl	%esi,		%edi

	roll	$18,		%edi
	movl	%edi,		%esi
	xorl	%eax,		%edi
	andl	$0xfff0000f,	%edi
	xorl	%edi,		%esi
	xorl	%edi,		%eax

	roll	$12,		%esi
	movl	%esi,		%edi
	xorl	%eax,		%esi
	andl	$0xf0f0f0f0,	%esi
	xorl	%esi,		%edi
	xorl	%esi,		%eax

	rorl	$4,		%eax
	movl	%eax,		(%edx)
	movl	%edi,		4(%edx)
	popl	%ecx
	popl	%edi
	popl	%esi
	popl	%ebx
	popl	%ebp
	ret
.fcrypt_body_end:
	SIZE(fcrypt_body,.fcrypt_body_end-fcrypt_body)
.ident	"fcrypt_body"

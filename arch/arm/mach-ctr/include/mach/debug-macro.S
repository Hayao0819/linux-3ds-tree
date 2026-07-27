/*
 * The 3DS has no UART, so a character is one grey pixel whose brightness is its
 * ASCII code, read back from a photo.
 */

#define FB_PHYS		0x18000000
#define FB_VIRT		0xF0000000
#define FB_HEAD		6		/* the cursor word sits on the first two pixels */
#define FB_END		0x1000		/* text wraps here, so the newest log is at the left */
#define FB_SIZE_HI	0x46000		/* 400*240*3, split into two ARM immediates */
#define FB_SIZE_LO	0x500

.macro	addruart,rp,rv,tmp
	mov	\rp, #FB_PHYS
	mov	\rv, #FB_VIRT
.endm

.macro	sendpixel,rd,rx,tmp
	ldr	\tmp, [\rx]
	cmp	\tmp, #FB_END
	bhs	.Lwipe\@		@ wrapped or uninitialized
	cmp	\tmp, #FB_HEAD
	movlo	\tmp, #FB_HEAD
	b	.Lwrite\@
.Lwipe\@:
	mov	\tmp, #FB_SIZE_HI	@ wipe rather than scroll, so what is on
	add	\tmp, \tmp, #FB_SIZE_LO	@ screen is always the newest log
.Lfill\@:
	subs	\tmp, \tmp, #1
	strb	\rd, [\rx, \tmp]
	bne	.Lfill\@
	mov	\tmp, #FB_HEAD
.Lwrite\@:
	orr	\rd, \rd, #0x80	@ ASCII alone is too dark to photograph
	strb	\rd, [\rx, \tmp]
	add	\tmp, \tmp, #1
	strb	\rd, [\rx, \tmp]
	add	\tmp, \tmp, #1
	strb	\rd, [\rx, \tmp]
	add	\tmp, \tmp, #1
	bic	\rd, \rd, #0x80
	str	\tmp, [\rx]
.endm

.macro	senduart,rd,rx
	.ifc	\rd, r2
	sendpixel \rd, \rx, r1	@ the decompressor's puts() passes the character in r2
	.else
	sendpixel \rd, \rx, r2	@ printascii already hands r2 to busyuart
	.endif
.endm

.macro	busyuart,rd,rx
.endm

.macro	waituarttxrdy,rd,rx
.endm

.macro	waituartcts,rd,rx
.endm

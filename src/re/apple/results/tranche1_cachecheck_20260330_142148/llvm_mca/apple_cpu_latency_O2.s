	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 26, 0	sdk_version 26, 0
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #160
	stp	d9, d8, [sp, #48]               ; 16-byte Folded Spill
	stp	x28, x27, [sp, #64]             ; 16-byte Folded Spill
	stp	x26, x25, [sp, #80]             ; 16-byte Folded Spill
	stp	x24, x23, [sp, #96]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #112]            ; 16-byte Folded Spill
	stp	x20, x19, [sp, #128]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #144]            ; 16-byte Folded Spill
	add	x29, sp, #144
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset w25, -72
	.cfi_offset w26, -80
	.cfi_offset w27, -88
	.cfi_offset w28, -96
	.cfi_offset b8, -104
	.cfi_offset b9, -112
	mov	w19, #16960                     ; =0x4240
	movk	w19, #15, lsl #16
	cmp	w0, #2
	b.lt	LBB0_9
; %bb.1:
	mov	x20, x1
	mov	x21, x0
	mov	w25, #0                         ; =0x0
	mov	w26, #1                         ; =0x1
Lloh0:
	adrp	x22, l_.str@PAGE
Lloh1:
	add	x22, x22, l_.str@PAGEOFF
Lloh2:
	adrp	x23, l_.str.1@PAGE
Lloh3:
	add	x23, x23, l_.str.1@PAGEOFF
	b	LBB0_4
LBB0_2:                                 ;   in Loop: Header=BB0_4 Depth=1
	ldr	x0, [x20, x27, lsl #3]
	mov	x1, #0                          ; =0x0
	mov	w2, #10                         ; =0xa
	bl	_strtoull
	mov	x19, x0
LBB0_3:                                 ;   in Loop: Header=BB0_4 Depth=1
	add	w26, w27, #1
	cmp	w26, w21
	b.ge	LBB0_7
LBB0_4:                                 ; =>This Inner Loop Header: Depth=1
	sxtw	x27, w26
	ldr	x24, [x20, w26, sxtw #3]
	mov	x0, x24
	mov	x1, x22
	bl	_strcmp
	cmp	w0, #0
	add	x27, x27, #1
	ccmp	w27, w21, #0, eq
	b.lt	LBB0_2
; %bb.5:                                ;   in Loop: Header=BB0_4 Depth=1
	mov	x0, x24
	mov	x1, x23
	bl	_strcmp
	cbnz	w0, LBB0_57
; %bb.6:                                ;   in Loop: Header=BB0_4 Depth=1
	mov	w25, #1                         ; =0x1
	mov	x27, x26
	b	LBB0_3
LBB0_7:
	tbz	w25, #0, LBB0_9
; %bb.8:
Lloh4:
	adrp	x0, l_str@PAGE
Lloh5:
	add	x0, x0, l_str@PAGEOFF
	bl	_puts
	mov	w21, #1                         ; =0x1
	b	LBB0_10
LBB0_9:
	mov	w21, #0                         ; =0x0
LBB0_10:
	adrp	x23, _apple_re_now_ns.timebase@PAGE+4
	ldr	w8, [x23, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB0_12
; %bb.11:
Lloh6:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh7:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB0_12:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh8:
	adrp	x22, _apple_re_now_ns.timebase@PAGE
Lloh9:
	add	x22, x22, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w25, w26, [x22]
	mov	w24, #1                         ; =0x1
	cbz	x19, LBB0_54
; %bb.13:
	mov	x8, x19
LBB0_14:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	add	x24, x24, #1
	; InlineAsm End
	subs	x8, x8, #1
	b.ne	LBB0_14
; %bb.15:
	ldr	w8, [x23, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB0_17
LBB0_16:
Lloh10:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh11:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB0_17:
	mul	x8, x20, x25
	udiv	x20, x8, x26
	bl	_mach_absolute_time
	ldp	w8, w9, [x22]
	mul	x8, x0, x8
	udiv	x9, x8, x9
	adrp	x8, _g_u64_sink@PAGE
	str	x24, [x8, _g_u64_sink@PAGEOFF]
	sub	x8, x9, x20
	ucvtf	d0, x19
	mov	x10, #4629700416936869888       ; =0x4040000000000000
	fmov	d1, x10
	fmul	d8, d0, d1
	movi	d1, #0000000000000000
	fcmp	d8, #0.0
	movi	d0, #0000000000000000
	b.le	LBB0_21
; %bb.18:
	ucvtf	d0, x8
	fdiv	d0, d0, d8
	cmp	x9, x20
	b.eq	LBB0_22
LBB0_19:
	ucvtf	d1, x8
	fdiv	d1, d8, d1
	cbnz	w21, LBB0_23
LBB0_20:
Lloh12:
	adrp	x9, l_.str.6@PAGE
Lloh13:
	add	x9, x9, l_.str.6@PAGEOFF
	stp	d0, d1, [sp, #16]
	stp	x9, x8, [sp]
Lloh14:
	adrp	x0, l_.str.10@PAGE
Lloh15:
	add	x0, x0, l_.str.10@PAGEOFF
	b	LBB0_24
LBB0_21:
	cmp	x9, x20
	b.ne	LBB0_19
LBB0_22:
	cbz	w21, LBB0_20
LBB0_23:
	stp	d0, d1, [sp, #32]
	mov	w9, #32                         ; =0x20
Lloh16:
	adrp	x10, l_.str.6@PAGE
Lloh17:
	add	x10, x10, l_.str.6@PAGEOFF
	stp	x9, x8, [sp, #16]
	stp	x10, x19, [sp]
Lloh18:
	adrp	x0, l_.str.9@PAGE
Lloh19:
	add	x0, x0, l_.str.9@PAGEOFF
LBB0_24:
	bl	_printf
	ldr	w8, [x23, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB0_26
; %bb.25:
Lloh20:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh21:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB0_26:
	bl	_mach_absolute_time
	mov	x20, x0
	ldp	w24, w25, [x22]
	cbz	x19, LBB0_55
; %bb.27:
	fmov	d0, #1.00000000
	mov	x8, x19
	fmov	d9, #1.00000000
LBB0_28:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	fadd	d9, d9, d0
	; InlineAsm End
	subs	x8, x8, #1
	b.ne	LBB0_28
; %bb.29:
	ldr	w8, [x23, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB0_31
LBB0_30:
Lloh22:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh23:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB0_31:
	mul	x8, x20, x24
	udiv	x20, x8, x25
	bl	_mach_absolute_time
	ldp	w8, w9, [x22]
	mul	x8, x0, x8
	udiv	x9, x8, x9
	adrp	x24, _g_f64_sink@PAGE
	str	d9, [x24, _g_f64_sink@PAGEOFF]
	sub	x8, x9, x20
	movi	d1, #0000000000000000
	fcmp	d8, #0.0
	movi	d0, #0000000000000000
	b.le	LBB0_35
; %bb.32:
	ucvtf	d0, x8
	fdiv	d0, d0, d8
	cmp	x9, x20
	b.eq	LBB0_36
LBB0_33:
	ucvtf	d1, x8
	fdiv	d1, d8, d1
	cbnz	w21, LBB0_37
LBB0_34:
Lloh24:
	adrp	x9, l_.str.7@PAGE
Lloh25:
	add	x9, x9, l_.str.7@PAGEOFF
	stp	d0, d1, [sp, #16]
	stp	x9, x8, [sp]
Lloh26:
	adrp	x0, l_.str.10@PAGE
Lloh27:
	add	x0, x0, l_.str.10@PAGEOFF
	b	LBB0_38
LBB0_35:
	cmp	x9, x20
	b.ne	LBB0_33
LBB0_36:
	cbz	w21, LBB0_34
LBB0_37:
	stp	d0, d1, [sp, #32]
	mov	w9, #32                         ; =0x20
Lloh28:
	adrp	x10, l_.str.7@PAGE
Lloh29:
	add	x10, x10, l_.str.7@PAGEOFF
	stp	x9, x8, [sp, #16]
	stp	x10, x19, [sp]
Lloh30:
	adrp	x0, l_.str.9@PAGE
Lloh31:
	add	x0, x0, l_.str.9@PAGEOFF
LBB0_38:
	bl	_printf
	ldr	w8, [x23, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB0_40
; %bb.39:
Lloh32:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh33:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB0_40:
	bl	_mach_absolute_time
	mov	x20, x0
	ldp	w25, w26, [x22]
	fmov	d9, #1.00000000
	cbz	x19, LBB0_56
; %bb.41:
	mov	x8, #536870912                  ; =0x20000000
	movk	x8, #16368, lsl #48
	fmov	d0, x8
	mov	x8, #281474439839744            ; =0xffffe0000000
	movk	x8, #16367, lsl #48
	fmov	d1, x8
	mov	x8, x19
LBB0_42:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	fmadd	d9, d9, d0, d1
	; InlineAsm End
	subs	x8, x8, #1
	b.ne	LBB0_42
; %bb.43:
	ldr	w8, [x23, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB0_45
LBB0_44:
Lloh34:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh35:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB0_45:
	mul	x8, x20, x25
	udiv	x20, x8, x26
	bl	_mach_absolute_time
	ldp	w8, w9, [x22]
	mul	x8, x0, x8
	udiv	x9, x8, x9
	str	d9, [x24, _g_f64_sink@PAGEOFF]
	sub	x8, x9, x20
	movi	d1, #0000000000000000
	fcmp	d8, #0.0
	movi	d0, #0000000000000000
	b.le	LBB0_49
; %bb.46:
	ucvtf	d0, x8
	fdiv	d0, d0, d8
	cmp	x9, x20
	b.eq	LBB0_50
LBB0_47:
	ucvtf	d1, x8
	fdiv	d1, d8, d1
	cbnz	w21, LBB0_51
LBB0_48:
Lloh36:
	adrp	x9, l_.str.8@PAGE
Lloh37:
	add	x9, x9, l_.str.8@PAGEOFF
	stp	d0, d1, [sp, #16]
	stp	x9, x8, [sp]
Lloh38:
	adrp	x0, l_.str.10@PAGE
Lloh39:
	add	x0, x0, l_.str.10@PAGEOFF
	b	LBB0_52
LBB0_49:
	cmp	x9, x20
	b.ne	LBB0_47
LBB0_50:
	cbz	w21, LBB0_48
LBB0_51:
	stp	d0, d1, [sp, #32]
	mov	w9, #32                         ; =0x20
Lloh40:
	adrp	x10, l_.str.8@PAGE
Lloh41:
	add	x10, x10, l_.str.8@PAGEOFF
	stp	x9, x8, [sp, #16]
	stp	x10, x19, [sp]
Lloh42:
	adrp	x0, l_.str.9@PAGE
Lloh43:
	add	x0, x0, l_.str.9@PAGEOFF
LBB0_52:
	bl	_printf
	mov	w0, #0                          ; =0x0
LBB0_53:
	ldp	x29, x30, [sp, #144]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #128]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #112]            ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #96]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #80]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #64]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #48]               ; 16-byte Folded Reload
	add	sp, sp, #160
	ret
LBB0_54:
	mov	x8, x26
	cbnz	w8, LBB0_17
	b	LBB0_16
LBB0_55:
	fmov	d9, #1.00000000
	mov	x8, x25
	cbnz	w8, LBB0_31
	b	LBB0_30
LBB0_56:
	mov	x8, x26
	cbnz	w8, LBB0_45
	b	LBB0_44
LBB0_57:
Lloh44:
	adrp	x1, l_.str.2@PAGE
Lloh45:
	add	x1, x1, l_.str.2@PAGEOFF
	mov	x0, x24
	bl	_strcmp
	cbz	w0, LBB0_59
; %bb.58:
Lloh46:
	adrp	x8, ___stderrp@GOTPAGE
Lloh47:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh48:
	ldr	x0, [x8]
	str	x24, [sp]
Lloh49:
	adrp	x1, l_.str.4@PAGE
Lloh50:
	add	x1, x1, l_.str.4@PAGEOFF
	bl	_fprintf
	mov	w0, #1                          ; =0x1
	b	LBB0_53
LBB0_59:
	ldr	x8, [x20]
	str	x8, [sp]
Lloh51:
	adrp	x0, l_.str.3@PAGE
Lloh52:
	add	x0, x0, l_.str.3@PAGEOFF
	b	LBB0_52
	.loh AdrpAdd	Lloh2, Lloh3
	.loh AdrpAdd	Lloh0, Lloh1
	.loh AdrpAdd	Lloh4, Lloh5
	.loh AdrpAdd	Lloh6, Lloh7
	.loh AdrpAdd	Lloh8, Lloh9
	.loh AdrpAdd	Lloh10, Lloh11
	.loh AdrpAdd	Lloh14, Lloh15
	.loh AdrpAdd	Lloh12, Lloh13
	.loh AdrpAdd	Lloh18, Lloh19
	.loh AdrpAdd	Lloh16, Lloh17
	.loh AdrpAdd	Lloh20, Lloh21
	.loh AdrpAdd	Lloh22, Lloh23
	.loh AdrpAdd	Lloh26, Lloh27
	.loh AdrpAdd	Lloh24, Lloh25
	.loh AdrpAdd	Lloh30, Lloh31
	.loh AdrpAdd	Lloh28, Lloh29
	.loh AdrpAdd	Lloh32, Lloh33
	.loh AdrpAdd	Lloh34, Lloh35
	.loh AdrpAdd	Lloh38, Lloh39
	.loh AdrpAdd	Lloh36, Lloh37
	.loh AdrpAdd	Lloh42, Lloh43
	.loh AdrpAdd	Lloh40, Lloh41
	.loh AdrpAdd	Lloh44, Lloh45
	.loh AdrpAdd	Lloh49, Lloh50
	.loh AdrpLdrGotLdr	Lloh46, Lloh47, Lloh48
	.loh AdrpAdd	Lloh51, Lloh52
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"--iters"

l_.str.1:                               ; @.str.1
	.asciz	"--csv"

l_.str.2:                               ; @.str.2
	.asciz	"--help"

l_.str.3:                               ; @.str.3
	.asciz	"usage: %s [--iters N] [--csv]\n"

l_.str.4:                               ; @.str.4
	.asciz	"unknown argument: %s\n"

l_.str.6:                               ; @.str.6
	.asciz	"add_dep_u64"

l_.str.7:                               ; @.str.7
	.asciz	"fadd_dep_f64"

l_.str.8:                               ; @.str.8
	.asciz	"fmadd_dep_f64"

l_.str.9:                               ; @.str.9
	.asciz	"%s,%llu,%d,%llu,%.6f,%.6f\n"

l_.str.10:                              ; @.str.10
	.asciz	"%-16s elapsed_ns=%llu ns/op=%.6f approx_Gops=%.6f\n"

.zerofill __DATA,__bss,_g_u64_sink,8,3  ; @g_u64_sink
.zerofill __DATA,__bss,_apple_re_now_ns.timebase,8,2 ; @apple_re_now_ns.timebase
.zerofill __DATA,__bss,_g_f64_sink,8,3  ; @g_f64_sink
l_str:                                  ; @str
	.asciz	"probe,iters,unroll,elapsed_ns,ns_per_op,approx_gops"

.subsections_via_symbols

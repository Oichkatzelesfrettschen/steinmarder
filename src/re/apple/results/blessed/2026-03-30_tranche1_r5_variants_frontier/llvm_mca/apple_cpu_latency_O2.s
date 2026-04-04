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
	mov	w20, #16960                     ; =0x4240
	movk	w20, #15, lsl #16
	cmp	w0, #2
	b.lt	LBB0_9
; %bb.1:
	mov	x21, x1
	mov	x22, x0
	mov	x19, #0                         ; =0x0
	mov	w27, #0                         ; =0x0
	mov	w28, #1                         ; =0x1
Lloh0:
	adrp	x23, l_.str@PAGE
Lloh1:
	add	x23, x23, l_.str@PAGEOFF
Lloh2:
	adrp	x24, l_.str.1@PAGE
Lloh3:
	add	x24, x24, l_.str.1@PAGEOFF
	b	LBB0_4
LBB0_2:                                 ;   in Loop: Header=BB0_4 Depth=1
	ldr	x0, [x21, x25, lsl #3]
	mov	x1, #0                          ; =0x0
	mov	w2, #10                         ; =0xa
	bl	_strtoull
	mov	x20, x0
LBB0_3:                                 ;   in Loop: Header=BB0_4 Depth=1
	add	w28, w25, #1
	cmp	w28, w22
	b.ge	LBB0_10
LBB0_4:                                 ; =>This Inner Loop Header: Depth=1
	sxtw	x25, w28
	ldr	x26, [x21, w28, sxtw #3]
	mov	x0, x26
	mov	x1, x23
	bl	_strcmp
	cmp	w0, #0
	add	x25, x25, #1
	ccmp	w25, w22, #0, eq
	b.lt	LBB0_2
; %bb.5:                                ;   in Loop: Header=BB0_4 Depth=1
	mov	x0, x26
	mov	x1, x24
	bl	_strcmp
	cmp	w0, #0
	ccmp	w25, w22, #0, eq
	b.lt	LBB0_8
; %bb.6:                                ;   in Loop: Header=BB0_4 Depth=1
	mov	x0, x26
Lloh4:
	adrp	x1, l_.str.2@PAGE
Lloh5:
	add	x1, x1, l_.str.2@PAGEOFF
	bl	_strcmp
	cbnz	w0, LBB0_30
; %bb.7:                                ;   in Loop: Header=BB0_4 Depth=1
	mov	w27, #1                         ; =0x1
	mov	x25, x28
	b	LBB0_3
LBB0_8:                                 ;   in Loop: Header=BB0_4 Depth=1
	ldr	x19, [x21, x25, lsl #3]
	b	LBB0_3
LBB0_9:
	mov	x19, #0                         ; =0x0
	b	LBB0_12
LBB0_10:
	tbz	w27, #0, LBB0_12
; %bb.11:
Lloh6:
	adrp	x0, l_str@PAGE
Lloh7:
	add	x0, x0, l_str@PAGEOFF
	bl	_puts
	mov	w24, #1                         ; =0x1
	b	LBB0_13
LBB0_12:
	mov	w24, #0                         ; =0x0
LBB0_13:
	mov	w27, #0                         ; =0x0
	ucvtf	d8, x20
Lloh8:
	adrp	x25, _g_probes@PAGE+16
Lloh9:
	add	x25, x25, _g_probes@PAGEOFF+16
	mov	w26, #7                         ; =0x7
Lloh10:
	adrp	x21, l_.str.17@PAGE
Lloh11:
	add	x21, x21, l_.str.17@PAGEOFF
Lloh12:
	adrp	x22, l_.str.18@PAGE
Lloh13:
	add	x22, x22, l_.str.18@PAGEOFF
	b	LBB0_17
LBB0_14:                                ;   in Loop: Header=BB0_17 Depth=1
	stp	d1, d0, [sp, #16]
	stp	x23, x0, [sp]
	mov	x0, x22
LBB0_15:                                ;   in Loop: Header=BB0_17 Depth=1
	bl	_printf
	mov	w27, #1                         ; =0x1
LBB0_16:                                ;   in Loop: Header=BB0_17 Depth=1
	add	x25, x25, #24
	subs	x26, x26, #1
	b.eq	LBB0_25
LBB0_17:                                ; =>This Inner Loop Header: Depth=1
	ldur	x23, [x25, #-16]
	cbz	x19, LBB0_19
; %bb.18:                               ;   in Loop: Header=BB0_17 Depth=1
	mov	x0, x19
	mov	x1, x23
	bl	_strcmp
	cbnz	w0, LBB0_16
LBB0_19:                                ;   in Loop: Header=BB0_17 Depth=1
	ldur	x8, [x25, #-8]
	mov	x0, x20
	blr	x8
	ldr	w8, [x25]
	ucvtf	d0, w8
	fmul	d2, d8, d0
	movi	d0, #0000000000000000
	fcmp	d2, #0.0
	ucvtf	d3, x0
	movi	d1, #0000000000000000
	b.le	LBB0_21
; %bb.20:                               ;   in Loop: Header=BB0_17 Depth=1
	fdiv	d1, d3, d2
LBB0_21:                                ;   in Loop: Header=BB0_17 Depth=1
	cbz	x0, LBB0_23
; %bb.22:                               ;   in Loop: Header=BB0_17 Depth=1
	fdiv	d0, d2, d3
LBB0_23:                                ;   in Loop: Header=BB0_17 Depth=1
	cbz	w24, LBB0_14
; %bb.24:                               ;   in Loop: Header=BB0_17 Depth=1
	stp	d1, d0, [sp, #32]
	stp	x8, x0, [sp, #16]
	stp	x23, x20, [sp]
	mov	x0, x21
	b	LBB0_15
LBB0_25:
	tbz	w27, #0, LBB0_27
; %bb.26:
	mov	w0, #0                          ; =0x0
	b	LBB0_29
LBB0_27:
Lloh14:
	adrp	x8, ___stderrp@GOTPAGE
Lloh15:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh16:
	ldr	x0, [x8]
Lloh17:
	adrp	x8, l_.str.9@PAGE
Lloh18:
	add	x8, x8, l_.str.9@PAGEOFF
	cmp	x19, #0
	csel	x8, x8, x19, eq
	str	x8, [sp]
Lloh19:
	adrp	x1, l_.str.8@PAGE
Lloh20:
	add	x1, x1, l_.str.8@PAGEOFF
LBB0_28:
	bl	_fprintf
	mov	w0, #1                          ; =0x1
LBB0_29:
	ldp	x29, x30, [sp, #144]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #128]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #112]            ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #96]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #80]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #64]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #48]               ; 16-byte Folded Reload
	add	sp, sp, #160
	ret
LBB0_30:
Lloh21:
	adrp	x1, l_.str.3@PAGE
Lloh22:
	add	x1, x1, l_.str.3@PAGEOFF
	mov	x0, x26
	bl	_strcmp
	cbz	w0, LBB0_33
; %bb.31:
Lloh23:
	adrp	x1, l_.str.4@PAGE
Lloh24:
	add	x1, x1, l_.str.4@PAGEOFF
	mov	x0, x26
	bl	_strcmp
	cbz	w0, LBB0_34
; %bb.32:
Lloh25:
	adrp	x8, ___stderrp@GOTPAGE
Lloh26:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh27:
	ldr	x0, [x8]
	str	x26, [sp]
Lloh28:
	adrp	x1, l_.str.6@PAGE
Lloh29:
	add	x1, x1, l_.str.6@PAGEOFF
	b	LBB0_28
LBB0_33:
Lloh30:
	adrp	x0, l_.str.10@PAGE
Lloh31:
	add	x0, x0, l_.str.10@PAGEOFF
	bl	_puts
Lloh32:
	adrp	x0, l_.str.11@PAGE
Lloh33:
	add	x0, x0, l_.str.11@PAGEOFF
	bl	_puts
Lloh34:
	adrp	x0, l_.str.12@PAGE
Lloh35:
	add	x0, x0, l_.str.12@PAGEOFF
	bl	_puts
Lloh36:
	adrp	x0, l_.str.13@PAGE
Lloh37:
	add	x0, x0, l_.str.13@PAGEOFF
	bl	_puts
Lloh38:
	adrp	x0, l_.str.14@PAGE
Lloh39:
	add	x0, x0, l_.str.14@PAGEOFF
	bl	_puts
Lloh40:
	adrp	x0, l_.str.15@PAGE
Lloh41:
	add	x0, x0, l_.str.15@PAGEOFF
	bl	_puts
Lloh42:
	adrp	x0, l_.str.16@PAGE
Lloh43:
	add	x0, x0, l_.str.16@PAGEOFF
	bl	_puts
	mov	w0, #0                          ; =0x0
	b	LBB0_29
LBB0_34:
	ldr	x8, [x21]
	str	x8, [sp]
Lloh44:
	adrp	x0, l_.str.5@PAGE
Lloh45:
	add	x0, x0, l_.str.5@PAGEOFF
	bl	_printf
	mov	w0, #0                          ; =0x0
	b	LBB0_29
	.loh AdrpAdd	Lloh2, Lloh3
	.loh AdrpAdd	Lloh0, Lloh1
	.loh AdrpAdd	Lloh4, Lloh5
	.loh AdrpAdd	Lloh6, Lloh7
	.loh AdrpAdd	Lloh12, Lloh13
	.loh AdrpAdd	Lloh10, Lloh11
	.loh AdrpAdd	Lloh8, Lloh9
	.loh AdrpAdd	Lloh19, Lloh20
	.loh AdrpAdd	Lloh17, Lloh18
	.loh AdrpLdrGotLdr	Lloh14, Lloh15, Lloh16
	.loh AdrpAdd	Lloh21, Lloh22
	.loh AdrpAdd	Lloh23, Lloh24
	.loh AdrpAdd	Lloh28, Lloh29
	.loh AdrpLdrGotLdr	Lloh25, Lloh26, Lloh27
	.loh AdrpAdd	Lloh42, Lloh43
	.loh AdrpAdd	Lloh40, Lloh41
	.loh AdrpAdd	Lloh38, Lloh39
	.loh AdrpAdd	Lloh36, Lloh37
	.loh AdrpAdd	Lloh34, Lloh35
	.loh AdrpAdd	Lloh32, Lloh33
	.loh AdrpAdd	Lloh30, Lloh31
	.loh AdrpAdd	Lloh44, Lloh45
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep
_bench_add_dep:                         ; @bench_add_dep
	.cfi_startproc
; %bb.0:
	stp	x26, x25, [sp, #-80]!           ; 16-byte Folded Spill
	stp	x24, x23, [sp, #16]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #32]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #48]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #64]             ; 16-byte Folded Spill
	add	x29, sp, #64
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
	mov	x20, x0
	adrp	x25, _apple_re_now_ns.timebase@PAGE+4
	ldr	w8, [x25, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB1_2
; %bb.1:
Lloh46:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh47:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB1_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh48:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh49:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w23, w24, [x21]
	mov	w22, #1                         ; =0x1
	cbz	x20, LBB1_7
LBB1_3:                                 ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	add	x22, x22, #1
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB1_3
; %bb.4:
	ldr	w8, [x25, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB1_6
LBB1_5:
Lloh50:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh51:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB1_6:
	mul	x8, x19, x23
	udiv	x19, x8, x24
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	adrp	x9, _g_u64_sink@PAGE
	str	x22, [x9, _g_u64_sink@PAGEOFF]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp], #80             ; 16-byte Folded Reload
	ret
LBB1_7:
	mov	x8, x24
	cbnz	w8, LBB1_6
	b	LBB1_5
	.loh AdrpAdd	Lloh46, Lloh47
	.loh AdrpAdd	Lloh48, Lloh49
	.loh AdrpAdd	Lloh50, Lloh51
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_fadd_dep
_bench_fadd_dep:                        ; @bench_fadd_dep
	.cfi_startproc
; %bb.0:
	stp	d9, d8, [sp, #-80]!             ; 16-byte Folded Spill
	stp	x24, x23, [sp, #16]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #32]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #48]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #64]             ; 16-byte Folded Spill
	add	x29, sp, #64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset b8, -72
	.cfi_offset b9, -80
	mov	x20, x0
	adrp	x24, _apple_re_now_ns.timebase@PAGE+4
	ldr	w8, [x24, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB2_2
; %bb.1:
Lloh52:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh53:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB2_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh54:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh55:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbz	x20, LBB2_8
; %bb.3:
	fmov	d0, #1.00000000
	fmov	d8, #1.00000000
LBB2_4:                                 ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	fadd	d8, d8, d0
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB2_4
; %bb.5:
	ldr	w8, [x24, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB2_7
LBB2_6:
Lloh56:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh57:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB2_7:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	adrp	x9, _g_f64_sink@PAGE
	str	d8, [x9, _g_f64_sink@PAGEOFF]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp], #80               ; 16-byte Folded Reload
	ret
LBB2_8:
	fmov	d8, #1.00000000
	mov	x8, x23
	cbnz	w8, LBB2_7
	b	LBB2_6
	.loh AdrpAdd	Lloh52, Lloh53
	.loh AdrpAdd	Lloh54, Lloh55
	.loh AdrpAdd	Lloh56, Lloh57
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_fmadd_dep
_bench_fmadd_dep:                       ; @bench_fmadd_dep
	.cfi_startproc
; %bb.0:
	stp	d9, d8, [sp, #-80]!             ; 16-byte Folded Spill
	stp	x24, x23, [sp, #16]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #32]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #48]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #64]             ; 16-byte Folded Spill
	add	x29, sp, #64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset b8, -72
	.cfi_offset b9, -80
	mov	x20, x0
	adrp	x24, _apple_re_now_ns.timebase@PAGE+4
	ldr	w8, [x24, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB3_2
; %bb.1:
Lloh58:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh59:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB3_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh60:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh61:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	fmov	d8, #1.00000000
	cbz	x20, LBB3_8
; %bb.3:
	mov	x8, #536870912                  ; =0x20000000
	movk	x8, #16368, lsl #48
	fmov	d0, x8
	mov	x8, #281474439839744            ; =0xffffe0000000
	movk	x8, #16367, lsl #48
	fmov	d1, x8
LBB3_4:                                 ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	fmadd	d8, d8, d0, d1
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB3_4
; %bb.5:
	ldr	w8, [x24, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB3_7
LBB3_6:
Lloh62:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh63:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB3_7:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	adrp	x9, _g_f64_sink@PAGE
	str	d8, [x9, _g_f64_sink@PAGEOFF]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp], #80               ; 16-byte Folded Reload
	ret
LBB3_8:
	mov	x8, x23
	cbnz	w8, LBB3_7
	b	LBB3_6
	.loh AdrpAdd	Lloh58, Lloh59
	.loh AdrpAdd	Lloh60, Lloh61
	.loh AdrpAdd	Lloh62, Lloh63
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_load_store_chain
_bench_load_store_chain:                ; @bench_load_store_chain
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #352
	stp	x28, x27, [sp, #272]            ; 16-byte Folded Spill
	stp	x24, x23, [sp, #288]            ; 16-byte Folded Spill
	stp	x22, x21, [sp, #304]            ; 16-byte Folded Spill
	stp	x20, x19, [sp, #320]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #336]            ; 16-byte Folded Spill
	add	x29, sp, #336
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset w27, -72
	.cfi_offset w28, -80
	mov	x19, x0
Lloh64:
	adrp	x8, ___stack_chk_guard@GOTPAGE
Lloh65:
	ldr	x8, [x8, ___stack_chk_guard@GOTPAGEOFF]
Lloh66:
	ldr	x8, [x8]
	stur	x8, [x29, #-72]
	mov	w8, #1                          ; =0x1
	str	x8, [sp, #8]
	mov	w8, #2                          ; =0x2
	str	x8, [sp, #16]
	mov	w8, #3                          ; =0x3
	str	x8, [sp, #24]
	mov	w8, #4                          ; =0x4
	str	x8, [sp, #32]
	mov	w8, #5                          ; =0x5
	str	x8, [sp, #40]
	mov	w8, #6                          ; =0x6
	str	x8, [sp, #48]
	mov	w8, #7                          ; =0x7
	str	x8, [sp, #56]
	mov	w8, #8                          ; =0x8
	str	x8, [sp, #64]
	mov	w8, #9                          ; =0x9
	str	x8, [sp, #72]
	mov	w8, #10                         ; =0xa
	str	x8, [sp, #80]
	mov	w8, #11                         ; =0xb
	str	x8, [sp, #88]
	mov	w8, #12                         ; =0xc
	str	x8, [sp, #96]
	mov	w8, #13                         ; =0xd
	str	x8, [sp, #104]
	mov	w8, #14                         ; =0xe
	str	x8, [sp, #112]
	mov	w8, #15                         ; =0xf
	str	x8, [sp, #120]
	mov	w8, #16                         ; =0x10
	str	x8, [sp, #128]
	mov	w8, #17                         ; =0x11
	str	x8, [sp, #136]
	mov	w8, #18                         ; =0x12
	str	x8, [sp, #144]
	mov	w8, #19                         ; =0x13
	str	x8, [sp, #152]
	mov	w8, #20                         ; =0x14
	str	x8, [sp, #160]
	mov	w8, #21                         ; =0x15
	str	x8, [sp, #168]
	mov	w8, #22                         ; =0x16
	str	x8, [sp, #176]
	mov	w8, #23                         ; =0x17
	str	x8, [sp, #184]
	mov	w8, #24                         ; =0x18
	str	x8, [sp, #192]
	mov	w8, #25                         ; =0x19
	str	x8, [sp, #200]
	mov	w8, #26                         ; =0x1a
	str	x8, [sp, #208]
	mov	w8, #27                         ; =0x1b
	str	x8, [sp, #216]
	mov	w8, #28                         ; =0x1c
	str	x8, [sp, #224]
	mov	w8, #29                         ; =0x1d
	str	x8, [sp, #232]
	mov	w8, #30                         ; =0x1e
	str	x8, [sp, #240]
	mov	w8, #31                         ; =0x1f
	str	x8, [sp, #248]
	mov	w8, #32                         ; =0x20
	str	x8, [sp, #256]
Lloh67:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh68:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB4_2
; %bb.1:
Lloh69:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh70:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB4_2:
	mov	x23, #31765                     ; =0x7c15
	movk	x23, #32586, lsl #16
	movk	x23, #31161, lsl #32
	movk	x23, #40503, lsl #48
	bl	_mach_absolute_time
	mov	x20, x0
Lloh71:
	adrp	x24, _apple_re_now_ns.timebase@PAGE
Lloh72:
	add	x24, x24, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w21, w22, [x24]
	cbz	x19, LBB4_7
; %bb.3:
	mov	x8, #0                          ; =0x0
	add	x9, sp, #8
LBB4_4:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB4_5 Depth 2
	mov	x10, #0                         ; =0x0
LBB4_5:                                 ;   Parent Loop BB4_4 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	x11, [x9, x10, lsl #3]
	add	x12, x10, x11
	eor	x12, x12, x23
	ror	x23, x12, #63
	add	x11, x23, x11
	str	x11, [x9, x10, lsl #3]
	add	x10, x10, #1
	cmp	x10, #32
	b.ne	LBB4_5
; %bb.6:                                ;   in Loop: Header=BB4_4 Depth=1
	add	x8, x8, #1
	cmp	x8, x19
	b.ne	LBB4_4
LBB4_7:
	cbnz	w22, LBB4_9
; %bb.8:
Lloh73:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh74:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB4_9:
	bl	_mach_absolute_time
	ldp	w9, w8, [x24]
	and	x10, x23, #0x1f
	add	x11, sp, #8
	ldr	x10, [x11, x10, lsl #3]
	eor	x10, x10, x23
Lloh75:
	adrp	x11, _g_u64_sink@PAGE
	str	x10, [x11, _g_u64_sink@PAGEOFF]
	ldur	x10, [x29, #-72]
Lloh76:
	adrp	x11, ___stack_chk_guard@GOTPAGE
Lloh77:
	ldr	x11, [x11, ___stack_chk_guard@GOTPAGEOFF]
Lloh78:
	ldr	x11, [x11]
	cmp	x11, x10
	b.ne	LBB4_11
; %bb.10:
	mul	x10, x20, x21
	udiv	x10, x10, x22
	mul	x9, x0, x9
	udiv	x8, x9, x8
	sub	x0, x8, x10
	ldp	x29, x30, [sp, #336]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #320]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #304]            ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #288]            ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #272]            ; 16-byte Folded Reload
	add	sp, sp, #352
	ret
LBB4_11:
	bl	___stack_chk_fail
	.loh AdrpLdr	Lloh67, Lloh68
	.loh AdrpLdrGotLdr	Lloh64, Lloh65, Lloh66
	.loh AdrpAdd	Lloh69, Lloh70
	.loh AdrpAdd	Lloh71, Lloh72
	.loh AdrpAdd	Lloh73, Lloh74
	.loh AdrpLdrGotLdr	Lloh76, Lloh77, Lloh78
	.loh AdrpAdrp	Lloh75, Lloh76
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_shuffle_dep
_bench_shuffle_dep:                     ; @bench_shuffle_dep
	.cfi_startproc
; %bb.0:
	stp	x24, x23, [sp, #-64]!           ; 16-byte Folded Spill
	stp	x22, x21, [sp, #16]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #32]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	mov	x19, x0
Lloh79:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh80:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB5_2
; %bb.1:
Lloh81:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh82:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB5_2:
	mov	x21, #52719                     ; =0xcdef
	movk	x21, #35243, lsl #16
	movk	x21, #17767, lsl #32
	movk	x21, #291, lsl #48
	bl	_mach_absolute_time
	mov	x20, x0
Lloh83:
	adrp	x22, _apple_re_now_ns.timebase@PAGE
Lloh84:
	add	x22, x22, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w23, w24, [x22]
	cbz	x19, LBB5_7
; %bb.3:
	mov	x8, #0                          ; =0x0
	mov	x9, #31765                      ; =0x7c15
	movk	x9, #32586, lsl #16
	movk	x9, #31161, lsl #32
	movk	x9, #40503, lsl #48
LBB5_4:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_5 Depth 2
	mov	x10, #0                         ; =0x0
	mov	x11, #0                         ; =0x0
LBB5_5:                                 ;   Parent Loop BB5_4 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	eor	x12, x10, x21
	and	w13, w11, #0xf
	add	w14, w13, #1
	lsr	x14, x12, x14
	eor	w13, w13, #0x3f
	lsl	x12, x12, x13
	orr	x12, x14, x12
	rev	x21, x12
	add	x11, x11, #1
	add	x10, x10, x9
	cmp	x11, #32
	b.ne	LBB5_5
; %bb.6:                                ;   in Loop: Header=BB5_4 Depth=1
	add	x8, x8, #1
	cmp	x8, x19
	b.ne	LBB5_4
LBB5_7:
	cbnz	w24, LBB5_9
; %bb.8:
Lloh85:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh86:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB5_9:
	mul	x8, x20, x23
	udiv	x19, x8, x24
	bl	_mach_absolute_time
	ldp	w8, w9, [x22]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	adrp	x9, _g_u64_sink@PAGE
	str	x21, [x9, _g_u64_sink@PAGEOFF]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
	.loh AdrpLdr	Lloh79, Lloh80
	.loh AdrpAdd	Lloh81, Lloh82
	.loh AdrpAdd	Lloh83, Lloh84
	.loh AdrpAdd	Lloh85, Lloh86
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_atomic_add_dep
_bench_atomic_add_dep:                  ; @bench_atomic_add_dep
	.cfi_startproc
; %bb.0:
	stp	x24, x23, [sp, #-64]!           ; 16-byte Folded Spill
	stp	x22, x21, [sp, #16]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #32]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	mov	x20, x0
	mov	w8, #1                          ; =0x1
	adrp	x9, _g_atomic_counter@PAGE
	str	x8, [x9, _g_atomic_counter@PAGEOFF]
Lloh87:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh88:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB6_2
; %bb.1:
Lloh89:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh90:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB6_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh91:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh92:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbz	x20, LBB6_7
; %bb.3:
Lloh93:
	adrp	x8, _g_atomic_counter@PAGE
Lloh94:
	add	x8, x8, _g_atomic_counter@PAGEOFF
	mov	w9, #1                          ; =0x1
LBB6_4:                                 ; =>This Inner Loop Header: Depth=1
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x10, [x8]
	ldadd	x9, x24, [x8]
	subs	x20, x20, #1
	b.ne	LBB6_4
; %bb.5:
	cbz	w23, LBB6_8
LBB6_6:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	adrp	x9, _g_u64_sink@PAGE
	str	x24, [x9, _g_u64_sink@PAGEOFF]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB6_7:
	mov	x24, #0                         ; =0x0
	cbnz	w23, LBB6_6
LBB6_8:
Lloh95:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh96:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
	b	LBB6_6
	.loh AdrpLdr	Lloh87, Lloh88
	.loh AdrpAdd	Lloh89, Lloh90
	.loh AdrpAdd	Lloh91, Lloh92
	.loh AdrpAdd	Lloh93, Lloh94
	.loh AdrpAdd	Lloh95, Lloh96
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_transcendental_dep
_bench_transcendental_dep:              ; @bench_transcendental_dep
	.cfi_startproc
; %bb.0:
	stp	d13, d12, [sp, #-112]!          ; 16-byte Folded Spill
	stp	d11, d10, [sp, #16]             ; 16-byte Folded Spill
	stp	d9, d8, [sp, #32]               ; 16-byte Folded Spill
	stp	x24, x23, [sp, #48]             ; 16-byte Folded Spill
	stp	x22, x21, [sp, #64]             ; 16-byte Folded Spill
	stp	x20, x19, [sp, #80]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #96]             ; 16-byte Folded Spill
	add	x29, sp, #96
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset b8, -72
	.cfi_offset b9, -80
	.cfi_offset b10, -88
	.cfi_offset b11, -96
	.cfi_offset b12, -104
	.cfi_offset b13, -112
	mov	x20, x0
Lloh97:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh98:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB7_2
; %bb.1:
Lloh99:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh100:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB7_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh101:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh102:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	mov	x8, #25439                      ; =0x635f
	movk	x8, #14137, lsl #16
	movk	x8, #39645, lsl #32
	movk	x8, #16319, lsl #48
	fmov	d8, x8
	cbz	x20, LBB7_5
; %bb.3:
	mov	x8, #43516                      ; =0xa9fc
	movk	x8, #54001, lsl #16
	movk	x8, #25165, lsl #32
	movk	x8, #16208, lsl #48
	fmov	d9, x8
	mov	x8, #43516                      ; =0xa9fc
	movk	x8, #54001, lsl #16
	movk	x8, #25165, lsl #32
	movk	x8, #16224, lsl #48
	fmov	d10, x8
	mov	x8, #32506                      ; =0x7efa
	movk	x8, #48234, lsl #16
	movk	x8, #37748, lsl #32
	movk	x8, #16232, lsl #48
	fmov	d11, x8
	mov	x8, #43516                      ; =0xa9fc
	movk	x8, #54001, lsl #16
	movk	x8, #25165, lsl #32
	movk	x8, #16240, lsl #48
	fmov	d12, x8
LBB7_4:                                 ; =>This Inner Loop Header: Depth=1
	fadd	d0, d8, d9
	bl	_sin
	fadd	d0, d8, d0
	bl	_cos
	fmov	d8, d0
	fadd	d0, d0, d10
	bl	_sin
	fadd	d0, d8, d0
	bl	_cos
	fmov	d8, d0
	fadd	d0, d0, d11
	bl	_sin
	fadd	d0, d8, d0
	bl	_cos
	fmov	d8, d0
	fadd	d0, d0, d12
	bl	_sin
	fadd	d0, d8, d0
	bl	_cos
	fmov	d8, d0
	subs	x20, x20, #1
	b.ne	LBB7_4
LBB7_5:
	cbnz	w23, LBB7_7
; %bb.6:
Lloh103:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh104:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB7_7:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	adrp	x9, _g_f64_sink@PAGE
	str	d8, [x9, _g_f64_sink@PAGEOFF]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #96]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #80]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #64]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #48]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #32]               ; 16-byte Folded Reload
	ldp	d11, d10, [sp, #16]             ; 16-byte Folded Reload
	ldp	d13, d12, [sp], #112            ; 16-byte Folded Reload
	ret
	.loh AdrpLdr	Lloh97, Lloh98
	.loh AdrpAdd	Lloh99, Lloh100
	.loh AdrpAdd	Lloh101, Lloh102
	.loh AdrpAdd	Lloh103, Lloh104
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__cstring,cstring_literals
l_.str:                                 ; @.str
	.asciz	"--iters"

l_.str.1:                               ; @.str.1
	.asciz	"--probe"

l_.str.2:                               ; @.str.2
	.asciz	"--csv"

l_.str.3:                               ; @.str.3
	.asciz	"--list-probes"

	.section	__DATA,__const
	.p2align	3, 0x0                          ; @g_probes
_g_probes:
	.quad	l_.str.10
	.quad	_bench_add_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.11
	.quad	_bench_fadd_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.12
	.quad	_bench_fmadd_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.13
	.quad	_bench_load_store_chain
	.long	96                              ; 0x60
	.space	4
	.quad	l_.str.14
	.quad	_bench_shuffle_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.15
	.quad	_bench_atomic_add_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.16
	.quad	_bench_transcendental_dep
	.long	8                               ; 0x8
	.space	4

	.section	__TEXT,__cstring,cstring_literals
l_.str.4:                               ; @.str.4
	.asciz	"--help"

l_.str.5:                               ; @.str.5
	.asciz	"usage: %s [--iters N] [--probe NAME] [--list-probes] [--csv]\n"

l_.str.6:                               ; @.str.6
	.asciz	"unknown argument: %s\n"

l_.str.8:                               ; @.str.8
	.asciz	"no probe matched --probe %s\n"

l_.str.9:                               ; @.str.9
	.asciz	"(null)"

l_.str.10:                              ; @.str.10
	.asciz	"add_dep_u64"

l_.str.11:                              ; @.str.11
	.asciz	"fadd_dep_f64"

l_.str.12:                              ; @.str.12
	.asciz	"fmadd_dep_f64"

l_.str.13:                              ; @.str.13
	.asciz	"load_store_chain_u64"

l_.str.14:                              ; @.str.14
	.asciz	"shuffle_lane_cross_u64"

l_.str.15:                              ; @.str.15
	.asciz	"atomic_add_relaxed_u64"

l_.str.16:                              ; @.str.16
	.asciz	"transcendental_sin_cos_f64"

.zerofill __DATA,__bss,_g_u64_sink,8,3  ; @g_u64_sink
.zerofill __DATA,__bss,_apple_re_now_ns.timebase,8,2 ; @apple_re_now_ns.timebase
.zerofill __DATA,__bss,_g_f64_sink,8,3  ; @g_f64_sink
	.section	__DATA,__data
	.p2align	3, 0x0                          ; @g_atomic_counter
_g_atomic_counter:
	.quad	1                               ; 0x1

	.section	__TEXT,__cstring,cstring_literals
l_.str.17:                              ; @.str.17
	.asciz	"%s,%llu,%u,%llu,%.6f,%.6f\n"

l_.str.18:                              ; @.str.18
	.asciz	"%-24s elapsed_ns=%llu ns/op=%.6f approx_Gops=%.6f\n"

l_str:                                  ; @str
	.asciz	"probe,iters,unroll,elapsed_ns,ns_per_op,approx_gops"

.subsections_via_symbols

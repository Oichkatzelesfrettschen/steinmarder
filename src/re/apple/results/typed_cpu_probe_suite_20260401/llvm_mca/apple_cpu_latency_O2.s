	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 26, 0	sdk_version 26, 4
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
	mov	w26, #23                        ; =0x17
Lloh10:
	adrp	x21, l_.str.33@PAGE
Lloh11:
	add	x21, x21, l_.str.33@PAGEOFF
Lloh12:
	adrp	x22, l_.str.34@PAGE
Lloh13:
	add	x22, x22, l_.str.34@PAGEOFF
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
Lloh44:
	adrp	x0, l_.str.17@PAGE
Lloh45:
	add	x0, x0, l_.str.17@PAGEOFF
	bl	_puts
Lloh46:
	adrp	x0, l_.str.18@PAGE
Lloh47:
	add	x0, x0, l_.str.18@PAGEOFF
	bl	_puts
Lloh48:
	adrp	x0, l_.str.19@PAGE
Lloh49:
	add	x0, x0, l_.str.19@PAGEOFF
	bl	_puts
Lloh50:
	adrp	x0, l_.str.20@PAGE
Lloh51:
	add	x0, x0, l_.str.20@PAGEOFF
	bl	_puts
Lloh52:
	adrp	x0, l_.str.21@PAGE
Lloh53:
	add	x0, x0, l_.str.21@PAGEOFF
	bl	_puts
Lloh54:
	adrp	x0, l_.str.22@PAGE
Lloh55:
	add	x0, x0, l_.str.22@PAGEOFF
	bl	_puts
Lloh56:
	adrp	x0, l_.str.23@PAGE
Lloh57:
	add	x0, x0, l_.str.23@PAGEOFF
	bl	_puts
Lloh58:
	adrp	x0, l_.str.24@PAGE
Lloh59:
	add	x0, x0, l_.str.24@PAGEOFF
	bl	_puts
Lloh60:
	adrp	x0, l_.str.25@PAGE
Lloh61:
	add	x0, x0, l_.str.25@PAGEOFF
	bl	_puts
Lloh62:
	adrp	x0, l_.str.26@PAGE
Lloh63:
	add	x0, x0, l_.str.26@PAGEOFF
	bl	_puts
Lloh64:
	adrp	x0, l_.str.27@PAGE
Lloh65:
	add	x0, x0, l_.str.27@PAGEOFF
	bl	_puts
Lloh66:
	adrp	x0, l_.str.28@PAGE
Lloh67:
	add	x0, x0, l_.str.28@PAGEOFF
	bl	_puts
Lloh68:
	adrp	x0, l_.str.29@PAGE
Lloh69:
	add	x0, x0, l_.str.29@PAGEOFF
	bl	_puts
Lloh70:
	adrp	x0, l_.str.30@PAGE
Lloh71:
	add	x0, x0, l_.str.30@PAGEOFF
	bl	_puts
Lloh72:
	adrp	x0, l_.str.31@PAGE
Lloh73:
	add	x0, x0, l_.str.31@PAGEOFF
	bl	_puts
Lloh74:
	adrp	x0, l_.str.32@PAGE
Lloh75:
	add	x0, x0, l_.str.32@PAGEOFF
	bl	_puts
	mov	w0, #0                          ; =0x0
	b	LBB0_29
LBB0_34:
	ldr	x8, [x21]
	str	x8, [sp]
Lloh76:
	adrp	x0, l_.str.5@PAGE
Lloh77:
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
	.loh AdrpAdd	Lloh74, Lloh75
	.loh AdrpAdd	Lloh72, Lloh73
	.loh AdrpAdd	Lloh70, Lloh71
	.loh AdrpAdd	Lloh68, Lloh69
	.loh AdrpAdd	Lloh66, Lloh67
	.loh AdrpAdd	Lloh64, Lloh65
	.loh AdrpAdd	Lloh62, Lloh63
	.loh AdrpAdd	Lloh60, Lloh61
	.loh AdrpAdd	Lloh58, Lloh59
	.loh AdrpAdd	Lloh56, Lloh57
	.loh AdrpAdd	Lloh54, Lloh55
	.loh AdrpAdd	Lloh52, Lloh53
	.loh AdrpAdd	Lloh50, Lloh51
	.loh AdrpAdd	Lloh48, Lloh49
	.loh AdrpAdd	Lloh46, Lloh47
	.loh AdrpAdd	Lloh44, Lloh45
	.loh AdrpAdd	Lloh42, Lloh43
	.loh AdrpAdd	Lloh40, Lloh41
	.loh AdrpAdd	Lloh38, Lloh39
	.loh AdrpAdd	Lloh36, Lloh37
	.loh AdrpAdd	Lloh34, Lloh35
	.loh AdrpAdd	Lloh32, Lloh33
	.loh AdrpAdd	Lloh30, Lloh31
	.loh AdrpAdd	Lloh76, Lloh77
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
Lloh78:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh79:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB1_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh80:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh81:
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
Lloh82:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh83:
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
	.loh AdrpAdd	Lloh78, Lloh79
	.loh AdrpAdd	Lloh80, Lloh81
	.loh AdrpAdd	Lloh82, Lloh83
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep_u32
_bench_add_dep_u32:                     ; @bench_add_dep_u32
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
Lloh84:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh85:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB2_2
; %bb.1:
Lloh86:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh87:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB2_2:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh88:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh89:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbnz	w23, LBB2_4
; %bb.3:
Lloh90:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh91:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB2_4:
	mov	w24, #1                         ; =0x1
	bfi	x24, x19, #5, #27
	mul	x8, x20, x22
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
	.loh AdrpLdr	Lloh84, Lloh85
	.loh AdrpAdd	Lloh86, Lloh87
	.loh AdrpAdd	Lloh88, Lloh89
	.loh AdrpAdd	Lloh90, Lloh91
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep_i64
_bench_add_dep_i64:                     ; @bench_add_dep_i64
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
Lloh92:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh93:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB3_2
; %bb.1:
Lloh94:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh95:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB3_2:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh96:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh97:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbnz	w23, LBB3_4
; %bb.3:
Lloh98:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh99:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB3_4:
	mov	w8, #1                          ; =0x1
	orr	x19, x8, x19, lsl #5
	mul	x8, x20, x22
	udiv	x20, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	adrp	x9, _g_u64_sink@PAGE
	str	x19, [x9, _g_u64_sink@PAGEOFF]
	sub	x0, x8, x20
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
	.loh AdrpLdr	Lloh92, Lloh93
	.loh AdrpAdd	Lloh94, Lloh95
	.loh AdrpAdd	Lloh96, Lloh97
	.loh AdrpAdd	Lloh98, Lloh99
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep_i16
_bench_add_dep_i16:                     ; @bench_add_dep_i16
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
Lloh100:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh101:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB4_2
; %bb.1:
Lloh102:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh103:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB4_2:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh104:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh105:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbnz	w23, LBB4_4
; %bb.3:
Lloh106:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh107:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB4_4:
	mov	w24, #1                         ; =0x1
	bfi	x24, x19, #5, #11
	mul	x8, x20, x22
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
	.loh AdrpLdr	Lloh100, Lloh101
	.loh AdrpAdd	Lloh102, Lloh103
	.loh AdrpAdd	Lloh104, Lloh105
	.loh AdrpAdd	Lloh106, Lloh107
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep_i8
_bench_add_dep_i8:                      ; @bench_add_dep_i8
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
Lloh108:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh109:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB5_2
; %bb.1:
Lloh110:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh111:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB5_2:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh112:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh113:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbnz	w23, LBB5_4
; %bb.3:
Lloh114:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh115:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB5_4:
	mov	w24, #1                         ; =0x1
	bfi	x24, x19, #5, #3
	mul	x8, x20, x22
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
	.loh AdrpLdr	Lloh108, Lloh109
	.loh AdrpAdd	Lloh110, Lloh111
	.loh AdrpAdd	Lloh112, Lloh113
	.loh AdrpAdd	Lloh114, Lloh115
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep_u16
_bench_add_dep_u16:                     ; @bench_add_dep_u16
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
Lloh116:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh117:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB6_2
; %bb.1:
Lloh118:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh119:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB6_2:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh120:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh121:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbnz	w23, LBB6_4
; %bb.3:
Lloh122:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh123:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB6_4:
	mov	w24, #1                         ; =0x1
	bfi	x24, x19, #5, #11
	mul	x8, x20, x22
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
	.loh AdrpLdr	Lloh116, Lloh117
	.loh AdrpAdd	Lloh118, Lloh119
	.loh AdrpAdd	Lloh120, Lloh121
	.loh AdrpAdd	Lloh122, Lloh123
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep_u8
_bench_add_dep_u8:                      ; @bench_add_dep_u8
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
Lloh124:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh125:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB7_2
; %bb.1:
Lloh126:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh127:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB7_2:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh128:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh129:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbnz	w23, LBB7_4
; %bb.3:
Lloh130:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh131:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB7_4:
	mov	w24, #1                         ; =0x1
	bfi	x24, x19, #5, #3
	mul	x8, x20, x22
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
	.loh AdrpLdr	Lloh124, Lloh125
	.loh AdrpAdd	Lloh126, Lloh127
	.loh AdrpAdd	Lloh128, Lloh129
	.loh AdrpAdd	Lloh130, Lloh131
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
	cbnz	w8, LBB8_2
; %bb.1:
Lloh132:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh133:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB8_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh134:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh135:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbz	x20, LBB8_8
; %bb.3:
	fmov	d0, #1.00000000
	fmov	d8, #1.00000000
LBB8_4:                                 ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB8_4
; %bb.5:
	ldr	w8, [x24, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB8_7
LBB8_6:
Lloh136:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh137:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB8_7:
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
LBB8_8:
	fmov	d8, #1.00000000
	mov	x8, x23
	cbnz	w8, LBB8_7
	b	LBB8_6
	.loh AdrpAdd	Lloh132, Lloh133
	.loh AdrpAdd	Lloh134, Lloh135
	.loh AdrpAdd	Lloh136, Lloh137
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_fadd_f32_dep
_bench_fadd_f32_dep:                    ; @bench_fadd_f32_dep
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
	cbnz	w8, LBB9_2
; %bb.1:
Lloh138:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh139:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB9_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh140:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh141:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbz	x20, LBB9_8
; %bb.3:
	fmov	s1, #1.00000000
	fmov	s0, #1.00000000
LBB9_4:                                 ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	fadd	s0, s0, s1
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB9_4
; %bb.5:
	ldr	w8, [x24, _apple_re_now_ns.timebase@PAGEOFF+4]
	fcvt	d8, s0
	cbnz	w8, LBB9_7
LBB9_6:
Lloh142:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh143:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB9_7:
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
LBB9_8:
	fmov	d8, #1.00000000
	mov	x8, x23
	cbnz	w8, LBB9_7
	b	LBB9_6
	.loh AdrpAdd	Lloh138, Lloh139
	.loh AdrpAdd	Lloh140, Lloh141
	.loh AdrpAdd	Lloh142, Lloh143
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
	cbnz	w8, LBB10_2
; %bb.1:
Lloh144:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh145:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB10_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh146:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh147:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	fmov	d8, #1.00000000
	cbz	x20, LBB10_8
; %bb.3:
	mov	x8, #536870912                  ; =0x20000000
	movk	x8, #16368, lsl #48
	fmov	d0, x8
	mov	x8, #281474439839744            ; =0xffffe0000000
	movk	x8, #16367, lsl #48
	fmov	d1, x8
LBB10_4:                                ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB10_4
; %bb.5:
	ldr	w8, [x24, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB10_7
LBB10_6:
Lloh148:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh149:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB10_7:
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
LBB10_8:
	mov	x8, x23
	cbnz	w8, LBB10_7
	b	LBB10_6
	.loh AdrpAdd	Lloh144, Lloh145
	.loh AdrpAdd	Lloh146, Lloh147
	.loh AdrpAdd	Lloh148, Lloh149
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_bfloat16_roundtrip_proxy
_bench_bfloat16_roundtrip_proxy:        ; @bench_bfloat16_roundtrip_proxy
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
Lloh150:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh151:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB11_2
; %bb.1:
Lloh152:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh153:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB11_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh154:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh155:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbz	x20, LBB11_10
; %bb.3:
	mov	x8, #0                          ; =0x0
	fmov	s0, #1.00000000
	fmov	s1, #0.12500000
LBB11_4:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB11_5 Depth 2
	mov	w9, #32                         ; =0x20
LBB11_5:                                ;   Parent Loop BB11_4 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	fadd	s0, s0, s1
	fmov	w10, s0
	add	w10, w10, #8, lsl #12           ; =32768
	and	w10, w10, #0xffff0000
	fmov	s0, w10
	subs	w9, w9, #1
	b.ne	LBB11_5
; %bb.6:                                ;   in Loop: Header=BB11_4 Depth=1
	add	x8, x8, #1
	cmp	x8, x20
	b.ne	LBB11_4
; %bb.7:
	fcvt	d8, s0
	cbnz	w23, LBB11_9
LBB11_8:
Lloh156:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh157:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB11_9:
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
LBB11_10:
	fmov	d8, #1.00000000
	cbnz	w23, LBB11_9
	b	LBB11_8
	.loh AdrpLdr	Lloh150, Lloh151
	.loh AdrpAdd	Lloh152, Lloh153
	.loh AdrpAdd	Lloh154, Lloh155
	.loh AdrpAdd	Lloh156, Lloh157
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_fcvt_f32_to_f16_dep
_bench_fcvt_f32_to_f16_dep:             ; @bench_fcvt_f32_to_f16_dep
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
	cbnz	w8, LBB12_2
; %bb.1:
Lloh158:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh159:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB12_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh160:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh161:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbz	x20, LBB12_8
; %bb.3:
	fmov	s0, #1.00000000
LBB12_4:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	fcvt	h0, s0
	fcvt	s0, h0
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB12_4
; %bb.5:
	ldr	w8, [x24, _apple_re_now_ns.timebase@PAGEOFF+4]
	fcvt	d8, s0
	cbnz	w8, LBB12_7
LBB12_6:
Lloh162:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh163:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB12_7:
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
LBB12_8:
	fmov	d8, #1.00000000
	mov	x8, x23
	cbnz	w8, LBB12_7
	b	LBB12_6
	.loh AdrpAdd	Lloh158, Lloh159
	.loh AdrpAdd	Lloh160, Lloh161
	.loh AdrpAdd	Lloh162, Lloh163
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_fadd_f16_dep
_bench_fadd_f16_dep:                    ; @bench_fadd_f16_dep
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
	cbnz	w8, LBB13_2
; %bb.1:
Lloh164:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh165:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB13_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh166:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh167:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbz	x20, LBB13_8
; %bb.3:
	fmov	h0, #1.00000000
	mov	w8, #5145                       ; =0x1419
	fmov	h1, w8
LBB13_4:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	fadd	h0, h0, h1
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB13_4
; %bb.5:
	ldr	w8, [x24, _apple_re_now_ns.timebase@PAGEOFF+4]
	fcvt	d8, h0
	cbnz	w8, LBB13_7
LBB13_6:
Lloh168:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh169:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB13_7:
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
LBB13_8:
	fmov	d8, #1.00000000
	mov	x8, x23
	cbnz	w8, LBB13_7
	b	LBB13_6
	.loh AdrpAdd	Lloh164, Lloh165
	.loh AdrpAdd	Lloh166, Lloh167
	.loh AdrpAdd	Lloh168, Lloh169
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__literal16,16byte_literals
	.p2align	4, 0x0                          ; -- Begin function bench_fmla_f16x8_dep
lCPI14_0:
	.byte	142                             ; 0x8e
	.byte	6                               ; 0x6
	.byte	142                             ; 0x8e
	.byte	6                               ; 0x6
	.byte	142                             ; 0x8e
	.byte	6                               ; 0x6
	.byte	142                             ; 0x8e
	.byte	6                               ; 0x6
	.byte	142                             ; 0x8e
	.byte	6                               ; 0x6
	.byte	142                             ; 0x8e
	.byte	6                               ; 0x6
	.byte	142                             ; 0x8e
	.byte	6                               ; 0x6
	.byte	142                             ; 0x8e
	.byte	6                               ; 0x6
	.section	__TEXT,__text,regular,pure_instructions
	.p2align	2
_bench_fmla_f16x8_dep:                  ; @bench_fmla_f16x8_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #80
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
	mov	x20, x0
	adrp	x24, _apple_re_now_ns.timebase@PAGE+4
	ldr	w8, [x24, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB14_2
; %bb.1:
Lloh170:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh171:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB14_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh172:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh173:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbz	x20, LBB14_8
; %bb.3:
	movi.8h	v0, #60, lsl #8
Lloh174:
	adrp	x8, lCPI14_0@PAGE
Lloh175:
	ldr	q1, [x8, lCPI14_0@PAGEOFF]
	movi.8h	v2, #60, lsl #8
LBB14_4:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	fmla.8h	v2, v0, v1
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB14_4
; %bb.5:
	ldr	w8, [x24, _apple_re_now_ns.timebase@PAGEOFF+4]
	str	q2, [sp]                        ; 16-byte Folded Spill
	cbnz	w8, LBB14_7
LBB14_6:
Lloh176:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh177:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB14_7:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	ldr	q0, [sp]                        ; 16-byte Folded Reload
	fcvt	d0, h0
	adrp	x9, _g_f64_sink@PAGE
	str	d0, [x9, _g_f64_sink@PAGEOFF]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	add	sp, sp, #80
	ret
LBB14_8:
	movi.8h	v2, #60, lsl #8
	mov	x8, x23
	str	q2, [sp]                        ; 16-byte Folded Spill
	cbnz	w8, LBB14_7
	b	LBB14_6
	.loh AdrpAdd	Lloh170, Lloh171
	.loh AdrpAdd	Lloh172, Lloh173
	.loh AdrpLdr	Lloh174, Lloh175
	.loh AdrpAdd	Lloh176, Lloh177
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_mul_dep
_bench_mul_dep:                         ; @bench_mul_dep
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
	cbnz	w8, LBB15_2
; %bb.1:
Lloh178:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh179:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB15_2:
	mov	x21, #31765                     ; =0x7c15
	movk	x21, #32586, lsl #16
	movk	x21, #31161, lsl #32
	movk	x21, #40503, lsl #48
	bl	_mach_absolute_time
	mov	x19, x0
Lloh180:
	adrp	x22, _apple_re_now_ns.timebase@PAGE
Lloh181:
	add	x22, x22, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w23, w24, [x22]
	cbz	x20, LBB15_8
; %bb.3:
	mov	x8, #322                        ; =0x142
	movk	x8, #1979, lsl #16
	movk	x8, #10030, lsl #32
	movk	x8, #27746, lsl #48
LBB15_4:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	mul	x21, x21, x8
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB15_4
; %bb.5:
	ldr	w8, [x25, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB15_7
LBB15_6:
Lloh182:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh183:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB15_7:
	mul	x8, x19, x23
	udiv	x19, x8, x24
	bl	_mach_absolute_time
	ldp	w8, w9, [x22]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	adrp	x9, _g_u64_sink@PAGE
	str	x21, [x9, _g_u64_sink@PAGEOFF]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp], #80             ; 16-byte Folded Reload
	ret
LBB15_8:
	mov	x8, x24
	cbnz	w8, LBB15_7
	b	LBB15_6
	.loh AdrpAdd	Lloh178, Lloh179
	.loh AdrpAdd	Lloh180, Lloh181
	.loh AdrpAdd	Lloh182, Lloh183
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_madd_dep
_bench_madd_dep:                        ; @bench_madd_dep
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
	cbnz	w8, LBB16_2
; %bb.1:
Lloh184:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh185:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB16_2:
	mov	x21, #31765                     ; =0x7c15
	movk	x21, #32586, lsl #16
	movk	x21, #31161, lsl #32
	movk	x21, #40503, lsl #48
	bl	_mach_absolute_time
	mov	x19, x0
Lloh186:
	adrp	x22, _apple_re_now_ns.timebase@PAGE
Lloh187:
	add	x22, x22, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w23, w24, [x22]
	cbz	x20, LBB16_8
; %bb.3:
	mov	x8, #322                        ; =0x142
	movk	x8, #1979, lsl #16
	movk	x8, #10030, lsl #32
	movk	x8, #27746, lsl #48
LBB16_4:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	madd	x21, x21, x8, x21
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB16_4
; %bb.5:
	ldr	w8, [x25, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB16_7
LBB16_6:
Lloh188:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh189:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB16_7:
	mul	x8, x19, x23
	udiv	x19, x8, x24
	bl	_mach_absolute_time
	ldp	w8, w9, [x22]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	adrp	x9, _g_u64_sink@PAGE
	str	x21, [x9, _g_u64_sink@PAGEOFF]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp], #80             ; 16-byte Folded Reload
	ret
LBB16_8:
	mov	x8, x24
	cbnz	w8, LBB16_7
	b	LBB16_6
	.loh AdrpAdd	Lloh184, Lloh185
	.loh AdrpAdd	Lloh186, Lloh187
	.loh AdrpAdd	Lloh188, Lloh189
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_msub_dep
_bench_msub_dep:                        ; @bench_msub_dep
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
	cbnz	w8, LBB17_2
; %bb.1:
Lloh190:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh191:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB17_2:
	mov	x21, #31765                     ; =0x7c15
	movk	x21, #32586, lsl #16
	movk	x21, #31161, lsl #32
	movk	x21, #40503, lsl #48
	bl	_mach_absolute_time
	mov	x19, x0
Lloh192:
	adrp	x22, _apple_re_now_ns.timebase@PAGE
Lloh193:
	add	x22, x22, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w23, w24, [x22]
	cbz	x20, LBB17_8
; %bb.3:
	mov	w8, #3                          ; =0x3
LBB17_4:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	msub	x21, x21, x8, x21
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB17_4
; %bb.5:
	ldr	w8, [x25, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB17_7
LBB17_6:
Lloh194:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh195:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB17_7:
	mul	x8, x19, x23
	udiv	x19, x8, x24
	bl	_mach_absolute_time
	ldp	w8, w9, [x22]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	adrp	x9, _g_u64_sink@PAGE
	str	x21, [x9, _g_u64_sink@PAGEOFF]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp], #80             ; 16-byte Folded Reload
	ret
LBB17_8:
	mov	x8, x24
	cbnz	w8, LBB17_7
	b	LBB17_6
	.loh AdrpAdd	Lloh190, Lloh191
	.loh AdrpAdd	Lloh192, Lloh193
	.loh AdrpAdd	Lloh194, Lloh195
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_umulh_dep
_bench_umulh_dep:                       ; @bench_umulh_dep
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
	cbnz	w8, LBB18_2
; %bb.1:
Lloh196:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh197:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB18_2:
	mov	x21, #31765                     ; =0x7c15
	movk	x21, #32586, lsl #16
	movk	x21, #31161, lsl #32
	movk	x21, #40503, lsl #48
	bl	_mach_absolute_time
	mov	x19, x0
Lloh198:
	adrp	x22, _apple_re_now_ns.timebase@PAGE
Lloh199:
	add	x22, x22, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w23, w24, [x22]
	cbz	x20, LBB18_8
; %bb.3:
	mov	x8, #58809                      ; =0xe5b9
	movk	x8, #7396, lsl #16
	movk	x8, #18285, lsl #32
	movk	x8, #48984, lsl #48
LBB18_4:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	umulh	x21, x21, x8
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB18_4
; %bb.5:
	ldr	w8, [x25, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB18_7
LBB18_6:
Lloh200:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh201:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB18_7:
	mul	x8, x19, x23
	udiv	x19, x8, x24
	bl	_mach_absolute_time
	ldp	w8, w9, [x22]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	adrp	x9, _g_u64_sink@PAGE
	str	x21, [x9, _g_u64_sink@PAGEOFF]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp], #80             ; 16-byte Folded Reload
	ret
LBB18_8:
	mov	x8, x24
	cbnz	w8, LBB18_7
	b	LBB18_6
	.loh AdrpAdd	Lloh196, Lloh197
	.loh AdrpAdd	Lloh198, Lloh199
	.loh AdrpAdd	Lloh200, Lloh201
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_smull_dep
_bench_smull_dep:                       ; @bench_smull_dep
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
	cbnz	w8, LBB19_2
; %bb.1:
Lloh202:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh203:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB19_2:
	mov	w21, #31161                     ; =0x79b9
	movk	w21, #40503, lsl #16
	bl	_mach_absolute_time
	mov	x19, x0
Lloh204:
	adrp	x22, _apple_re_now_ns.timebase@PAGE
Lloh205:
	add	x22, x22, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w23, w24, [x22]
	cbz	x20, LBB19_8
; %bb.3:
	mov	w8, #10030                      ; =0x272e
	movk	w8, #27746, lsl #16
LBB19_4:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	smull	x21, w21, w8
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB19_4
; %bb.5:
	ldr	w8, [x25, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB19_7
LBB19_6:
Lloh206:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh207:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB19_7:
	mul	x8, x19, x23
	udiv	x19, x8, x24
	bl	_mach_absolute_time
	ldp	w8, w9, [x22]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	adrp	x9, _g_u64_sink@PAGE
	str	x21, [x9, _g_u64_sink@PAGEOFF]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp], #80             ; 16-byte Folded Reload
	ret
LBB19_8:
	mov	x8, x24
	cbnz	w8, LBB19_7
	b	LBB19_6
	.loh AdrpAdd	Lloh202, Lloh203
	.loh AdrpAdd	Lloh204, Lloh205
	.loh AdrpAdd	Lloh206, Lloh207
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
Lloh208:
	adrp	x8, ___stack_chk_guard@GOTPAGE
Lloh209:
	ldr	x8, [x8, ___stack_chk_guard@GOTPAGEOFF]
Lloh210:
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
Lloh211:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh212:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB20_2
; %bb.1:
Lloh213:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh214:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB20_2:
	mov	x23, #31765                     ; =0x7c15
	movk	x23, #32586, lsl #16
	movk	x23, #31161, lsl #32
	movk	x23, #40503, lsl #48
	bl	_mach_absolute_time
	mov	x20, x0
Lloh215:
	adrp	x24, _apple_re_now_ns.timebase@PAGE
Lloh216:
	add	x24, x24, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w21, w22, [x24]
	cbz	x19, LBB20_7
; %bb.3:
	mov	x8, #0                          ; =0x0
	add	x9, sp, #8
LBB20_4:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB20_5 Depth 2
	mov	x10, #0                         ; =0x0
LBB20_5:                                ;   Parent Loop BB20_4 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	x11, [x9, x10, lsl #3]
	add	x12, x10, x11
	eor	x12, x12, x23
	ror	x23, x12, #63
	add	x11, x23, x11
	str	x11, [x9, x10, lsl #3]
	add	x10, x10, #1
	cmp	x10, #32
	b.ne	LBB20_5
; %bb.6:                                ;   in Loop: Header=BB20_4 Depth=1
	add	x8, x8, #1
	cmp	x8, x19
	b.ne	LBB20_4
LBB20_7:
	cbnz	w22, LBB20_9
; %bb.8:
Lloh217:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh218:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB20_9:
	bl	_mach_absolute_time
	ldp	w9, w8, [x24]
	and	x10, x23, #0x1f
	add	x11, sp, #8
	ldr	x10, [x11, x10, lsl #3]
	eor	x10, x10, x23
Lloh219:
	adrp	x11, _g_u64_sink@PAGE
	str	x10, [x11, _g_u64_sink@PAGEOFF]
	ldur	x10, [x29, #-72]
Lloh220:
	adrp	x11, ___stack_chk_guard@GOTPAGE
Lloh221:
	ldr	x11, [x11, ___stack_chk_guard@GOTPAGEOFF]
Lloh222:
	ldr	x11, [x11]
	cmp	x11, x10
	b.ne	LBB20_11
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
LBB20_11:
	bl	___stack_chk_fail
	.loh AdrpLdr	Lloh211, Lloh212
	.loh AdrpLdrGotLdr	Lloh208, Lloh209, Lloh210
	.loh AdrpAdd	Lloh213, Lloh214
	.loh AdrpAdd	Lloh215, Lloh216
	.loh AdrpAdd	Lloh217, Lloh218
	.loh AdrpLdrGotLdr	Lloh220, Lloh221, Lloh222
	.loh AdrpAdrp	Lloh219, Lloh220
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
Lloh223:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh224:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB21_2
; %bb.1:
Lloh225:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh226:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB21_2:
	mov	x21, #52719                     ; =0xcdef
	movk	x21, #35243, lsl #16
	movk	x21, #17767, lsl #32
	movk	x21, #291, lsl #48
	bl	_mach_absolute_time
	mov	x20, x0
Lloh227:
	adrp	x22, _apple_re_now_ns.timebase@PAGE
Lloh228:
	add	x22, x22, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w23, w24, [x22]
	cbz	x19, LBB21_7
; %bb.3:
	mov	x8, #0                          ; =0x0
	mov	x9, #31765                      ; =0x7c15
	movk	x9, #32586, lsl #16
	movk	x9, #31161, lsl #32
	movk	x9, #40503, lsl #48
LBB21_4:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB21_5 Depth 2
	mov	x10, #0                         ; =0x0
	mov	x11, #0                         ; =0x0
LBB21_5:                                ;   Parent Loop BB21_4 Depth=1
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
	b.ne	LBB21_5
; %bb.6:                                ;   in Loop: Header=BB21_4 Depth=1
	add	x8, x8, #1
	cmp	x8, x19
	b.ne	LBB21_4
LBB21_7:
	cbnz	w24, LBB21_9
; %bb.8:
Lloh229:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh230:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB21_9:
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
	.loh AdrpLdr	Lloh223, Lloh224
	.loh AdrpAdd	Lloh225, Lloh226
	.loh AdrpAdd	Lloh227, Lloh228
	.loh AdrpAdd	Lloh229, Lloh230
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
Lloh231:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh232:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB22_2
; %bb.1:
Lloh233:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh234:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB22_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh235:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh236:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	cbz	x20, LBB22_7
; %bb.3:
Lloh237:
	adrp	x8, _g_atomic_counter@PAGE
Lloh238:
	add	x8, x8, _g_atomic_counter@PAGEOFF
	mov	w9, #1                          ; =0x1
LBB22_4:                                ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB22_4
; %bb.5:
	cbz	w23, LBB22_8
LBB22_6:
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
LBB22_7:
	mov	x24, #0                         ; =0x0
	cbnz	w23, LBB22_6
LBB22_8:
Lloh239:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh240:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
	b	LBB22_6
	.loh AdrpLdr	Lloh231, Lloh232
	.loh AdrpAdd	Lloh233, Lloh234
	.loh AdrpAdd	Lloh235, Lloh236
	.loh AdrpAdd	Lloh237, Lloh238
	.loh AdrpAdd	Lloh239, Lloh240
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
Lloh241:
	adrp	x8, _apple_re_now_ns.timebase@PAGE+4
Lloh242:
	ldr	w8, [x8, _apple_re_now_ns.timebase@PAGEOFF+4]
	cbnz	w8, LBB23_2
; %bb.1:
Lloh243:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh244:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB23_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh245:
	adrp	x21, _apple_re_now_ns.timebase@PAGE
Lloh246:
	add	x21, x21, _apple_re_now_ns.timebase@PAGEOFF
	ldp	w22, w23, [x21]
	mov	x8, #25439                      ; =0x635f
	movk	x8, #14137, lsl #16
	movk	x8, #39645, lsl #32
	movk	x8, #16319, lsl #48
	fmov	d8, x8
	cbz	x20, LBB23_5
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
LBB23_4:                                ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB23_4
LBB23_5:
	cbnz	w23, LBB23_7
; %bb.6:
Lloh247:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
Lloh248:
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
LBB23_7:
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
	.loh AdrpLdr	Lloh241, Lloh242
	.loh AdrpAdd	Lloh243, Lloh244
	.loh AdrpAdd	Lloh245, Lloh246
	.loh AdrpAdd	Lloh247, Lloh248
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
	.quad	_bench_add_dep_u32
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.12
	.quad	_bench_add_dep_i64
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.13
	.quad	_bench_add_dep_i16
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.14
	.quad	_bench_add_dep_i8
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.15
	.quad	_bench_add_dep_u16
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.16
	.quad	_bench_add_dep_u8
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.17
	.quad	_bench_fadd_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.18
	.quad	_bench_fadd_f32_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.19
	.quad	_bench_fmadd_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.20
	.quad	_bench_bfloat16_roundtrip_proxy
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.21
	.quad	_bench_fcvt_f32_to_f16_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.22
	.quad	_bench_fadd_f16_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.23
	.quad	_bench_fmla_f16x8_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.24
	.quad	_bench_mul_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.25
	.quad	_bench_madd_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.26
	.quad	_bench_msub_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.27
	.quad	_bench_umulh_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.28
	.quad	_bench_smull_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.29
	.quad	_bench_load_store_chain
	.long	96                              ; 0x60
	.space	4
	.quad	l_.str.30
	.quad	_bench_shuffle_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.31
	.quad	_bench_atomic_add_dep
	.long	32                              ; 0x20
	.space	4
	.quad	l_.str.32
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
	.asciz	"add_dep_u32"

l_.str.12:                              ; @.str.12
	.asciz	"add_dep_i64"

l_.str.13:                              ; @.str.13
	.asciz	"add_dep_i16"

l_.str.14:                              ; @.str.14
	.asciz	"add_dep_i8"

l_.str.15:                              ; @.str.15
	.asciz	"add_dep_u16"

l_.str.16:                              ; @.str.16
	.asciz	"add_dep_u8"

l_.str.17:                              ; @.str.17
	.asciz	"fadd_dep_f64"

l_.str.18:                              ; @.str.18
	.asciz	"fadd_dep_f32"

l_.str.19:                              ; @.str.19
	.asciz	"fmadd_dep_f64"

l_.str.20:                              ; @.str.20
	.asciz	"bf16_f32_roundtrip_proxy"

l_.str.21:                              ; @.str.21
	.asciz	"fcvt_f32_f16_roundtrip"

l_.str.22:                              ; @.str.22
	.asciz	"fadd_dep_f16_scalar"

l_.str.23:                              ; @.str.23
	.asciz	"fmla_dep_f16x8_simd"

l_.str.24:                              ; @.str.24
	.asciz	"mul_dep_u64"

l_.str.25:                              ; @.str.25
	.asciz	"madd_dep_u64"

l_.str.26:                              ; @.str.26
	.asciz	"msub_dep_u64"

l_.str.27:                              ; @.str.27
	.asciz	"umulh_dep_u64"

l_.str.28:                              ; @.str.28
	.asciz	"smull_dep_i32_to_i64"

l_.str.29:                              ; @.str.29
	.asciz	"load_store_chain_u64"

l_.str.30:                              ; @.str.30
	.asciz	"shuffle_lane_cross_u64"

l_.str.31:                              ; @.str.31
	.asciz	"atomic_add_relaxed_u64"

l_.str.32:                              ; @.str.32
	.asciz	"transcendental_sin_cos_f64"

.zerofill __DATA,__bss,_g_u64_sink,8,3  ; @g_u64_sink
.zerofill __DATA,__bss,_apple_re_now_ns.timebase,8,2 ; @apple_re_now_ns.timebase
.zerofill __DATA,__bss,_g_f64_sink,8,3  ; @g_f64_sink
	.section	__DATA,__data
	.p2align	3, 0x0                          ; @g_atomic_counter
_g_atomic_counter:
	.quad	1                               ; 0x1

	.section	__TEXT,__cstring,cstring_literals
l_.str.33:                              ; @.str.33
	.asciz	"%s,%llu,%u,%llu,%.6f,%.6f\n"

l_.str.34:                              ; @.str.34
	.asciz	"%-24s elapsed_ns=%llu ns/op=%.6f approx_Gops=%.6f\n"

l_str:                                  ; @str
	.asciz	"probe,iters,unroll,elapsed_ns,ns_per_op,approx_gops"

.subsections_via_symbols

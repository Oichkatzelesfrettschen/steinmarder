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
	mov	w19, #16960                     ; =0x4240
	movk	w19, #15, lsl #16
	cmp	w0, #2
	b.lt	LBB0_9
; %bb.1:
	mov	x21, x1
	mov	x22, x0
	mov	x20, #0                         ; =0x0
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
	mov	x19, x0
LBB0_3:                                 ;   in Loop: Header=BB0_4 Depth=1
	add	w28, w25, #1
	cmp	w28, w22
	b.ge	LBB0_18
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
	cbnz	w0, LBB0_32
; %bb.7:                                ;   in Loop: Header=BB0_4 Depth=1
	mov	w27, #1                         ; =0x1
	mov	x25, x28
	b	LBB0_3
LBB0_8:                                 ;   in Loop: Header=BB0_4 Depth=1
	ldr	x20, [x21, x25, lsl #3]
	b	LBB0_3
LBB0_9:
	mov	w27, #0                         ; =0x0
	mov	x8, #145685290680320            ; =0x848000000000
	movk	x8, #16686, lsl #48
	fmov	d8, x8
LBB0_10:
Lloh6:
	adrp	x22, _g_probes@PAGE+16
Lloh7:
	add	x22, x22, _g_probes@PAGEOFF+16
	mov	w23, #23                        ; =0x17
Lloh8:
	adrp	x20, l_.str.33@PAGE
Lloh9:
	add	x20, x20, l_.str.33@PAGEOFF
Lloh10:
	adrp	x21, l_.str.34@PAGE
Lloh11:
	add	x21, x21, l_.str.34@PAGEOFF
	b	LBB0_12
LBB0_11:                                ;   in Loop: Header=BB0_12 Depth=1
	stp	d1, d0, [sp, #32]
	stp	x8, x0, [sp, #16]
	stp	x24, x19, [sp]
	mov	x0, x20
	bl	_printf
	add	x22, x22, #24
	subs	x23, x23, #1
	b.eq	LBB0_40
LBB0_12:                                ; =>This Inner Loop Header: Depth=1
	ldp	x24, x8, [x22, #-16]
	mov	x0, x19
	blr	x8
	ldr	w8, [x22]
	ucvtf	d0, w8
	fmul	d2, d8, d0
	movi	d0, #0000000000000000
	fcmp	d2, #0.0
	ucvtf	d3, x0
	movi	d1, #0000000000000000
	b.le	LBB0_14
; %bb.13:                               ;   in Loop: Header=BB0_12 Depth=1
	fdiv	d1, d3, d2
LBB0_14:                                ;   in Loop: Header=BB0_12 Depth=1
	cbz	x0, LBB0_16
; %bb.15:                               ;   in Loop: Header=BB0_12 Depth=1
	fdiv	d0, d2, d3
LBB0_16:                                ;   in Loop: Header=BB0_12 Depth=1
	tbnz	w27, #0, LBB0_11
; %bb.17:                               ;   in Loop: Header=BB0_12 Depth=1
	stp	d1, d0, [sp, #16]
	stp	x24, x0, [sp]
	mov	x0, x21
	bl	_printf
	add	x22, x22, #24
	subs	x23, x23, #1
	b.ne	LBB0_12
	b	LBB0_40
LBB0_18:
	tbz	w27, #0, LBB0_20
; %bb.19:
Lloh12:
	adrp	x0, l_str@PAGE
Lloh13:
	add	x0, x0, l_str@PAGEOFF
	bl	_puts
LBB0_20:
	ucvtf	d8, x19
	cbz	x20, LBB0_10
; %bb.21:
	mov	w26, #0                         ; =0x0
Lloh14:
	adrp	x24, _g_probes@PAGE+16
Lloh15:
	add	x24, x24, _g_probes@PAGEOFF+16
	mov	w25, #23                        ; =0x17
Lloh16:
	adrp	x21, l_.str.33@PAGE
Lloh17:
	add	x21, x21, l_.str.33@PAGEOFF
Lloh18:
	adrp	x22, l_.str.34@PAGE
Lloh19:
	add	x22, x22, l_.str.34@PAGEOFF
	b	LBB0_25
LBB0_22:                                ;   in Loop: Header=BB0_25 Depth=1
	stp	d1, d0, [sp, #16]
	stp	x23, x0, [sp]
	mov	x0, x22
LBB0_23:                                ;   in Loop: Header=BB0_25 Depth=1
	bl	_printf
	mov	w26, #1                         ; =0x1
LBB0_24:                                ;   in Loop: Header=BB0_25 Depth=1
	add	x24, x24, #24
	subs	x25, x25, #1
	b.eq	LBB0_35
LBB0_25:                                ; =>This Inner Loop Header: Depth=1
	ldur	x23, [x24, #-16]
	mov	x0, x20
	mov	x1, x23
	bl	_strcmp
	cbnz	w0, LBB0_24
; %bb.26:                               ;   in Loop: Header=BB0_25 Depth=1
	ldur	x8, [x24, #-8]
	mov	x0, x19
	blr	x8
	ldr	w8, [x24]
	ucvtf	d0, w8
	fmul	d2, d8, d0
	movi	d0, #0000000000000000
	fcmp	d2, #0.0
	ucvtf	d3, x0
	movi	d1, #0000000000000000
	b.le	LBB0_28
; %bb.27:                               ;   in Loop: Header=BB0_25 Depth=1
	fdiv	d1, d3, d2
LBB0_28:                                ;   in Loop: Header=BB0_25 Depth=1
	cbz	x0, LBB0_30
; %bb.29:                               ;   in Loop: Header=BB0_25 Depth=1
	fdiv	d0, d2, d3
LBB0_30:                                ;   in Loop: Header=BB0_25 Depth=1
	tbz	w27, #0, LBB0_22
; %bb.31:                               ;   in Loop: Header=BB0_25 Depth=1
	stp	d1, d0, [sp, #32]
	stp	x8, x0, [sp, #16]
	stp	x23, x19, [sp]
	mov	x0, x21
	b	LBB0_23
LBB0_32:
Lloh20:
	adrp	x1, l_.str.3@PAGE
Lloh21:
	add	x1, x1, l_.str.3@PAGEOFF
	mov	x0, x26
	bl	_strcmp
	cbz	w0, LBB0_38
; %bb.33:
Lloh22:
	adrp	x1, l_.str.4@PAGE
Lloh23:
	add	x1, x1, l_.str.4@PAGEOFF
	mov	x0, x26
	bl	_strcmp
	cbz	w0, LBB0_39
; %bb.34:
Lloh24:
	adrp	x8, ___stderrp@GOTPAGE
Lloh25:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh26:
	ldr	x0, [x8]
	str	x26, [sp]
Lloh27:
	adrp	x1, l_.str.6@PAGE
Lloh28:
	add	x1, x1, l_.str.6@PAGEOFF
	b	LBB0_37
LBB0_35:
	tbnz	w26, #0, LBB0_40
; %bb.36:
Lloh29:
	adrp	x8, ___stderrp@GOTPAGE
Lloh30:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh31:
	ldr	x0, [x8]
	str	x20, [sp]
Lloh32:
	adrp	x1, l_.str.8@PAGE
Lloh33:
	add	x1, x1, l_.str.8@PAGEOFF
LBB0_37:
	bl	_fprintf
	mov	w0, #1                          ; =0x1
	b	LBB0_41
LBB0_38:
Lloh34:
	adrp	x0, l_.str.10@PAGE
Lloh35:
	add	x0, x0, l_.str.10@PAGEOFF
	bl	_puts
Lloh36:
	adrp	x0, l_.str.11@PAGE
Lloh37:
	add	x0, x0, l_.str.11@PAGEOFF
	bl	_puts
Lloh38:
	adrp	x0, l_.str.12@PAGE
Lloh39:
	add	x0, x0, l_.str.12@PAGEOFF
	bl	_puts
Lloh40:
	adrp	x0, l_.str.13@PAGE
Lloh41:
	add	x0, x0, l_.str.13@PAGEOFF
	bl	_puts
Lloh42:
	adrp	x0, l_.str.14@PAGE
Lloh43:
	add	x0, x0, l_.str.14@PAGEOFF
	bl	_puts
Lloh44:
	adrp	x0, l_.str.15@PAGE
Lloh45:
	add	x0, x0, l_.str.15@PAGEOFF
	bl	_puts
Lloh46:
	adrp	x0, l_.str.16@PAGE
Lloh47:
	add	x0, x0, l_.str.16@PAGEOFF
	bl	_puts
Lloh48:
	adrp	x0, l_.str.17@PAGE
Lloh49:
	add	x0, x0, l_.str.17@PAGEOFF
	bl	_puts
Lloh50:
	adrp	x0, l_.str.18@PAGE
Lloh51:
	add	x0, x0, l_.str.18@PAGEOFF
	bl	_puts
Lloh52:
	adrp	x0, l_.str.19@PAGE
Lloh53:
	add	x0, x0, l_.str.19@PAGEOFF
	bl	_puts
Lloh54:
	adrp	x0, l_.str.20@PAGE
Lloh55:
	add	x0, x0, l_.str.20@PAGEOFF
	bl	_puts
Lloh56:
	adrp	x0, l_.str.21@PAGE
Lloh57:
	add	x0, x0, l_.str.21@PAGEOFF
	bl	_puts
Lloh58:
	adrp	x0, l_.str.22@PAGE
Lloh59:
	add	x0, x0, l_.str.22@PAGEOFF
	bl	_puts
Lloh60:
	adrp	x0, l_.str.23@PAGE
Lloh61:
	add	x0, x0, l_.str.23@PAGEOFF
	bl	_puts
Lloh62:
	adrp	x0, l_.str.24@PAGE
Lloh63:
	add	x0, x0, l_.str.24@PAGEOFF
	bl	_puts
Lloh64:
	adrp	x0, l_.str.25@PAGE
Lloh65:
	add	x0, x0, l_.str.25@PAGEOFF
	bl	_puts
Lloh66:
	adrp	x0, l_.str.26@PAGE
Lloh67:
	add	x0, x0, l_.str.26@PAGEOFF
	bl	_puts
Lloh68:
	adrp	x0, l_.str.27@PAGE
Lloh69:
	add	x0, x0, l_.str.27@PAGEOFF
	bl	_puts
Lloh70:
	adrp	x0, l_.str.28@PAGE
Lloh71:
	add	x0, x0, l_.str.28@PAGEOFF
	bl	_puts
Lloh72:
	adrp	x0, l_.str.29@PAGE
Lloh73:
	add	x0, x0, l_.str.29@PAGEOFF
	bl	_puts
Lloh74:
	adrp	x0, l_.str.30@PAGE
Lloh75:
	add	x0, x0, l_.str.30@PAGEOFF
	bl	_puts
Lloh76:
	adrp	x0, l_.str.31@PAGE
Lloh77:
	add	x0, x0, l_.str.31@PAGEOFF
	bl	_puts
Lloh78:
	adrp	x0, l_.str.32@PAGE
Lloh79:
	add	x0, x0, l_.str.32@PAGEOFF
	bl	_puts
	b	LBB0_40
LBB0_39:
	ldr	x8, [x21]
	str	x8, [sp]
Lloh80:
	adrp	x0, l_.str.5@PAGE
Lloh81:
	add	x0, x0, l_.str.5@PAGEOFF
	bl	_printf
LBB0_40:
	mov	w0, #0                          ; =0x0
LBB0_41:
	ldp	x29, x30, [sp, #144]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #128]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #112]            ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #96]             ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #80]             ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #64]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #48]               ; 16-byte Folded Reload
	add	sp, sp, #160
	ret
	.loh AdrpAdd	Lloh2, Lloh3
	.loh AdrpAdd	Lloh0, Lloh1
	.loh AdrpAdd	Lloh4, Lloh5
	.loh AdrpAdd	Lloh10, Lloh11
	.loh AdrpAdd	Lloh8, Lloh9
	.loh AdrpAdd	Lloh6, Lloh7
	.loh AdrpAdd	Lloh12, Lloh13
	.loh AdrpAdd	Lloh18, Lloh19
	.loh AdrpAdd	Lloh16, Lloh17
	.loh AdrpAdd	Lloh14, Lloh15
	.loh AdrpAdd	Lloh20, Lloh21
	.loh AdrpAdd	Lloh22, Lloh23
	.loh AdrpAdd	Lloh27, Lloh28
	.loh AdrpLdrGotLdr	Lloh24, Lloh25, Lloh26
	.loh AdrpAdd	Lloh32, Lloh33
	.loh AdrpLdrGotLdr	Lloh29, Lloh30, Lloh31
	.loh AdrpAdd	Lloh78, Lloh79
	.loh AdrpAdd	Lloh76, Lloh77
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
	.loh AdrpAdd	Lloh80, Lloh81
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep
_bench_add_dep:                         ; @bench_add_dep
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
	adrp	x24, __MergedGlobals@PAGE+12
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB1_2
; %bb.1:
Lloh82:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh83:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB1_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh84:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh85:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x8]
	mov	w21, #1                         ; =0x1
	cbz	x20, LBB1_7
LBB1_3:                                 ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	add	x21, x21, #1
	; InlineAsm End
	subs	x20, x20, #1
	b.ne	LBB1_3
; %bb.4:
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB1_6
LBB1_5:
Lloh86:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh87:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB1_6:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
Lloh88:
	adrp	x8, __MergedGlobals@PAGE
Lloh89:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x21, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB1_7:
	mov	x8, x23
	cbnz	w8, LBB1_6
	b	LBB1_5
	.loh AdrpAdd	Lloh82, Lloh83
	.loh AdrpAdd	Lloh84, Lloh85
	.loh AdrpAdd	Lloh86, Lloh87
	.loh AdrpAdd	Lloh88, Lloh89
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
Lloh90:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh91:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB2_3
; %bb.1:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh92:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh93:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbz	w22, LBB2_4
LBB2_2:
	mov	w23, #1                         ; =0x1
	bfi	x23, x19, #5, #27
	mul	x8, x20, x21
	udiv	x19, x8, x22
	bl	_mach_absolute_time
Lloh94:
	adrp	x8, __MergedGlobals@PAGE
Lloh95:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x23, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB2_3:
Lloh96:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh97:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x20, x0
Lloh98:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh99:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbnz	w22, LBB2_2
LBB2_4:
Lloh100:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh101:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	b	LBB2_2
	.loh AdrpLdr	Lloh90, Lloh91
	.loh AdrpAdd	Lloh92, Lloh93
	.loh AdrpAdd	Lloh94, Lloh95
	.loh AdrpAdd	Lloh98, Lloh99
	.loh AdrpAdd	Lloh96, Lloh97
	.loh AdrpAdd	Lloh100, Lloh101
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep_i64
_bench_add_dep_i64:                     ; @bench_add_dep_i64
	.cfi_startproc
; %bb.0:
	stp	x22, x21, [sp, #-48]!           ; 16-byte Folded Spill
	stp	x20, x19, [sp, #16]             ; 16-byte Folded Spill
	stp	x29, x30, [sp, #32]             ; 16-byte Folded Spill
	add	x29, sp, #32
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	mov	x19, x0
Lloh102:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh103:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB3_3
; %bb.1:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh104:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh105:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbz	w22, LBB3_4
LBB3_2:
	mov	w8, #1                          ; =0x1
	orr	x19, x8, x19, lsl #5
	mul	x8, x20, x21
	udiv	x20, x8, x22
	bl	_mach_absolute_time
Lloh106:
	adrp	x8, __MergedGlobals@PAGE
Lloh107:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x19, [x8]
	sub	x0, x9, x20
	ldp	x29, x30, [sp, #32]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #16]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp], #48             ; 16-byte Folded Reload
	ret
LBB3_3:
Lloh108:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh109:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x20, x0
Lloh110:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh111:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbnz	w22, LBB3_2
LBB3_4:
Lloh112:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh113:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	b	LBB3_2
	.loh AdrpLdr	Lloh102, Lloh103
	.loh AdrpAdd	Lloh104, Lloh105
	.loh AdrpAdd	Lloh106, Lloh107
	.loh AdrpAdd	Lloh110, Lloh111
	.loh AdrpAdd	Lloh108, Lloh109
	.loh AdrpAdd	Lloh112, Lloh113
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
Lloh114:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh115:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB4_3
; %bb.1:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh116:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh117:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbz	w22, LBB4_4
LBB4_2:
	mov	w23, #1                         ; =0x1
	bfi	x23, x19, #5, #11
	mul	x8, x20, x21
	udiv	x19, x8, x22
	bl	_mach_absolute_time
Lloh118:
	adrp	x8, __MergedGlobals@PAGE
Lloh119:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x23, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB4_3:
Lloh120:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh121:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x20, x0
Lloh122:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh123:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbnz	w22, LBB4_2
LBB4_4:
Lloh124:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh125:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	b	LBB4_2
	.loh AdrpLdr	Lloh114, Lloh115
	.loh AdrpAdd	Lloh116, Lloh117
	.loh AdrpAdd	Lloh118, Lloh119
	.loh AdrpAdd	Lloh122, Lloh123
	.loh AdrpAdd	Lloh120, Lloh121
	.loh AdrpAdd	Lloh124, Lloh125
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
Lloh126:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh127:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB5_3
; %bb.1:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh128:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh129:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbz	w22, LBB5_4
LBB5_2:
	mov	w23, #1                         ; =0x1
	bfi	x23, x19, #5, #3
	mul	x8, x20, x21
	udiv	x19, x8, x22
	bl	_mach_absolute_time
Lloh130:
	adrp	x8, __MergedGlobals@PAGE
Lloh131:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x23, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB5_3:
Lloh132:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh133:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x20, x0
Lloh134:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh135:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbnz	w22, LBB5_2
LBB5_4:
Lloh136:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh137:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	b	LBB5_2
	.loh AdrpLdr	Lloh126, Lloh127
	.loh AdrpAdd	Lloh128, Lloh129
	.loh AdrpAdd	Lloh130, Lloh131
	.loh AdrpAdd	Lloh134, Lloh135
	.loh AdrpAdd	Lloh132, Lloh133
	.loh AdrpAdd	Lloh136, Lloh137
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
Lloh138:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh139:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB6_3
; %bb.1:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh140:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh141:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbz	w22, LBB6_4
LBB6_2:
	mov	w23, #1                         ; =0x1
	bfi	x23, x19, #5, #11
	mul	x8, x20, x21
	udiv	x19, x8, x22
	bl	_mach_absolute_time
Lloh142:
	adrp	x8, __MergedGlobals@PAGE
Lloh143:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x23, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB6_3:
Lloh144:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh145:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x20, x0
Lloh146:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh147:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbnz	w22, LBB6_2
LBB6_4:
Lloh148:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh149:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	b	LBB6_2
	.loh AdrpLdr	Lloh138, Lloh139
	.loh AdrpAdd	Lloh140, Lloh141
	.loh AdrpAdd	Lloh142, Lloh143
	.loh AdrpAdd	Lloh146, Lloh147
	.loh AdrpAdd	Lloh144, Lloh145
	.loh AdrpAdd	Lloh148, Lloh149
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
Lloh150:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh151:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB7_3
; %bb.1:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh152:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh153:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbz	w22, LBB7_4
LBB7_2:
	mov	w23, #1                         ; =0x1
	bfi	x23, x19, #5, #3
	mul	x8, x20, x21
	udiv	x19, x8, x22
	bl	_mach_absolute_time
Lloh154:
	adrp	x8, __MergedGlobals@PAGE
Lloh155:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x23, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB7_3:
Lloh156:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh157:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x20, x0
Lloh158:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh159:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbnz	w22, LBB7_2
LBB7_4:
Lloh160:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh161:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	b	LBB7_2
	.loh AdrpLdr	Lloh150, Lloh151
	.loh AdrpAdd	Lloh152, Lloh153
	.loh AdrpAdd	Lloh154, Lloh155
	.loh AdrpAdd	Lloh158, Lloh159
	.loh AdrpAdd	Lloh156, Lloh157
	.loh AdrpAdd	Lloh160, Lloh161
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
	adrp	x24, __MergedGlobals@PAGE+12
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB8_7
; %bb.1:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh162:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh163:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	cbz	x20, LBB8_8
LBB8_2:
	fmov	d0, #1.00000000
	fmov	d8, #1.00000000
LBB8_3:                                 ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB8_3
; %bb.4:
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB8_6
LBB8_5:
Lloh164:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh165:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB8_6:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	str	d8, [x21, #8]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp], #80               ; 16-byte Folded Reload
	ret
LBB8_7:
Lloh166:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh167:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x19, x0
Lloh168:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh169:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	cbnz	x20, LBB8_2
LBB8_8:
	fmov	d8, #1.00000000
	mov	x8, x23
	cbnz	w8, LBB8_6
	b	LBB8_5
	.loh AdrpAdd	Lloh162, Lloh163
	.loh AdrpAdd	Lloh164, Lloh165
	.loh AdrpAdd	Lloh168, Lloh169
	.loh AdrpAdd	Lloh166, Lloh167
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
	adrp	x24, __MergedGlobals@PAGE+12
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB9_7
; %bb.1:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh170:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh171:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	cbz	x20, LBB9_8
LBB9_2:
	fmov	s1, #1.00000000
	fmov	s0, #1.00000000
LBB9_3:                                 ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB9_3
; %bb.4:
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	fcvt	d8, s0
	cbnz	w8, LBB9_6
LBB9_5:
Lloh172:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh173:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB9_6:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	str	d8, [x21, #8]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp], #80               ; 16-byte Folded Reload
	ret
LBB9_7:
Lloh174:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh175:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x19, x0
Lloh176:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh177:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	cbnz	x20, LBB9_2
LBB9_8:
	fmov	d8, #1.00000000
	mov	x8, x23
	cbnz	w8, LBB9_6
	b	LBB9_5
	.loh AdrpAdd	Lloh170, Lloh171
	.loh AdrpAdd	Lloh172, Lloh173
	.loh AdrpAdd	Lloh176, Lloh177
	.loh AdrpAdd	Lloh174, Lloh175
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
	adrp	x24, __MergedGlobals@PAGE+12
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB10_2
; %bb.1:
Lloh178:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh179:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB10_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh180:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh181:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
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
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB10_7
LBB10_6:
Lloh182:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh183:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB10_7:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	str	d8, [x21, #8]
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
	.loh AdrpAdd	Lloh178, Lloh179
	.loh AdrpAdd	Lloh180, Lloh181
	.loh AdrpAdd	Lloh182, Lloh183
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
Lloh184:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh185:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB11_7
; %bb.1:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh186:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh187:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	cbz	x20, LBB11_8
LBB11_2:
	fmov	s1, #1.00000000
	fmov	s0, #0.12500000
LBB11_3:                                ; =>This Inner Loop Header: Depth=1
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	fadd	s1, s1, s0
	fmov	w8, s1
	add	w8, w8, #8, lsl #12             ; =32768
	and	w8, w8, #0xffff0000
	fmov	s1, w8
	subs	x20, x20, #1
	b.ne	LBB11_3
; %bb.4:
	fcvt	d8, s1
	cbnz	w23, LBB11_6
LBB11_5:
Lloh188:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh189:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB11_6:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	str	d8, [x21, #8]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp], #80               ; 16-byte Folded Reload
	ret
LBB11_7:
Lloh190:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh191:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x19, x0
Lloh192:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh193:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	cbnz	x20, LBB11_2
LBB11_8:
	fmov	d8, #1.00000000
	cbnz	w23, LBB11_6
	b	LBB11_5
	.loh AdrpLdr	Lloh184, Lloh185
	.loh AdrpAdd	Lloh186, Lloh187
	.loh AdrpAdd	Lloh188, Lloh189
	.loh AdrpAdd	Lloh192, Lloh193
	.loh AdrpAdd	Lloh190, Lloh191
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
	adrp	x24, __MergedGlobals@PAGE+12
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB12_7
; %bb.1:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh194:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh195:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	cbz	x20, LBB12_8
LBB12_2:
	fmov	s0, #1.00000000
LBB12_3:                                ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB12_3
; %bb.4:
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	fcvt	d8, s0
	cbnz	w8, LBB12_6
LBB12_5:
Lloh196:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh197:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB12_6:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	str	d8, [x21, #8]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp], #80               ; 16-byte Folded Reload
	ret
LBB12_7:
Lloh198:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh199:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x19, x0
Lloh200:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh201:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	cbnz	x20, LBB12_2
LBB12_8:
	fmov	d8, #1.00000000
	mov	x8, x23
	cbnz	w8, LBB12_6
	b	LBB12_5
	.loh AdrpAdd	Lloh194, Lloh195
	.loh AdrpAdd	Lloh196, Lloh197
	.loh AdrpAdd	Lloh200, Lloh201
	.loh AdrpAdd	Lloh198, Lloh199
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
	adrp	x24, __MergedGlobals@PAGE+12
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB13_7
; %bb.1:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh202:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh203:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	cbz	x20, LBB13_8
LBB13_2:
	fmov	h0, #1.00000000
	mov	w8, #5145                       ; =0x1419
	fmov	h1, w8
LBB13_3:                                ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB13_3
; %bb.4:
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	fcvt	d8, h0
	cbnz	w8, LBB13_6
LBB13_5:
Lloh204:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh205:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB13_6:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	str	d8, [x21, #8]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp], #80               ; 16-byte Folded Reload
	ret
LBB13_7:
Lloh206:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh207:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x19, x0
Lloh208:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh209:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	cbnz	x20, LBB13_2
LBB13_8:
	fmov	d8, #1.00000000
	mov	x8, x23
	cbnz	w8, LBB13_6
	b	LBB13_5
	.loh AdrpAdd	Lloh202, Lloh203
	.loh AdrpAdd	Lloh204, Lloh205
	.loh AdrpAdd	Lloh208, Lloh209
	.loh AdrpAdd	Lloh206, Lloh207
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
	adrp	x24, __MergedGlobals@PAGE+12
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB14_7
; %bb.1:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh210:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh211:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	cbz	x20, LBB14_8
LBB14_2:
	movi.8h	v0, #60, lsl #8
Lloh212:
	adrp	x8, lCPI14_0@PAGE
Lloh213:
	ldr	q1, [x8, lCPI14_0@PAGEOFF]
	movi.8h	v2, #60, lsl #8
LBB14_3:                                ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB14_3
; %bb.4:
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	str	q2, [sp]                        ; 16-byte Folded Spill
	cbnz	w8, LBB14_6
LBB14_5:
Lloh214:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh215:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB14_6:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	ldr	q0, [sp]                        ; 16-byte Folded Reload
	fcvt	d0, h0
	str	d0, [x21, #8]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #48]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #32]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #16]             ; 16-byte Folded Reload
	add	sp, sp, #80
	ret
LBB14_7:
Lloh216:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh217:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x19, x0
Lloh218:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh219:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	cbnz	x20, LBB14_2
LBB14_8:
	movi.8h	v2, #60, lsl #8
	mov	x8, x23
	str	q2, [sp]                        ; 16-byte Folded Spill
	cbnz	w8, LBB14_6
	b	LBB14_5
	.loh AdrpAdd	Lloh210, Lloh211
	.loh AdrpLdr	Lloh212, Lloh213
	.loh AdrpAdd	Lloh214, Lloh215
	.loh AdrpAdd	Lloh218, Lloh219
	.loh AdrpAdd	Lloh216, Lloh217
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_mul_dep
_bench_mul_dep:                         ; @bench_mul_dep
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
	adrp	x24, __MergedGlobals@PAGE+12
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB15_2
; %bb.1:
Lloh220:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh221:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB15_2:
	mov	x21, #31765                     ; =0x7c15
	movk	x21, #32586, lsl #16
	movk	x21, #31161, lsl #32
	movk	x21, #40503, lsl #48
	bl	_mach_absolute_time
	mov	x19, x0
Lloh222:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh223:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x8]
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
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB15_7
LBB15_6:
Lloh224:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh225:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB15_7:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
Lloh226:
	adrp	x8, __MergedGlobals@PAGE
Lloh227:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x21, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB15_8:
	mov	x8, x23
	cbnz	w8, LBB15_7
	b	LBB15_6
	.loh AdrpAdd	Lloh220, Lloh221
	.loh AdrpAdd	Lloh222, Lloh223
	.loh AdrpAdd	Lloh224, Lloh225
	.loh AdrpAdd	Lloh226, Lloh227
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_madd_dep
_bench_madd_dep:                        ; @bench_madd_dep
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
	adrp	x24, __MergedGlobals@PAGE+12
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB16_2
; %bb.1:
Lloh228:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh229:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB16_2:
	mov	x21, #31765                     ; =0x7c15
	movk	x21, #32586, lsl #16
	movk	x21, #31161, lsl #32
	movk	x21, #40503, lsl #48
	bl	_mach_absolute_time
	mov	x19, x0
Lloh230:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh231:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x8]
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
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB16_7
LBB16_6:
Lloh232:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh233:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB16_7:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
Lloh234:
	adrp	x8, __MergedGlobals@PAGE
Lloh235:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x21, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB16_8:
	mov	x8, x23
	cbnz	w8, LBB16_7
	b	LBB16_6
	.loh AdrpAdd	Lloh228, Lloh229
	.loh AdrpAdd	Lloh230, Lloh231
	.loh AdrpAdd	Lloh232, Lloh233
	.loh AdrpAdd	Lloh234, Lloh235
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_msub_dep
_bench_msub_dep:                        ; @bench_msub_dep
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
	adrp	x24, __MergedGlobals@PAGE+12
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB17_2
; %bb.1:
Lloh236:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh237:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB17_2:
	mov	x21, #31765                     ; =0x7c15
	movk	x21, #32586, lsl #16
	movk	x21, #31161, lsl #32
	movk	x21, #40503, lsl #48
	bl	_mach_absolute_time
	mov	x19, x0
Lloh238:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh239:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x8]
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
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB17_7
LBB17_6:
Lloh240:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh241:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB17_7:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
Lloh242:
	adrp	x8, __MergedGlobals@PAGE
Lloh243:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x21, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB17_8:
	mov	x8, x23
	cbnz	w8, LBB17_7
	b	LBB17_6
	.loh AdrpAdd	Lloh236, Lloh237
	.loh AdrpAdd	Lloh238, Lloh239
	.loh AdrpAdd	Lloh240, Lloh241
	.loh AdrpAdd	Lloh242, Lloh243
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_umulh_dep
_bench_umulh_dep:                       ; @bench_umulh_dep
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
	adrp	x24, __MergedGlobals@PAGE+12
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB18_2
; %bb.1:
Lloh244:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh245:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB18_2:
	mov	x21, #31765                     ; =0x7c15
	movk	x21, #32586, lsl #16
	movk	x21, #31161, lsl #32
	movk	x21, #40503, lsl #48
	bl	_mach_absolute_time
	mov	x19, x0
Lloh246:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh247:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x8]
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
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB18_7
LBB18_6:
Lloh248:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh249:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB18_7:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
Lloh250:
	adrp	x8, __MergedGlobals@PAGE
Lloh251:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x21, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB18_8:
	mov	x8, x23
	cbnz	w8, LBB18_7
	b	LBB18_6
	.loh AdrpAdd	Lloh244, Lloh245
	.loh AdrpAdd	Lloh246, Lloh247
	.loh AdrpAdd	Lloh248, Lloh249
	.loh AdrpAdd	Lloh250, Lloh251
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_smull_dep
_bench_smull_dep:                       ; @bench_smull_dep
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
	adrp	x24, __MergedGlobals@PAGE+12
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB19_2
; %bb.1:
Lloh252:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh253:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB19_2:
	mov	w21, #31161                     ; =0x79b9
	movk	w21, #40503, lsl #16
	bl	_mach_absolute_time
	mov	x19, x0
Lloh254:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh255:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x8]
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
	ldr	w8, [x24, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB19_7
LBB19_6:
Lloh256:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh257:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB19_7:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
Lloh258:
	adrp	x8, __MergedGlobals@PAGE
Lloh259:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x21, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB19_8:
	mov	x8, x23
	cbnz	w8, LBB19_7
	b	LBB19_6
	.loh AdrpAdd	Lloh252, Lloh253
	.loh AdrpAdd	Lloh254, Lloh255
	.loh AdrpAdd	Lloh256, Lloh257
	.loh AdrpAdd	Lloh258, Lloh259
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_load_store_chain
_bench_load_store_chain:                ; @bench_load_store_chain
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #336
	stp	x24, x23, [sp, #272]            ; 16-byte Folded Spill
	stp	x22, x21, [sp, #288]            ; 16-byte Folded Spill
	stp	x20, x19, [sp, #304]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #320]            ; 16-byte Folded Spill
	add	x29, sp, #320
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
Lloh260:
	adrp	x8, ___stack_chk_guard@GOTPAGE
Lloh261:
	ldr	x8, [x8, ___stack_chk_guard@GOTPAGEOFF]
Lloh262:
	ldr	x8, [x8]
	stur	x8, [x29, #-56]
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
Lloh263:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh264:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB20_2
; %bb.1:
Lloh265:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh266:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB20_2:
	mov	x23, #31765                     ; =0x7c15
	movk	x23, #32586, lsl #16
	movk	x23, #31161, lsl #32
	movk	x23, #40503, lsl #48
	bl	_mach_absolute_time
	mov	x20, x0
Lloh267:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh268:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbz	x19, LBB20_4
LBB20_3:                                ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	eor	x9, x8, x23
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #8]
	ldr	x8, [sp, #16]
	add	x10, x8, #1
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #16]
	ldr	x8, [sp, #24]
	add	x10, x8, #2
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #24]
	ldr	x8, [sp, #32]
	add	x10, x8, #3
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #32]
	ldr	x8, [sp, #40]
	add	x10, x8, #4
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #40]
	ldr	x8, [sp, #48]
	add	x10, x8, #5
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #48]
	ldr	x8, [sp, #56]
	add	x10, x8, #6
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #56]
	ldr	x8, [sp, #64]
	add	x10, x8, #7
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #64]
	ldr	x8, [sp, #72]
	add	x10, x8, #8
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #72]
	ldr	x8, [sp, #80]
	add	x10, x8, #9
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #80]
	ldr	x8, [sp, #88]
	add	x10, x8, #10
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #88]
	ldr	x8, [sp, #96]
	add	x10, x8, #11
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #96]
	ldr	x8, [sp, #104]
	add	x10, x8, #12
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #104]
	ldr	x8, [sp, #112]
	add	x10, x8, #13
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #112]
	ldr	x8, [sp, #120]
	add	x10, x8, #14
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #120]
	ldr	x8, [sp, #128]
	add	x10, x8, #15
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #128]
	ldr	x8, [sp, #136]
	add	x10, x8, #16
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #136]
	ldr	x8, [sp, #144]
	add	x10, x8, #17
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #144]
	ldr	x8, [sp, #152]
	add	x10, x8, #18
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #152]
	ldr	x8, [sp, #160]
	add	x10, x8, #19
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #160]
	ldr	x8, [sp, #168]
	add	x10, x8, #20
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #168]
	ldr	x8, [sp, #176]
	add	x10, x8, #21
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #176]
	ldr	x8, [sp, #184]
	add	x10, x8, #22
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #184]
	ldr	x8, [sp, #192]
	add	x10, x8, #23
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #192]
	ldr	x8, [sp, #200]
	add	x10, x8, #24
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #200]
	ldr	x8, [sp, #208]
	add	x10, x8, #25
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #208]
	ldr	x8, [sp, #216]
	add	x10, x8, #26
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #216]
	ldr	x8, [sp, #224]
	add	x10, x8, #27
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #224]
	ldr	x8, [sp, #232]
	add	x10, x8, #28
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #232]
	ldr	x8, [sp, #240]
	add	x10, x8, #29
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #240]
	ldr	x8, [sp, #248]
	add	x10, x8, #30
	eor	x9, x10, x9
	ror	x9, x9, #63
	add	x8, x9, x8
	str	x8, [sp, #248]
	ldr	x8, [sp, #256]
	add	x10, x8, #31
	eor	x9, x10, x9
	ror	x23, x9, #63
	add	x8, x23, x8
	str	x8, [sp, #256]
	subs	x19, x19, #1
	b.ne	LBB20_3
LBB20_4:
	cbnz	w22, LBB20_6
; %bb.5:
Lloh269:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh270:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB20_6:
	bl	_mach_absolute_time
Lloh271:
	adrp	x10, __MergedGlobals@PAGE
Lloh272:
	add	x10, x10, __MergedGlobals@PAGEOFF
	ldp	w9, w8, [x10, #8]
	and	x11, x23, #0x1f
	add	x12, sp, #8
	ldr	x11, [x12, x11, lsl #3]
	eor	x11, x11, x23
	str	x11, [x10]
	ldur	x10, [x29, #-56]
Lloh273:
	adrp	x11, ___stack_chk_guard@GOTPAGE
Lloh274:
	ldr	x11, [x11, ___stack_chk_guard@GOTPAGEOFF]
Lloh275:
	ldr	x11, [x11]
	cmp	x11, x10
	b.ne	LBB20_8
; %bb.7:
	mul	x10, x20, x21
	udiv	x10, x10, x22
	mul	x9, x0, x9
	udiv	x8, x9, x8
	sub	x0, x8, x10
	ldp	x29, x30, [sp, #320]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #304]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #288]            ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #272]            ; 16-byte Folded Reload
	add	sp, sp, #336
	ret
LBB20_8:
	bl	___stack_chk_fail
	.loh AdrpLdr	Lloh263, Lloh264
	.loh AdrpLdrGotLdr	Lloh260, Lloh261, Lloh262
	.loh AdrpAdd	Lloh265, Lloh266
	.loh AdrpAdd	Lloh267, Lloh268
	.loh AdrpAdd	Lloh269, Lloh270
	.loh AdrpLdrGotLdr	Lloh273, Lloh274, Lloh275
	.loh AdrpAdd	Lloh271, Lloh272
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
Lloh276:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh277:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB21_2
; %bb.1:
Lloh278:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh279:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB21_2:
	mov	x21, #52719                     ; =0xcdef
	movk	x21, #35243, lsl #16
	movk	x21, #17767, lsl #32
	movk	x21, #291, lsl #48
	bl	_mach_absolute_time
	mov	x20, x0
Lloh280:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh281:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x8]
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
	cbnz	w23, LBB21_9
; %bb.8:
Lloh282:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh283:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB21_9:
	mul	x8, x20, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
Lloh284:
	adrp	x8, __MergedGlobals@PAGE
Lloh285:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x21, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
	.loh AdrpLdr	Lloh276, Lloh277
	.loh AdrpAdd	Lloh278, Lloh279
	.loh AdrpAdd	Lloh280, Lloh281
	.loh AdrpAdd	Lloh282, Lloh283
	.loh AdrpAdd	Lloh284, Lloh285
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
Lloh286:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh287:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB22_6
; %bb.1:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh288:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh289:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbz	x20, LBB22_7
LBB22_2:
Lloh290:
	adrp	x8, _g_atomic_counter@PAGE
Lloh291:
	add	x8, x8, _g_atomic_counter@PAGEOFF
	mov	w9, #1                          ; =0x1
LBB22_3:                                ; =>This Inner Loop Header: Depth=1
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
	ldadd	x9, x23, [x8]
	subs	x20, x20, #1
	b.ne	LBB22_3
; %bb.4:
	cbz	w22, LBB22_8
LBB22_5:
	mul	x8, x19, x21
	udiv	x19, x8, x22
	bl	_mach_absolute_time
Lloh292:
	adrp	x8, __MergedGlobals@PAGE
Lloh293:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x23, [x8]
	sub	x0, x9, x19
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #32]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #16]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp], #64             ; 16-byte Folded Reload
	ret
LBB22_6:
Lloh294:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh295:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x19, x0
Lloh296:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh297:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbnz	x20, LBB22_2
LBB22_7:
	mov	x23, #0                         ; =0x0
	cbnz	w22, LBB22_5
LBB22_8:
Lloh298:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh299:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	b	LBB22_5
	.loh AdrpLdr	Lloh286, Lloh287
	.loh AdrpAdd	Lloh288, Lloh289
	.loh AdrpAdd	Lloh290, Lloh291
	.loh AdrpAdd	Lloh292, Lloh293
	.loh AdrpAdd	Lloh296, Lloh297
	.loh AdrpAdd	Lloh294, Lloh295
	.loh AdrpAdd	Lloh298, Lloh299
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
Lloh300:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh301:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB23_2
; %bb.1:
Lloh302:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh303:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB23_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh304:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh305:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
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
Lloh306:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh307:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB23_7:
	mul	x8, x19, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x8, x8, x9
	str	d8, [x21, #8]
	sub	x0, x8, x19
	ldp	x29, x30, [sp, #96]             ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #80]             ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #64]             ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #48]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #32]               ; 16-byte Folded Reload
	ldp	d11, d10, [sp, #16]             ; 16-byte Folded Reload
	ldp	d13, d12, [sp], #112            ; 16-byte Folded Reload
	ret
	.loh AdrpLdr	Lloh300, Lloh301
	.loh AdrpAdd	Lloh302, Lloh303
	.loh AdrpAdd	Lloh304, Lloh305
	.loh AdrpAdd	Lloh306, Lloh307
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

.zerofill __DATA,__bss,__MergedGlobals,24,3 ; @_MergedGlobals
.subsections_via_symbols

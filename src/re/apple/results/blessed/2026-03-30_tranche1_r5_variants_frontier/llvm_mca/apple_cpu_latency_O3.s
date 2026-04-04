	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 26, 0	sdk_version 26, 0
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #192
	stp	d11, d10, [sp, #64]             ; 16-byte Folded Spill
	stp	d9, d8, [sp, #80]               ; 16-byte Folded Spill
	stp	x28, x27, [sp, #96]             ; 16-byte Folded Spill
	stp	x26, x25, [sp, #112]            ; 16-byte Folded Spill
	stp	x24, x23, [sp, #128]            ; 16-byte Folded Spill
	stp	x22, x21, [sp, #144]            ; 16-byte Folded Spill
	stp	x20, x19, [sp, #160]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #176]            ; 16-byte Folded Spill
	add	x29, sp, #176
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
	.cfi_offset b10, -120
	.cfi_offset b11, -128
	mov	w28, #16960                     ; =0x4240
	movk	w28, #15, lsl #16
	cmp	w0, #2
	b.lt	LBB0_2
; %bb.1:
	mov	x21, x1
	mov	x22, x0
	mov	x20, #0                         ; =0x0
	mov	w25, #0                         ; =0x0
	mov	w27, w0
	mov	w8, #1                          ; =0x1
Lloh0:
	adrp	x23, l_.str@PAGE
Lloh1:
	add	x23, x23, l_.str@PAGEOFF
Lloh2:
	adrp	x24, l_.str.1@PAGE
Lloh3:
	add	x24, x24, l_.str.1@PAGEOFF
	b	LBB0_80
LBB0_2:
	mov	w23, #0                         ; =0x0
	mov	x8, #145685290680320            ; =0x848000000000
	movk	x8, #16686, lsl #48
	fmov	d8, x8
LBB0_3:
	adrp	x22, __MergedGlobals@PAGE+12
	ldr	w8, [x22, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_5
; %bb.4:
Lloh4:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh5:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB0_5:
	bl	_mach_absolute_time
	mov	x20, x0
Lloh6:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh7:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w25, w26, [x21]
	mov	w24, #1                         ; =0x1
	cbz	x28, LBB0_75
; %bb.6:
	mov	x8, x28
LBB0_7:                                 ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB0_7
; %bb.8:
	ldr	w8, [x22, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_10
LBB0_9:
Lloh8:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh9:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB0_10:
	mul	x8, x20, x25
	udiv	x19, x8, x26
	bl	_mach_absolute_time
Lloh10:
	adrp	x8, __MergedGlobals@PAGE
Lloh11:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x24, [x8]
	sub	x8, x9, x19
	mov	x10, #4629700416936869888       ; =0x4040000000000000
	fmov	d0, x10
	fmul	d9, d8, d0
	movi	d1, #0000000000000000
	fcmp	d9, #0.0
	movi	d0, #0000000000000000
	b.le	LBB0_34
; %bb.11:
	ucvtf	d0, x8
	fdiv	d0, d0, d9
	cmp	x9, x19
	b.eq	LBB0_35
LBB0_12:
	ucvtf	d1, x8
	fdiv	d1, d9, d1
	tbnz	w23, #0, LBB0_36
LBB0_13:
Lloh12:
	adrp	x9, l_.str.10@PAGE
Lloh13:
	add	x9, x9, l_.str.10@PAGEOFF
	stp	d0, d1, [sp, #16]
	stp	x9, x8, [sp]
Lloh14:
	adrp	x0, l_.str.18@PAGE
Lloh15:
	add	x0, x0, l_.str.18@PAGEOFF
	bl	_printf
	ldr	w8, [x22, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_15
LBB0_14:
Lloh16:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh17:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB0_15:
	bl	_mach_absolute_time
	mov	x20, x0
	ldp	w24, w25, [x21]
	cbz	x28, LBB0_76
; %bb.16:
	fmov	d0, #1.00000000
	mov	x8, x28
	fmov	d10, #1.00000000
LBB0_17:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	fadd	d10, d10, d0
	; InlineAsm End
	subs	x8, x8, #1
	b.ne	LBB0_17
; %bb.18:
	ldr	w8, [x22, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_20
LBB0_19:
Lloh18:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh19:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB0_20:
	mul	x8, x20, x24
	udiv	x19, x8, x25
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x9, x8, x9
	str	d10, [x21, #8]
	sub	x8, x9, x19
	movi	d1, #0000000000000000
	fcmp	d9, #0.0
	movi	d0, #0000000000000000
	b.le	LBB0_37
; %bb.21:
	ucvtf	d0, x8
	fdiv	d0, d0, d9
	cmp	x9, x19
	b.eq	LBB0_38
LBB0_22:
	ucvtf	d1, x8
	fdiv	d1, d9, d1
	tbnz	w23, #0, LBB0_39
LBB0_23:
Lloh20:
	adrp	x9, l_.str.11@PAGE
Lloh21:
	add	x9, x9, l_.str.11@PAGEOFF
	stp	d0, d1, [sp, #16]
	stp	x9, x8, [sp]
Lloh22:
	adrp	x0, l_.str.18@PAGE
Lloh23:
	add	x0, x0, l_.str.18@PAGEOFF
	bl	_printf
	ldr	w8, [x22, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_25
LBB0_24:
Lloh24:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh25:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB0_25:
	bl	_mach_absolute_time
	mov	x20, x0
	ldp	w24, w25, [x21]
	fmov	d10, #1.00000000
	cbz	x28, LBB0_77
; %bb.26:
	mov	x8, #536870912                  ; =0x20000000
	movk	x8, #16368, lsl #48
	fmov	d0, x8
	mov	x8, #281474439839744            ; =0xffffe0000000
	movk	x8, #16367, lsl #48
	fmov	d1, x8
	mov	x8, x28
LBB0_27:                                ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	fmadd	d10, d10, d0, d1
	; InlineAsm End
	subs	x8, x8, #1
	b.ne	LBB0_27
; %bb.28:
	ldr	w8, [x22, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_30
LBB0_29:
Lloh26:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh27:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB0_30:
	mul	x8, x20, x24
	udiv	x19, x8, x25
	bl	_mach_absolute_time
	ldp	w8, w9, [x21]
	mul	x8, x0, x8
	udiv	x9, x8, x9
	str	d10, [x21, #8]
	sub	x8, x9, x19
	movi	d1, #0000000000000000
	fcmp	d9, #0.0
	movi	d0, #0000000000000000
	b.le	LBB0_40
; %bb.31:
	ucvtf	d0, x8
	fdiv	d0, d0, d9
	cmp	x9, x19
	b.eq	LBB0_41
LBB0_32:
	ucvtf	d1, x8
	fdiv	d1, d9, d1
	tbnz	w23, #0, LBB0_42
LBB0_33:
Lloh28:
	adrp	x9, l_.str.12@PAGE
Lloh29:
	add	x9, x9, l_.str.12@PAGEOFF
	stp	d0, d1, [sp, #16]
	stp	x9, x8, [sp]
Lloh30:
	adrp	x0, l_.str.18@PAGE
Lloh31:
	add	x0, x0, l_.str.18@PAGEOFF
	b	LBB0_43
LBB0_34:
	cmp	x9, x19
	b.ne	LBB0_12
LBB0_35:
	tbz	w23, #0, LBB0_13
LBB0_36:
	stp	d0, d1, [sp, #32]
	mov	w9, #32                         ; =0x20
Lloh32:
	adrp	x10, l_.str.10@PAGE
Lloh33:
	add	x10, x10, l_.str.10@PAGEOFF
	stp	x9, x8, [sp, #16]
	stp	x10, x28, [sp]
Lloh34:
	adrp	x0, l_.str.17@PAGE
Lloh35:
	add	x0, x0, l_.str.17@PAGEOFF
	bl	_printf
	ldr	w8, [x22, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_15
	b	LBB0_14
LBB0_37:
	cmp	x9, x19
	b.ne	LBB0_22
LBB0_38:
	tbz	w23, #0, LBB0_23
LBB0_39:
	stp	d0, d1, [sp, #32]
	mov	w9, #32                         ; =0x20
Lloh36:
	adrp	x10, l_.str.11@PAGE
Lloh37:
	add	x10, x10, l_.str.11@PAGEOFF
	stp	x9, x8, [sp, #16]
	stp	x10, x28, [sp]
Lloh38:
	adrp	x0, l_.str.17@PAGE
Lloh39:
	add	x0, x0, l_.str.17@PAGEOFF
	bl	_printf
	ldr	w8, [x22, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_25
	b	LBB0_24
LBB0_40:
	cmp	x9, x19
	b.ne	LBB0_32
LBB0_41:
	tbz	w23, #0, LBB0_33
LBB0_42:
	stp	d0, d1, [sp, #32]
	mov	w9, #32                         ; =0x20
Lloh40:
	adrp	x10, l_.str.12@PAGE
Lloh41:
	add	x10, x10, l_.str.12@PAGEOFF
	stp	x9, x8, [sp, #16]
	stp	x10, x28, [sp]
Lloh42:
	adrp	x0, l_.str.17@PAGE
Lloh43:
	add	x0, x0, l_.str.17@PAGEOFF
LBB0_43:
	bl	_printf
	mov	x0, x28
	bl	_bench_load_store_chain
	mov	x8, #4636455816377925632        ; =0x4058000000000000
	fmov	d0, x8
	fmul	d2, d8, d0
	movi	d0, #0000000000000000
	fcmp	d2, #0.0
	ucvtf	d3, x0
	movi	d1, #0000000000000000
	b.le	LBB0_45
; %bb.44:
	fdiv	d1, d3, d2
LBB0_45:
	cbz	x0, LBB0_47
; %bb.46:
	fdiv	d0, d2, d3
LBB0_47:
	tbz	w23, #0, LBB0_49
; %bb.48:
	stp	d1, d0, [sp, #32]
	mov	w8, #96                         ; =0x60
Lloh44:
	adrp	x9, l_.str.13@PAGE
Lloh45:
	add	x9, x9, l_.str.13@PAGEOFF
	stp	x8, x0, [sp, #16]
	stp	x9, x28, [sp]
Lloh46:
	adrp	x0, l_.str.17@PAGE
Lloh47:
	add	x0, x0, l_.str.17@PAGEOFF
	b	LBB0_50
LBB0_49:
Lloh48:
	adrp	x8, l_.str.13@PAGE
Lloh49:
	add	x8, x8, l_.str.13@PAGEOFF
	stp	d1, d0, [sp, #16]
	stp	x8, x0, [sp]
Lloh50:
	adrp	x0, l_.str.18@PAGE
Lloh51:
	add	x0, x0, l_.str.18@PAGEOFF
LBB0_50:
	bl	_printf
	mov	x0, x28
	bl	_bench_shuffle_dep
	movi	d0, #0000000000000000
	fcmp	d9, #0.0
	ucvtf	d2, x0
	movi	d1, #0000000000000000
	b.le	LBB0_52
; %bb.51:
	fdiv	d1, d2, d9
LBB0_52:
	cbz	x0, LBB0_54
; %bb.53:
	fdiv	d0, d9, d2
LBB0_54:
	tbz	w23, #0, LBB0_56
; %bb.55:
	stp	d1, d0, [sp, #32]
	mov	w8, #32                         ; =0x20
Lloh52:
	adrp	x9, l_.str.14@PAGE
Lloh53:
	add	x9, x9, l_.str.14@PAGEOFF
	stp	x8, x0, [sp, #16]
	stp	x9, x28, [sp]
Lloh54:
	adrp	x0, l_.str.17@PAGE
Lloh55:
	add	x0, x0, l_.str.17@PAGEOFF
	b	LBB0_57
LBB0_56:
Lloh56:
	adrp	x8, l_.str.14@PAGE
Lloh57:
	add	x8, x8, l_.str.14@PAGEOFF
	stp	d1, d0, [sp, #16]
	stp	x8, x0, [sp]
Lloh58:
	adrp	x0, l_.str.18@PAGE
Lloh59:
	add	x0, x0, l_.str.18@PAGEOFF
LBB0_57:
	bl	_printf
	mov	x0, x28
	bl	_bench_atomic_add_dep
	movi	d0, #0000000000000000
	fcmp	d9, #0.0
	ucvtf	d2, x0
	movi	d1, #0000000000000000
	b.le	LBB0_59
; %bb.58:
	fdiv	d1, d2, d9
LBB0_59:
	cbz	x0, LBB0_61
; %bb.60:
	fdiv	d0, d9, d2
LBB0_61:
	tbz	w23, #0, LBB0_63
; %bb.62:
	stp	d1, d0, [sp, #32]
	mov	w8, #32                         ; =0x20
Lloh60:
	adrp	x9, l_.str.15@PAGE
Lloh61:
	add	x9, x9, l_.str.15@PAGEOFF
	stp	x8, x0, [sp, #16]
	stp	x9, x28, [sp]
Lloh62:
	adrp	x0, l_.str.17@PAGE
Lloh63:
	add	x0, x0, l_.str.17@PAGEOFF
	b	LBB0_64
LBB0_63:
Lloh64:
	adrp	x8, l_.str.15@PAGE
Lloh65:
	add	x8, x8, l_.str.15@PAGEOFF
	stp	d1, d0, [sp, #16]
	stp	x8, x0, [sp]
Lloh66:
	adrp	x0, l_.str.18@PAGE
Lloh67:
	add	x0, x0, l_.str.18@PAGEOFF
LBB0_64:
	bl	_printf
	mov	x0, x28
	bl	_bench_transcendental_dep
	fmov	d0, #8.00000000
	fmul	d2, d8, d0
	movi	d0, #0000000000000000
	fcmp	d2, #0.0
	ucvtf	d3, x0
	movi	d1, #0000000000000000
	b.le	LBB0_66
; %bb.65:
	fdiv	d1, d3, d2
LBB0_66:
	cbz	x0, LBB0_68
; %bb.67:
	fdiv	d0, d2, d3
LBB0_68:
	tbz	w23, #0, LBB0_70
; %bb.69:
	stp	d1, d0, [sp, #32]
	mov	w8, #8                          ; =0x8
Lloh68:
	adrp	x9, l_.str.16@PAGE
Lloh69:
	add	x9, x9, l_.str.16@PAGEOFF
	stp	x8, x0, [sp, #16]
	stp	x9, x28, [sp]
Lloh70:
	adrp	x0, l_.str.17@PAGE
Lloh71:
	add	x0, x0, l_.str.17@PAGEOFF
	b	LBB0_72
LBB0_70:
Lloh72:
	adrp	x8, l_.str.16@PAGE
Lloh73:
	add	x8, x8, l_.str.16@PAGEOFF
	stp	d1, d0, [sp, #16]
	stp	x8, x0, [sp]
LBB0_71:
Lloh74:
	adrp	x0, l_.str.18@PAGE
Lloh75:
	add	x0, x0, l_.str.18@PAGEOFF
LBB0_72:
	bl	_printf
LBB0_73:
	mov	w0, #0                          ; =0x0
LBB0_74:
	ldp	x29, x30, [sp, #176]            ; 16-byte Folded Reload
	ldp	x20, x19, [sp, #160]            ; 16-byte Folded Reload
	ldp	x22, x21, [sp, #144]            ; 16-byte Folded Reload
	ldp	x24, x23, [sp, #128]            ; 16-byte Folded Reload
	ldp	x26, x25, [sp, #112]            ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #96]             ; 16-byte Folded Reload
	ldp	d9, d8, [sp, #80]               ; 16-byte Folded Reload
	ldp	d11, d10, [sp, #64]             ; 16-byte Folded Reload
	add	sp, sp, #192
	ret
LBB0_75:
	mov	x8, x26
	cbnz	w8, LBB0_10
	b	LBB0_9
LBB0_76:
	fmov	d10, #1.00000000
	mov	x8, x25
	cbnz	w8, LBB0_20
	b	LBB0_19
LBB0_77:
	mov	x8, x25
	cbnz	w8, LBB0_30
	b	LBB0_29
LBB0_78:                                ;   in Loop: Header=BB0_80 Depth=1
	mov	w25, #1                         ; =0x1
	mov	x19, x28
LBB0_79:                                ;   in Loop: Header=BB0_80 Depth=1
	sbfiz	x8, x19, #3, #32
	ldr	x0, [x21, x8]
	mov	x1, #0                          ; =0x0
	mov	w2, #10                         ; =0xa
	bl	_strtoull
	mov	x28, x0
	add	w8, w19, #1
	cmp	w8, w22
	b.ge	LBB0_90
LBB0_80:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB0_84 Depth 2
	sxtw	x19, w8
	ldr	x26, [x21, w8, sxtw #3]
	mov	x0, x26
	mov	x1, x23
	bl	_strcmp
	add	x19, x19, #1
	cmp	w0, #0
	ccmp	x19, x27, #0, eq
	b.lt	LBB0_79
; %bb.81:                               ;   in Loop: Header=BB0_80 Depth=1
	mov	x0, x26
	mov	x1, x24
	bl	_strcmp
	cmp	w0, #0
	ccmp	x19, x27, #0, eq
	b.lt	LBB0_89
; %bb.82:                               ;   in Loop: Header=BB0_80 Depth=1
	mov	x25, x28
	mov	x0, x26
Lloh76:
	adrp	x1, l_.str.2@PAGE
Lloh77:
	add	x1, x1, l_.str.2@PAGEOFF
	bl	_strcmp
	cbnz	w0, LBB0_92
; %bb.83:                               ;   in Loop: Header=BB0_80 Depth=1
	cmp	x19, x27
	b.ge	LBB0_95
LBB0_84:                                ;   Parent Loop BB0_80 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	x26, [x21, x19, lsl #3]
	mov	x0, x26
	mov	x1, x23
	bl	_strcmp
	add	x28, x19, #1
	cmp	w0, #0
	ccmp	x28, x27, #0, eq
	b.lt	LBB0_78
; %bb.85:                               ;   in Loop: Header=BB0_84 Depth=2
	mov	x0, x26
	mov	x1, x24
	bl	_strcmp
	cmp	w0, #0
	ccmp	x28, x27, #0, eq
	b.lt	LBB0_88
; %bb.86:                               ;   in Loop: Header=BB0_84 Depth=2
	mov	x0, x26
Lloh78:
	adrp	x1, l_.str.2@PAGE
Lloh79:
	add	x1, x1, l_.str.2@PAGEOFF
	bl	_strcmp
	cbnz	w0, LBB0_92
; %bb.87:                               ;   in Loop: Header=BB0_84 Depth=2
	mov	x19, x28
	cmp	x28, x27
	b.lt	LBB0_84
	b	LBB0_95
LBB0_88:                                ;   in Loop: Header=BB0_80 Depth=1
	add	x19, x19, #1
	mov	x28, x25
	mov	w25, #1                         ; =0x1
LBB0_89:                                ;   in Loop: Header=BB0_80 Depth=1
	sbfiz	x8, x19, #3, #32
	ldr	x20, [x21, x8]
	add	w8, w19, #1
	cmp	w8, w22
	b.lt	LBB0_80
LBB0_90:
	tbnz	w25, #0, LBB0_96
; %bb.91:
	mov	w23, #0                         ; =0x0
	b	LBB0_97
LBB0_92:
Lloh80:
	adrp	x1, l_.str.3@PAGE
Lloh81:
	add	x1, x1, l_.str.3@PAGEOFF
	mov	x0, x26
	bl	_strcmp
	cbz	w0, LBB0_108
; %bb.93:
Lloh82:
	adrp	x1, l_.str.4@PAGE
Lloh83:
	add	x1, x1, l_.str.4@PAGEOFF
	mov	x0, x26
	bl	_strcmp
	cbz	w0, LBB0_160
; %bb.94:
Lloh84:
	adrp	x8, ___stderrp@GOTPAGE
Lloh85:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh86:
	ldr	x0, [x8]
	str	x26, [sp]
Lloh87:
	adrp	x1, l_.str.6@PAGE
Lloh88:
	add	x1, x1, l_.str.6@PAGEOFF
	b	LBB0_107
LBB0_95:
	mov	x28, x25
LBB0_96:
Lloh89:
	adrp	x0, l_str@PAGE
Lloh90:
	add	x0, x0, l_str@PAGEOFF
	bl	_puts
	mov	w23, #1                         ; =0x1
LBB0_97:
	ucvtf	d8, x28
	cbz	x20, LBB0_3
; %bb.98:
Lloh91:
	adrp	x21, l_.str.10@PAGE
Lloh92:
	add	x21, x21, l_.str.10@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cmp	w0, #0
	cset	w24, eq
	cbz	w0, LBB0_109
; %bb.99:
Lloh93:
	adrp	x21, l_.str.11@PAGE
Lloh94:
	add	x21, x21, l_.str.11@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cbz	w0, LBB0_124
LBB0_100:
Lloh95:
	adrp	x21, l_.str.12@PAGE
Lloh96:
	add	x21, x21, l_.str.12@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cbz	w0, LBB0_139
LBB0_101:
Lloh97:
	adrp	x21, l_.str.13@PAGE
Lloh98:
	add	x21, x21, l_.str.13@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cbz	w0, LBB0_154
LBB0_102:
Lloh99:
	adrp	x21, l_.str.14@PAGE
Lloh100:
	add	x21, x21, l_.str.14@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cbz	w0, LBB0_163
LBB0_103:
Lloh101:
	adrp	x21, l_.str.15@PAGE
Lloh102:
	add	x21, x21, l_.str.15@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cbz	w0, LBB0_171
LBB0_104:
Lloh103:
	adrp	x21, l_.str.16@PAGE
Lloh104:
	add	x21, x21, l_.str.16@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cbz	w0, LBB0_179
LBB0_105:
	tbnz	w24, #0, LBB0_73
; %bb.106:
Lloh105:
	adrp	x8, ___stderrp@GOTPAGE
Lloh106:
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
Lloh107:
	ldr	x0, [x8]
	str	x20, [sp]
Lloh108:
	adrp	x1, l_.str.8@PAGE
Lloh109:
	add	x1, x1, l_.str.8@PAGEOFF
LBB0_107:
	bl	_fprintf
	mov	w0, #1                          ; =0x1
	b	LBB0_74
LBB0_108:
Lloh110:
	adrp	x0, l_.str.10@PAGE
Lloh111:
	add	x0, x0, l_.str.10@PAGEOFF
	bl	_puts
Lloh112:
	adrp	x0, l_.str.11@PAGE
Lloh113:
	add	x0, x0, l_.str.11@PAGEOFF
	bl	_puts
Lloh114:
	adrp	x0, l_.str.12@PAGE
Lloh115:
	add	x0, x0, l_.str.12@PAGEOFF
	bl	_puts
Lloh116:
	adrp	x0, l_.str.13@PAGE
Lloh117:
	add	x0, x0, l_.str.13@PAGEOFF
	bl	_puts
Lloh118:
	adrp	x0, l_.str.14@PAGE
Lloh119:
	add	x0, x0, l_.str.14@PAGEOFF
	bl	_puts
Lloh120:
	adrp	x0, l_.str.15@PAGE
Lloh121:
	add	x0, x0, l_.str.15@PAGEOFF
	bl	_puts
Lloh122:
	adrp	x0, l_.str.16@PAGE
Lloh123:
	add	x0, x0, l_.str.16@PAGEOFF
	bl	_puts
	b	LBB0_73
LBB0_109:
	adrp	x27, __MergedGlobals@PAGE+12
	ldr	w8, [x27, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_111
; %bb.110:
Lloh124:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh125:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB0_111:
	str	x20, [sp, #56]                  ; 8-byte Folded Spill
	bl	_mach_absolute_time
	mov	x22, x0
Lloh126:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh127:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w25, w26, [x8]
	mov	w20, #1                         ; =0x1
	cbz	x28, LBB0_186
; %bb.112:
	mov	x8, x28
LBB0_113:                               ; =>This Inner Loop Header: Depth=1
	; InlineAsm Start
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	add	x20, x20, #1
	; InlineAsm End
	subs	x8, x8, #1
	b.ne	LBB0_113
; %bb.114:
	ldr	w8, [x27, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_116
LBB0_115:
Lloh128:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh129:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB0_116:
	mul	x8, x22, x25
	udiv	x19, x8, x26
	bl	_mach_absolute_time
Lloh130:
	adrp	x8, __MergedGlobals@PAGE
Lloh131:
	add	x8, x8, __MergedGlobals@PAGEOFF
	ldp	w9, w10, [x8, #8]
	mul	x9, x0, x9
	udiv	x9, x9, x10
	str	x20, [x8]
	sub	x8, x9, x19
	mov	x10, #4629700416936869888       ; =0x4040000000000000
	fmov	d0, x10
	fmul	d2, d8, d0
	movi	d1, #0000000000000000
	fcmp	d2, #0.0
	movi	d0, #0000000000000000
	b.le	LBB0_120
; %bb.117:
	ucvtf	d0, x8
	fdiv	d0, d0, d2
	cmp	x9, x19
	ldr	x20, [sp, #56]                  ; 8-byte Folded Reload
	b.eq	LBB0_121
LBB0_118:
	ucvtf	d1, x8
	fdiv	d1, d2, d1
	cbnz	w23, LBB0_122
LBB0_119:
	stp	d0, d1, [sp, #16]
	stp	x21, x8, [sp]
Lloh132:
	adrp	x0, l_.str.18@PAGE
Lloh133:
	add	x0, x0, l_.str.18@PAGEOFF
	b	LBB0_123
LBB0_120:
	cmp	x9, x19
	ldr	x20, [sp, #56]                  ; 8-byte Folded Reload
	b.ne	LBB0_118
LBB0_121:
	cbz	w23, LBB0_119
LBB0_122:
	stp	d0, d1, [sp, #32]
	mov	w9, #32                         ; =0x20
	stp	x9, x8, [sp, #16]
	stp	x21, x28, [sp]
Lloh134:
	adrp	x0, l_.str.17@PAGE
Lloh135:
	add	x0, x0, l_.str.17@PAGEOFF
LBB0_123:
	bl	_printf
Lloh136:
	adrp	x21, l_.str.11@PAGE
Lloh137:
	add	x21, x21, l_.str.11@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cbnz	w0, LBB0_100
LBB0_124:
	mov	x26, x20
	adrp	x19, __MergedGlobals@PAGE+12
	ldr	w8, [x19, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_126
; %bb.125:
Lloh138:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh139:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB0_126:
	bl	_mach_absolute_time
	mov	x22, x0
Lloh140:
	adrp	x20, __MergedGlobals@PAGE+8
Lloh141:
	add	x20, x20, __MergedGlobals@PAGEOFF+8
	ldp	w24, w25, [x20]
	cbz	x28, LBB0_187
; %bb.127:
	fmov	d0, #1.00000000
	mov	x8, x28
	fmov	d9, #1.00000000
LBB0_128:                               ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB0_128
; %bb.129:
	ldr	w8, [x19, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_131
LBB0_130:
Lloh142:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh143:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB0_131:
	mul	x8, x22, x24
	udiv	x19, x8, x25
	bl	_mach_absolute_time
	ldp	w8, w9, [x20]
	mul	x8, x0, x8
	udiv	x9, x8, x9
	str	d9, [x20, #8]
	sub	x8, x9, x19
	mov	x10, #4629700416936869888       ; =0x4040000000000000
	fmov	d0, x10
	fmul	d2, d8, d0
	movi	d1, #0000000000000000
	fcmp	d2, #0.0
	movi	d0, #0000000000000000
	b.le	LBB0_135
; %bb.132:
	ucvtf	d0, x8
	fdiv	d0, d0, d2
	cmp	x9, x19
	mov	x20, x26
	b.eq	LBB0_136
LBB0_133:
	ucvtf	d1, x8
	fdiv	d1, d2, d1
	tbnz	w23, #0, LBB0_137
LBB0_134:
	stp	d0, d1, [sp, #16]
	stp	x21, x8, [sp]
Lloh144:
	adrp	x0, l_.str.18@PAGE
Lloh145:
	add	x0, x0, l_.str.18@PAGEOFF
	b	LBB0_138
LBB0_135:
	cmp	x9, x19
	mov	x20, x26
	b.ne	LBB0_133
LBB0_136:
	tbz	w23, #0, LBB0_134
LBB0_137:
	stp	d0, d1, [sp, #32]
	mov	w9, #32                         ; =0x20
	stp	x9, x8, [sp, #16]
	stp	x21, x28, [sp]
Lloh146:
	adrp	x0, l_.str.17@PAGE
Lloh147:
	add	x0, x0, l_.str.17@PAGEOFF
LBB0_138:
	bl	_printf
	mov	w24, #1                         ; =0x1
Lloh148:
	adrp	x21, l_.str.12@PAGE
Lloh149:
	add	x21, x21, l_.str.12@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cbnz	w0, LBB0_101
LBB0_139:
	mov	x27, x20
	adrp	x26, __MergedGlobals@PAGE+12
	ldr	w8, [x26, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_141
; %bb.140:
Lloh150:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh151:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB0_141:
	bl	_mach_absolute_time
	mov	x22, x0
Lloh152:
	adrp	x20, __MergedGlobals@PAGE+8
Lloh153:
	add	x20, x20, __MergedGlobals@PAGEOFF+8
	ldp	w24, w25, [x20]
	fmov	d9, #1.00000000
	cbz	x28, LBB0_188
; %bb.142:
	mov	x8, #536870912                  ; =0x20000000
	movk	x8, #16368, lsl #48
	fmov	d0, x8
	mov	x8, #281474439839744            ; =0xffffe0000000
	movk	x8, #16367, lsl #48
	fmov	d1, x8
	mov	x8, x28
LBB0_143:                               ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB0_143
; %bb.144:
	ldr	w8, [x26, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB0_146
LBB0_145:
Lloh154:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh155:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB0_146:
	mul	x8, x22, x24
	udiv	x19, x8, x25
	bl	_mach_absolute_time
	ldp	w8, w9, [x20]
	mul	x8, x0, x8
	udiv	x9, x8, x9
	str	d9, [x20, #8]
	sub	x8, x9, x19
	mov	x10, #4629700416936869888       ; =0x4040000000000000
	fmov	d0, x10
	fmul	d2, d8, d0
	movi	d1, #0000000000000000
	fcmp	d2, #0.0
	movi	d0, #0000000000000000
	b.le	LBB0_150
; %bb.147:
	ucvtf	d0, x8
	fdiv	d0, d0, d2
	cmp	x9, x19
	mov	x20, x27
	b.eq	LBB0_151
LBB0_148:
	ucvtf	d1, x8
	fdiv	d1, d2, d1
	tbnz	w23, #0, LBB0_152
LBB0_149:
	stp	d0, d1, [sp, #16]
	stp	x21, x8, [sp]
Lloh156:
	adrp	x0, l_.str.18@PAGE
Lloh157:
	add	x0, x0, l_.str.18@PAGEOFF
	b	LBB0_153
LBB0_150:
	cmp	x9, x19
	mov	x20, x27
	b.ne	LBB0_148
LBB0_151:
	tbz	w23, #0, LBB0_149
LBB0_152:
	stp	d0, d1, [sp, #32]
	mov	w9, #32                         ; =0x20
	stp	x9, x8, [sp, #16]
	stp	x21, x28, [sp]
Lloh158:
	adrp	x0, l_.str.17@PAGE
Lloh159:
	add	x0, x0, l_.str.17@PAGEOFF
LBB0_153:
	bl	_printf
	mov	w24, #1                         ; =0x1
Lloh160:
	adrp	x21, l_.str.13@PAGE
Lloh161:
	add	x21, x21, l_.str.13@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cbnz	w0, LBB0_102
LBB0_154:
	mov	x0, x28
	bl	_bench_load_store_chain
	mov	x8, #4636455816377925632        ; =0x4058000000000000
	fmov	d0, x8
	fmul	d2, d8, d0
	movi	d0, #0000000000000000
	fcmp	d2, #0.0
	ucvtf	d3, x0
	movi	d1, #0000000000000000
	b.le	LBB0_156
; %bb.155:
	fdiv	d1, d3, d2
LBB0_156:
	cbz	x0, LBB0_158
; %bb.157:
	fdiv	d0, d2, d3
LBB0_158:
	tbz	w23, #0, LBB0_161
; %bb.159:
	stp	d1, d0, [sp, #32]
	mov	w8, #96                         ; =0x60
	stp	x8, x0, [sp, #16]
	stp	x21, x28, [sp]
Lloh162:
	adrp	x0, l_.str.17@PAGE
Lloh163:
	add	x0, x0, l_.str.17@PAGEOFF
	b	LBB0_162
LBB0_160:
	ldr	x8, [x21]
	str	x8, [sp]
Lloh164:
	adrp	x0, l_.str.5@PAGE
Lloh165:
	add	x0, x0, l_.str.5@PAGEOFF
	b	LBB0_72
LBB0_161:
	stp	d1, d0, [sp, #16]
	stp	x21, x0, [sp]
Lloh166:
	adrp	x0, l_.str.18@PAGE
Lloh167:
	add	x0, x0, l_.str.18@PAGEOFF
LBB0_162:
	bl	_printf
	mov	w24, #1                         ; =0x1
Lloh168:
	adrp	x21, l_.str.14@PAGE
Lloh169:
	add	x21, x21, l_.str.14@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cbnz	w0, LBB0_103
LBB0_163:
	mov	x0, x28
	bl	_bench_shuffle_dep
	mov	x8, #4629700416936869888        ; =0x4040000000000000
	fmov	d0, x8
	fmul	d2, d8, d0
	movi	d0, #0000000000000000
	fcmp	d2, #0.0
	ucvtf	d3, x0
	movi	d1, #0000000000000000
	b.le	LBB0_165
; %bb.164:
	fdiv	d1, d3, d2
LBB0_165:
	cbz	x0, LBB0_167
; %bb.166:
	fdiv	d0, d2, d3
LBB0_167:
	tbz	w23, #0, LBB0_169
; %bb.168:
	stp	d1, d0, [sp, #32]
	mov	w8, #32                         ; =0x20
	stp	x8, x0, [sp, #16]
	stp	x21, x28, [sp]
Lloh170:
	adrp	x0, l_.str.17@PAGE
Lloh171:
	add	x0, x0, l_.str.17@PAGEOFF
	b	LBB0_170
LBB0_169:
	stp	d1, d0, [sp, #16]
	stp	x21, x0, [sp]
Lloh172:
	adrp	x0, l_.str.18@PAGE
Lloh173:
	add	x0, x0, l_.str.18@PAGEOFF
LBB0_170:
	bl	_printf
	mov	w24, #1                         ; =0x1
Lloh174:
	adrp	x21, l_.str.15@PAGE
Lloh175:
	add	x21, x21, l_.str.15@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cbnz	w0, LBB0_104
LBB0_171:
	mov	x0, x28
	bl	_bench_atomic_add_dep
	mov	x8, #4629700416936869888        ; =0x4040000000000000
	fmov	d0, x8
	fmul	d2, d8, d0
	movi	d0, #0000000000000000
	fcmp	d2, #0.0
	ucvtf	d3, x0
	movi	d1, #0000000000000000
	b.le	LBB0_173
; %bb.172:
	fdiv	d1, d3, d2
LBB0_173:
	cbz	x0, LBB0_175
; %bb.174:
	fdiv	d0, d2, d3
LBB0_175:
	tbz	w23, #0, LBB0_177
; %bb.176:
	stp	d1, d0, [sp, #32]
	mov	w8, #32                         ; =0x20
	stp	x8, x0, [sp, #16]
	stp	x21, x28, [sp]
Lloh176:
	adrp	x0, l_.str.17@PAGE
Lloh177:
	add	x0, x0, l_.str.17@PAGEOFF
	b	LBB0_178
LBB0_177:
	stp	d1, d0, [sp, #16]
	stp	x21, x0, [sp]
Lloh178:
	adrp	x0, l_.str.18@PAGE
Lloh179:
	add	x0, x0, l_.str.18@PAGEOFF
LBB0_178:
	bl	_printf
	mov	w24, #1                         ; =0x1
Lloh180:
	adrp	x21, l_.str.16@PAGE
Lloh181:
	add	x21, x21, l_.str.16@PAGEOFF
	mov	x0, x20
	mov	x1, x21
	bl	_strcmp
	cbnz	w0, LBB0_105
LBB0_179:
	mov	x0, x28
	bl	_bench_transcendental_dep
	fmov	d0, #8.00000000
	fmul	d2, d8, d0
	movi	d0, #0000000000000000
	fcmp	d2, #0.0
	ucvtf	d3, x0
	movi	d1, #0000000000000000
	b.le	LBB0_181
; %bb.180:
	fdiv	d1, d3, d2
LBB0_181:
	cbz	x0, LBB0_183
; %bb.182:
	fdiv	d0, d2, d3
LBB0_183:
	tbz	w23, #0, LBB0_185
; %bb.184:
	stp	d1, d0, [sp, #32]
	mov	w8, #8                          ; =0x8
	stp	x8, x0, [sp, #16]
	stp	x21, x28, [sp]
Lloh182:
	adrp	x0, l_.str.17@PAGE
Lloh183:
	add	x0, x0, l_.str.17@PAGEOFF
	b	LBB0_72
LBB0_185:
	stp	d1, d0, [sp, #16]
	stp	x21, x0, [sp]
	b	LBB0_71
LBB0_186:
	mov	x8, x26
	cbnz	w8, LBB0_116
	b	LBB0_115
LBB0_187:
	fmov	d9, #1.00000000
	mov	x8, x25
	cbnz	w8, LBB0_131
	b	LBB0_130
LBB0_188:
	mov	x8, x25
	cbnz	w8, LBB0_146
	b	LBB0_145
	.loh AdrpAdd	Lloh2, Lloh3
	.loh AdrpAdd	Lloh0, Lloh1
	.loh AdrpAdd	Lloh4, Lloh5
	.loh AdrpAdd	Lloh6, Lloh7
	.loh AdrpAdd	Lloh8, Lloh9
	.loh AdrpAdd	Lloh10, Lloh11
	.loh AdrpAdd	Lloh14, Lloh15
	.loh AdrpAdd	Lloh12, Lloh13
	.loh AdrpAdd	Lloh16, Lloh17
	.loh AdrpAdd	Lloh18, Lloh19
	.loh AdrpAdd	Lloh22, Lloh23
	.loh AdrpAdd	Lloh20, Lloh21
	.loh AdrpAdd	Lloh24, Lloh25
	.loh AdrpAdd	Lloh26, Lloh27
	.loh AdrpAdd	Lloh30, Lloh31
	.loh AdrpAdd	Lloh28, Lloh29
	.loh AdrpAdd	Lloh34, Lloh35
	.loh AdrpAdd	Lloh32, Lloh33
	.loh AdrpAdd	Lloh38, Lloh39
	.loh AdrpAdd	Lloh36, Lloh37
	.loh AdrpAdd	Lloh42, Lloh43
	.loh AdrpAdd	Lloh40, Lloh41
	.loh AdrpAdd	Lloh46, Lloh47
	.loh AdrpAdd	Lloh44, Lloh45
	.loh AdrpAdd	Lloh50, Lloh51
	.loh AdrpAdd	Lloh48, Lloh49
	.loh AdrpAdd	Lloh54, Lloh55
	.loh AdrpAdd	Lloh52, Lloh53
	.loh AdrpAdd	Lloh58, Lloh59
	.loh AdrpAdd	Lloh56, Lloh57
	.loh AdrpAdd	Lloh62, Lloh63
	.loh AdrpAdd	Lloh60, Lloh61
	.loh AdrpAdd	Lloh66, Lloh67
	.loh AdrpAdd	Lloh64, Lloh65
	.loh AdrpAdd	Lloh70, Lloh71
	.loh AdrpAdd	Lloh68, Lloh69
	.loh AdrpAdd	Lloh72, Lloh73
	.loh AdrpAdd	Lloh74, Lloh75
	.loh AdrpAdd	Lloh76, Lloh77
	.loh AdrpAdd	Lloh78, Lloh79
	.loh AdrpAdd	Lloh80, Lloh81
	.loh AdrpAdd	Lloh82, Lloh83
	.loh AdrpAdd	Lloh87, Lloh88
	.loh AdrpLdrGotLdr	Lloh84, Lloh85, Lloh86
	.loh AdrpAdd	Lloh89, Lloh90
	.loh AdrpAdd	Lloh91, Lloh92
	.loh AdrpAdd	Lloh93, Lloh94
	.loh AdrpAdd	Lloh95, Lloh96
	.loh AdrpAdd	Lloh97, Lloh98
	.loh AdrpAdd	Lloh99, Lloh100
	.loh AdrpAdd	Lloh101, Lloh102
	.loh AdrpAdd	Lloh103, Lloh104
	.loh AdrpAdd	Lloh108, Lloh109
	.loh AdrpLdrGotLdr	Lloh105, Lloh106, Lloh107
	.loh AdrpAdd	Lloh122, Lloh123
	.loh AdrpAdd	Lloh120, Lloh121
	.loh AdrpAdd	Lloh118, Lloh119
	.loh AdrpAdd	Lloh116, Lloh117
	.loh AdrpAdd	Lloh114, Lloh115
	.loh AdrpAdd	Lloh112, Lloh113
	.loh AdrpAdd	Lloh110, Lloh111
	.loh AdrpAdd	Lloh124, Lloh125
	.loh AdrpAdd	Lloh126, Lloh127
	.loh AdrpAdd	Lloh128, Lloh129
	.loh AdrpAdd	Lloh130, Lloh131
	.loh AdrpAdd	Lloh132, Lloh133
	.loh AdrpAdd	Lloh134, Lloh135
	.loh AdrpAdd	Lloh136, Lloh137
	.loh AdrpAdd	Lloh138, Lloh139
	.loh AdrpAdd	Lloh140, Lloh141
	.loh AdrpAdd	Lloh142, Lloh143
	.loh AdrpAdd	Lloh144, Lloh145
	.loh AdrpAdd	Lloh146, Lloh147
	.loh AdrpAdd	Lloh148, Lloh149
	.loh AdrpAdd	Lloh150, Lloh151
	.loh AdrpAdd	Lloh152, Lloh153
	.loh AdrpAdd	Lloh154, Lloh155
	.loh AdrpAdd	Lloh156, Lloh157
	.loh AdrpAdd	Lloh158, Lloh159
	.loh AdrpAdd	Lloh160, Lloh161
	.loh AdrpAdd	Lloh162, Lloh163
	.loh AdrpAdd	Lloh164, Lloh165
	.loh AdrpAdd	Lloh166, Lloh167
	.loh AdrpAdd	Lloh168, Lloh169
	.loh AdrpAdd	Lloh170, Lloh171
	.loh AdrpAdd	Lloh172, Lloh173
	.loh AdrpAdd	Lloh174, Lloh175
	.loh AdrpAdd	Lloh176, Lloh177
	.loh AdrpAdd	Lloh178, Lloh179
	.loh AdrpAdd	Lloh180, Lloh181
	.loh AdrpAdd	Lloh182, Lloh183
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
Lloh184:
	adrp	x8, ___stack_chk_guard@GOTPAGE
Lloh185:
	ldr	x8, [x8, ___stack_chk_guard@GOTPAGEOFF]
Lloh186:
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
Lloh187:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh188:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB1_2
; %bb.1:
Lloh189:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh190:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB1_2:
	mov	x23, #31765                     ; =0x7c15
	movk	x23, #32586, lsl #16
	movk	x23, #31161, lsl #32
	movk	x23, #40503, lsl #48
	bl	_mach_absolute_time
	mov	x20, x0
Lloh191:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh192:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbz	x19, LBB1_4
LBB1_3:                                 ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB1_3
LBB1_4:
	cbnz	w22, LBB1_6
; %bb.5:
Lloh193:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh194:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB1_6:
	bl	_mach_absolute_time
Lloh195:
	adrp	x10, __MergedGlobals@PAGE
Lloh196:
	add	x10, x10, __MergedGlobals@PAGEOFF
	ldp	w9, w8, [x10, #8]
	and	x11, x23, #0x1f
	add	x12, sp, #8
	ldr	x11, [x12, x11, lsl #3]
	eor	x11, x11, x23
	str	x11, [x10]
	ldur	x10, [x29, #-56]
Lloh197:
	adrp	x11, ___stack_chk_guard@GOTPAGE
Lloh198:
	ldr	x11, [x11, ___stack_chk_guard@GOTPAGEOFF]
Lloh199:
	ldr	x11, [x11]
	cmp	x11, x10
	b.ne	LBB1_8
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
LBB1_8:
	bl	___stack_chk_fail
	.loh AdrpLdr	Lloh187, Lloh188
	.loh AdrpLdrGotLdr	Lloh184, Lloh185, Lloh186
	.loh AdrpAdd	Lloh189, Lloh190
	.loh AdrpAdd	Lloh191, Lloh192
	.loh AdrpAdd	Lloh193, Lloh194
	.loh AdrpLdrGotLdr	Lloh197, Lloh198, Lloh199
	.loh AdrpAdd	Lloh195, Lloh196
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
Lloh200:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh201:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB2_2
; %bb.1:
Lloh202:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh203:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB2_2:
	mov	x21, #52719                     ; =0xcdef
	movk	x21, #35243, lsl #16
	movk	x21, #17767, lsl #32
	movk	x21, #291, lsl #48
	bl	_mach_absolute_time
	mov	x20, x0
Lloh204:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh205:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x8]
	cbz	x19, LBB2_7
; %bb.3:
	mov	x8, #0                          ; =0x0
	mov	x9, #31765                      ; =0x7c15
	movk	x9, #32586, lsl #16
	movk	x9, #31161, lsl #32
	movk	x9, #40503, lsl #48
LBB2_4:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB2_5 Depth 2
	mov	x10, #0                         ; =0x0
	mov	x11, #0                         ; =0x0
LBB2_5:                                 ;   Parent Loop BB2_4 Depth=1
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
	b.ne	LBB2_5
; %bb.6:                                ;   in Loop: Header=BB2_4 Depth=1
	add	x8, x8, #1
	cmp	x8, x19
	b.ne	LBB2_4
LBB2_7:
	cbnz	w23, LBB2_9
; %bb.8:
Lloh206:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh207:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB2_9:
	mul	x8, x20, x22
	udiv	x19, x8, x23
	bl	_mach_absolute_time
Lloh208:
	adrp	x8, __MergedGlobals@PAGE
Lloh209:
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
	.loh AdrpLdr	Lloh200, Lloh201
	.loh AdrpAdd	Lloh202, Lloh203
	.loh AdrpAdd	Lloh204, Lloh205
	.loh AdrpAdd	Lloh206, Lloh207
	.loh AdrpAdd	Lloh208, Lloh209
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
Lloh210:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh211:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbz	w8, LBB3_6
; %bb.1:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh212:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh213:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbz	x20, LBB3_7
LBB3_2:
Lloh214:
	adrp	x8, _g_atomic_counter@PAGE
Lloh215:
	add	x8, x8, _g_atomic_counter@PAGEOFF
	mov	w9, #1                          ; =0x1
LBB3_3:                                 ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB3_3
; %bb.4:
	cbz	w22, LBB3_8
LBB3_5:
	mul	x8, x19, x21
	udiv	x19, x8, x22
	bl	_mach_absolute_time
Lloh216:
	adrp	x8, __MergedGlobals@PAGE
Lloh217:
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
LBB3_6:
Lloh218:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh219:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	bl	_mach_absolute_time
	mov	x19, x0
Lloh220:
	adrp	x8, __MergedGlobals@PAGE+8
Lloh221:
	add	x8, x8, __MergedGlobals@PAGEOFF+8
	ldp	w21, w22, [x8]
	cbnz	x20, LBB3_2
LBB3_7:
	mov	x23, #0                         ; =0x0
	cbnz	w22, LBB3_5
LBB3_8:
Lloh222:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh223:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
	b	LBB3_5
	.loh AdrpLdr	Lloh210, Lloh211
	.loh AdrpAdd	Lloh212, Lloh213
	.loh AdrpAdd	Lloh214, Lloh215
	.loh AdrpAdd	Lloh216, Lloh217
	.loh AdrpAdd	Lloh220, Lloh221
	.loh AdrpAdd	Lloh218, Lloh219
	.loh AdrpAdd	Lloh222, Lloh223
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
Lloh224:
	adrp	x8, __MergedGlobals@PAGE+12
Lloh225:
	ldr	w8, [x8, __MergedGlobals@PAGEOFF+12]
	cbnz	w8, LBB4_2
; %bb.1:
Lloh226:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh227:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB4_2:
	bl	_mach_absolute_time
	mov	x19, x0
Lloh228:
	adrp	x21, __MergedGlobals@PAGE+8
Lloh229:
	add	x21, x21, __MergedGlobals@PAGEOFF+8
	ldp	w22, w23, [x21]
	mov	x8, #25439                      ; =0x635f
	movk	x8, #14137, lsl #16
	movk	x8, #39645, lsl #32
	movk	x8, #16319, lsl #48
	fmov	d8, x8
	cbz	x20, LBB4_5
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
LBB4_4:                                 ; =>This Inner Loop Header: Depth=1
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
	b.ne	LBB4_4
LBB4_5:
	cbnz	w23, LBB4_7
; %bb.6:
Lloh230:
	adrp	x0, __MergedGlobals@PAGE+8
Lloh231:
	add	x0, x0, __MergedGlobals@PAGEOFF+8
	bl	_mach_timebase_info
LBB4_7:
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
	.loh AdrpLdr	Lloh224, Lloh225
	.loh AdrpAdd	Lloh226, Lloh227
	.loh AdrpAdd	Lloh228, Lloh229
	.loh AdrpAdd	Lloh230, Lloh231
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

.zerofill __DATA,__bss,__MergedGlobals,24,3 ; @_MergedGlobals
.subsections_via_symbols

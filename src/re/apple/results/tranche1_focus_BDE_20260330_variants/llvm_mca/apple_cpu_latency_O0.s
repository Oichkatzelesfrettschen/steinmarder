	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 26, 0	sdk_version 26, 0
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #128
	stp	x29, x30, [sp, #112]            ; 16-byte Folded Spill
	add	x29, sp, #112
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	wzr, [x29, #-4]
	stur	w0, [x29, #-8]
	stur	x1, [x29, #-16]
	mov	x8, #16960                      ; =0x4240
	movk	x8, #15, lsl #16
	stur	x8, [x29, #-24]
	sturb	wzr, [x29, #-25]
                                        ; kill: def $x8 killed $xzr
	stur	xzr, [x29, #-40]
	mov	w8, #1                          ; =0x1
	stur	w8, [x29, #-44]
	b	LBB0_1
LBB0_1:                                 ; =>This Inner Loop Header: Depth=1
	ldur	w8, [x29, #-44]
	ldur	w9, [x29, #-8]
	subs	w8, w8, w9
	b.ge	LBB0_23
	b	LBB0_2
LBB0_2:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldur	x8, [x29, #-16]
	ldursw	x9, [x29, #-44]
	ldr	x0, [x8, x9, lsl #3]
	adrp	x1, l_.str@PAGE
	add	x1, x1, l_.str@PAGEOFF
	bl	_strcmp
	cbnz	w0, LBB0_5
	b	LBB0_3
LBB0_3:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldur	w8, [x29, #-44]
	add	w8, w8, #1
	ldur	w9, [x29, #-8]
	subs	w8, w8, w9
	b.ge	LBB0_5
	b	LBB0_4
LBB0_4:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldur	x8, [x29, #-16]
	ldur	w9, [x29, #-44]
	add	w9, w9, #1
	stur	w9, [x29, #-44]
	ldr	x0, [x8, w9, sxtw #3]
	mov	x1, #0                          ; =0x0
	mov	w2, #10                         ; =0xa
	bl	_strtoull
	stur	x0, [x29, #-24]
	b	LBB0_21
LBB0_5:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldur	x8, [x29, #-16]
	ldursw	x9, [x29, #-44]
	ldr	x0, [x8, x9, lsl #3]
	adrp	x1, l_.str.1@PAGE
	add	x1, x1, l_.str.1@PAGEOFF
	bl	_strcmp
	cbnz	w0, LBB0_8
	b	LBB0_6
LBB0_6:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldur	w8, [x29, #-44]
	add	w8, w8, #1
	ldur	w9, [x29, #-8]
	subs	w8, w8, w9
	b.ge	LBB0_8
	b	LBB0_7
LBB0_7:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldur	x8, [x29, #-16]
	ldur	w9, [x29, #-44]
	add	w9, w9, #1
	stur	w9, [x29, #-44]
	ldr	x8, [x8, w9, sxtw #3]
	stur	x8, [x29, #-40]
	b	LBB0_20
LBB0_8:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldur	x8, [x29, #-16]
	ldursw	x9, [x29, #-44]
	ldr	x0, [x8, x9, lsl #3]
	adrp	x1, l_.str.2@PAGE
	add	x1, x1, l_.str.2@PAGEOFF
	bl	_strcmp
	cbnz	w0, LBB0_10
	b	LBB0_9
LBB0_9:                                 ;   in Loop: Header=BB0_1 Depth=1
	mov	w8, #1                          ; =0x1
	sturb	w8, [x29, #-25]
	b	LBB0_19
LBB0_10:
	ldur	x8, [x29, #-16]
	ldursw	x9, [x29, #-44]
	ldr	x0, [x8, x9, lsl #3]
	adrp	x1, l_.str.3@PAGE
	add	x1, x1, l_.str.3@PAGEOFF
	bl	_strcmp
	cbnz	w0, LBB0_16
	b	LBB0_11
LBB0_11:
	str	xzr, [sp, #56]
	b	LBB0_12
LBB0_12:                                ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #56]
	subs	x8, x8, #7
	b.hs	LBB0_15
	b	LBB0_13
LBB0_13:                                ;   in Loop: Header=BB0_12 Depth=1
	ldr	x8, [sp, #56]
	mov	x9, #24                         ; =0x18
	mul	x9, x8, x9
	adrp	x8, _g_probes@PAGE
	add	x8, x8, _g_probes@PAGEOFF
	ldr	x0, [x8, x9]
	bl	_puts
	b	LBB0_14
LBB0_14:                                ;   in Loop: Header=BB0_12 Depth=1
	ldr	x8, [sp, #56]
	add	x8, x8, #1
	str	x8, [sp, #56]
	b	LBB0_12
LBB0_15:
	stur	wzr, [x29, #-4]
	b	LBB0_38
LBB0_16:
	ldur	x8, [x29, #-16]
	ldursw	x9, [x29, #-44]
	ldr	x0, [x8, x9, lsl #3]
	adrp	x1, l_.str.4@PAGE
	add	x1, x1, l_.str.4@PAGEOFF
	bl	_strcmp
	cbnz	w0, LBB0_18
	b	LBB0_17
LBB0_17:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8]
	mov	x9, sp
	str	x8, [x9]
	adrp	x0, l_.str.5@PAGE
	add	x0, x0, l_.str.5@PAGEOFF
	bl	_printf
	stur	wzr, [x29, #-4]
	b	LBB0_38
LBB0_18:
	adrp	x8, ___stderrp@GOTPAGE
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
	ldr	x0, [x8]
	ldur	x8, [x29, #-16]
	ldursw	x9, [x29, #-44]
	ldr	x8, [x8, x9, lsl #3]
	mov	x9, sp
	str	x8, [x9]
	adrp	x1, l_.str.6@PAGE
	add	x1, x1, l_.str.6@PAGEOFF
	bl	_fprintf
	mov	w8, #1                          ; =0x1
	stur	w8, [x29, #-4]
	b	LBB0_38
LBB0_19:                                ;   in Loop: Header=BB0_1 Depth=1
	b	LBB0_20
LBB0_20:                                ;   in Loop: Header=BB0_1 Depth=1
	b	LBB0_21
LBB0_21:                                ;   in Loop: Header=BB0_1 Depth=1
	b	LBB0_22
LBB0_22:                                ;   in Loop: Header=BB0_1 Depth=1
	ldur	w8, [x29, #-44]
	add	w8, w8, #1
	stur	w8, [x29, #-44]
	b	LBB0_1
LBB0_23:
	ldurb	w8, [x29, #-25]
	tbz	w8, #0, LBB0_25
	b	LBB0_24
LBB0_24:
	adrp	x0, l_.str.7@PAGE
	add	x0, x0, l_.str.7@PAGEOFF
	bl	_printf
	b	LBB0_25
LBB0_25:
	strb	wzr, [sp, #55]
	str	xzr, [sp, #40]
	b	LBB0_26
LBB0_26:                                ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #40]
	subs	x8, x8, #7
	b.hs	LBB0_32
	b	LBB0_27
LBB0_27:                                ;   in Loop: Header=BB0_26 Depth=1
	ldr	x8, [sp, #40]
	mov	x9, #24                         ; =0x18
	mul	x9, x8, x9
	adrp	x8, _g_probes@PAGE
	add	x8, x8, _g_probes@PAGEOFF
	add	x8, x8, x9
	str	x8, [sp, #32]
	ldur	x8, [x29, #-40]
	cbz	x8, LBB0_30
	b	LBB0_28
LBB0_28:                                ;   in Loop: Header=BB0_26 Depth=1
	ldur	x0, [x29, #-40]
	ldr	x8, [sp, #32]
	ldr	x1, [x8]
	bl	_strcmp
	cbz	w0, LBB0_30
	b	LBB0_29
LBB0_29:                                ;   in Loop: Header=BB0_26 Depth=1
	b	LBB0_31
LBB0_30:                                ;   in Loop: Header=BB0_26 Depth=1
	mov	w8, #1                          ; =0x1
	strb	w8, [sp, #55]
	ldr	x8, [sp, #32]
	ldr	x8, [x8]
	str	x8, [sp, #24]                   ; 8-byte Folded Spill
	ldr	x8, [sp, #32]
	ldr	x8, [x8, #8]
	ldur	x0, [x29, #-24]
	blr	x8
	mov	x1, x0
	ldr	x0, [sp, #24]                   ; 8-byte Folded Reload
	ldur	x2, [x29, #-24]
	ldr	x8, [sp, #32]
	ldr	w3, [x8, #16]
	ldurb	w8, [x29, #-25]
	and	w4, w8, #0x1
	bl	_print_result
	b	LBB0_31
LBB0_31:                                ;   in Loop: Header=BB0_26 Depth=1
	ldr	x8, [sp, #40]
	add	x8, x8, #1
	str	x8, [sp, #40]
	b	LBB0_26
LBB0_32:
	ldrb	w8, [sp, #55]
	tbnz	w8, #0, LBB0_37
	b	LBB0_33
LBB0_33:
	adrp	x8, ___stderrp@GOTPAGE
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
	ldr	x8, [x8]
	str	x8, [sp, #16]                   ; 8-byte Folded Spill
	ldur	x8, [x29, #-40]
	cbz	x8, LBB0_35
	b	LBB0_34
LBB0_34:
	ldur	x8, [x29, #-40]
	str	x8, [sp, #8]                    ; 8-byte Folded Spill
	b	LBB0_36
LBB0_35:
	adrp	x8, l_.str.9@PAGE
	add	x8, x8, l_.str.9@PAGEOFF
	str	x8, [sp, #8]                    ; 8-byte Folded Spill
	b	LBB0_36
LBB0_36:
	ldr	x0, [sp, #16]                   ; 8-byte Folded Reload
	ldr	x8, [sp, #8]                    ; 8-byte Folded Reload
	mov	x9, sp
	str	x8, [x9]
	adrp	x1, l_.str.8@PAGE
	add	x1, x1, l_.str.8@PAGEOFF
	bl	_fprintf
	mov	w8, #1                          ; =0x1
	stur	w8, [x29, #-4]
	b	LBB0_38
LBB0_37:
	stur	wzr, [x29, #-4]
	b	LBB0_38
LBB0_38:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #112]            ; 16-byte Folded Reload
	add	sp, sp, #128
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function print_result
_print_result:                          ; @print_result
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #144
	stp	x29, x30, [sp, #128]            ; 16-byte Folded Spill
	add	x29, sp, #128
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	x1, [x29, #-16]
	stur	x2, [x29, #-24]
	stur	w3, [x29, #-28]
	sturb	w4, [x29, #-29]
	ldur	d0, [x29, #-24]
	ucvtf	d0, d0
	ldur	s2, [x29, #-28]
                                        ; implicit-def: $d1
	fmov	s1, s2
	ucvtf	d1, d1
	fmul	d0, d0, d1
	stur	d0, [x29, #-40]
	ldur	d0, [x29, #-40]
	fcmp	d0, #0.0
	b.le	LBB1_2
	b	LBB1_1
LBB1_1:
	ldur	d0, [x29, #-16]
	ucvtf	d0, d0
	ldur	d1, [x29, #-40]
	fdiv	d0, d0, d1
	str	d0, [sp, #56]                   ; 8-byte Folded Spill
	b	LBB1_3
LBB1_2:
	movi	d0, #0000000000000000
	str	d0, [sp, #56]                   ; 8-byte Folded Spill
	b	LBB1_3
LBB1_3:
	ldr	d0, [sp, #56]                   ; 8-byte Folded Reload
	stur	d0, [x29, #-48]
	ldur	x8, [x29, #-16]
	subs	x8, x8, #0
	b.ls	LBB1_5
	b	LBB1_4
LBB1_4:
	ldur	d0, [x29, #-40]
	ldur	d1, [x29, #-16]
	ucvtf	d1, d1
	fdiv	d0, d0, d1
	str	d0, [sp, #48]                   ; 8-byte Folded Spill
	b	LBB1_6
LBB1_5:
	movi	d0, #0000000000000000
	str	d0, [sp, #48]                   ; 8-byte Folded Spill
	b	LBB1_6
LBB1_6:
	ldr	d0, [sp, #48]                   ; 8-byte Folded Reload
	stur	d0, [x29, #-56]
	ldur	d0, [x29, #-56]
	str	d0, [sp, #64]
	ldurb	w8, [x29, #-29]
	tbz	w8, #0, LBB1_8
	b	LBB1_7
LBB1_7:
	ldur	x12, [x29, #-8]
	ldur	x11, [x29, #-24]
	ldur	w8, [x29, #-28]
	mov	x10, x8
	ldur	x9, [x29, #-16]
	ldur	d1, [x29, #-48]
	ldr	d0, [sp, #64]
	mov	x8, sp
	str	x12, [x8]
	str	x11, [x8, #8]
	str	x10, [x8, #16]
	str	x9, [x8, #24]
	str	d1, [x8, #32]
	str	d0, [x8, #40]
	adrp	x0, l_.str.17@PAGE
	add	x0, x0, l_.str.17@PAGEOFF
	bl	_printf
	b	LBB1_9
LBB1_8:
	ldur	x10, [x29, #-8]
	ldur	x9, [x29, #-16]
	ldur	d1, [x29, #-48]
	ldr	d0, [sp, #64]
	mov	x8, sp
	str	x10, [x8]
	str	x9, [x8, #8]
	str	d1, [x8, #16]
	str	d0, [x8, #24]
	adrp	x0, l_.str.18@PAGE
	add	x0, x0, l_.str.18@PAGEOFF
	bl	_printf
	b	LBB1_9
LBB1_9:
	ldp	x29, x30, [sp, #128]            ; 16-byte Folded Reload
	add	sp, sp, #144
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep
_bench_add_dep:                         ; @bench_add_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	mov	x8, #1                          ; =0x1
	stur	x8, [x29, #-16]
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	str	xzr, [sp, #8]
	b	LBB2_1
LBB2_1:                                 ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB2_4
	b	LBB2_2
LBB2_2:                                 ;   in Loop: Header=BB2_1 Depth=1
	ldur	x8, [x29, #-16]
	; InlineAsm Start
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	add	x8, x8, #1
	; InlineAsm End
	stur	x8, [x29, #-16]
	b	LBB2_3
LBB2_3:                                 ;   in Loop: Header=BB2_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #1
	str	x8, [sp, #8]
	b	LBB2_1
LBB2_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #16]
	ldur	x8, [x29, #-16]
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_fadd_dep
_bench_fadd_dep:                        ; @bench_fadd_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	fmov	d0, #1.00000000
	stur	d0, [x29, #-16]
	str	d0, [sp, #24]
	bl	_apple_re_now_ns
	str	x0, [sp, #16]
	str	xzr, [sp]
	b	LBB3_1
LBB3_1:                                 ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB3_4
	b	LBB3_2
LBB3_2:                                 ;   in Loop: Header=BB3_1 Depth=1
	ldur	d0, [x29, #-16]
	fmov	d1, #1.00000000
	; InlineAsm Start
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	fadd	d0, d0, d1
	; InlineAsm End
	stur	d0, [x29, #-16]
	b	LBB3_3
LBB3_3:                                 ;   in Loop: Header=BB3_1 Depth=1
	ldr	x8, [sp]
	add	x8, x8, #1
	str	x8, [sp]
	b	LBB3_1
LBB3_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #8]
	ldur	d0, [x29, #-16]
	adrp	x8, _g_f64_sink@PAGE
	str	d0, [x8, _g_f64_sink@PAGEOFF]
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_fmadd_dep
_bench_fmadd_dep:                       ; @bench_fmadd_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #80
	stp	x29, x30, [sp, #64]             ; 16-byte Folded Spill
	add	x29, sp, #64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	fmov	d0, #1.00000000
	stur	d0, [x29, #-16]
	mov	x8, #536870912                  ; =0x20000000
	movk	x8, #16368, lsl #48
	fmov	d0, x8
	stur	d0, [x29, #-24]
	mov	x8, #281474439839744            ; =0xffffe0000000
	movk	x8, #16367, lsl #48
	fmov	d0, x8
	str	d0, [sp, #32]
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	str	xzr, [sp, #8]
	b	LBB4_1
LBB4_1:                                 ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB4_4
	b	LBB4_2
LBB4_2:                                 ;   in Loop: Header=BB4_1 Depth=1
	ldur	d0, [x29, #-16]
	mov	x8, #536870912                  ; =0x20000000
	movk	x8, #16368, lsl #48
	fmov	d1, x8
	mov	x8, #281474439839744            ; =0xffffe0000000
	movk	x8, #16367, lsl #48
	fmov	d2, x8
	; InlineAsm Start
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	fmadd	d0, d0, d1, d2
	; InlineAsm End
	stur	d0, [x29, #-16]
	b	LBB4_3
LBB4_3:                                 ;   in Loop: Header=BB4_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #1
	str	x8, [sp, #8]
	b	LBB4_1
LBB4_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #16]
	ldur	d0, [x29, #-16]
	adrp	x8, _g_f64_sink@PAGE
	str	d0, [x8, _g_f64_sink@PAGEOFF]
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	add	sp, sp, #80
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_load_store_chain
_bench_load_store_chain:                ; @bench_load_store_chain
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #368
	stp	x28, x27, [sp, #336]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #352]            ; 16-byte Folded Spill
	add	x29, sp, #352
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w27, -24
	.cfi_offset w28, -32
	adrp	x8, ___stack_chk_guard@GOTPAGE
	ldr	x8, [x8, ___stack_chk_guard@GOTPAGEOFF]
	ldr	x8, [x8]
	stur	x8, [x29, #-24]
	str	x0, [sp, #64]
	mov	x8, #31765                      ; =0x7c15
	movk	x8, #32586, lsl #16
	movk	x8, #31161, lsl #32
	movk	x8, #40503, lsl #48
	str	x8, [sp, #56]
	str	wzr, [sp, #36]
	b	LBB5_1
LBB5_1:                                 ; =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #36]
	subs	w8, w8, #32
	b.hs	LBB5_4
	b	LBB5_2
LBB5_2:                                 ;   in Loop: Header=BB5_1 Depth=1
	ldr	w8, [sp, #36]
	add	w8, w8, #1
	mov	w8, w8
                                        ; kill: def $x8 killed $w8
	ldr	w9, [sp, #36]
	mov	x10, x9
	add	x9, sp, #72
	str	x8, [x9, x10, lsl #3]
	b	LBB5_3
LBB5_3:                                 ;   in Loop: Header=BB5_1 Depth=1
	ldr	w8, [sp, #36]
	add	w8, w8, #1
	str	w8, [sp, #36]
	b	LBB5_1
LBB5_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #48]
	str	xzr, [sp, #24]
	b	LBB5_5
LBB5_5:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_7 Depth 2
	ldr	x8, [sp, #24]
	ldr	x9, [sp, #64]
	subs	x8, x8, x9
	b.hs	LBB5_12
	b	LBB5_6
LBB5_6:                                 ;   in Loop: Header=BB5_5 Depth=1
	str	wzr, [sp, #20]
	b	LBB5_7
LBB5_7:                                 ;   Parent Loop BB5_5 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #20]
	subs	w8, w8, #32
	b.hs	LBB5_10
	b	LBB5_8
LBB5_8:                                 ;   in Loop: Header=BB5_7 Depth=2
	ldr	w8, [sp, #20]
                                        ; kill: def $x8 killed $w8
	add	x9, sp, #72
	ldr	x8, [x9, x8, lsl #3]
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	ldr	w10, [sp, #20]
                                        ; kill: def $x10 killed $w10
	add	x10, x8, x10
	ldr	x8, [sp, #56]
	eor	x8, x8, x10
	str	x8, [sp, #56]
	ldr	x10, [sp, #56]
	ldr	x8, [sp, #56]
	lsr	x8, x8, #63
	orr	x8, x8, x10, lsl #1
	str	x8, [sp, #56]
	ldr	x8, [sp, #56]
	ldr	x10, [sp, #8]
	add	x8, x8, x10
	ldr	w10, [sp, #20]
                                        ; kill: def $x10 killed $w10
	str	x8, [x9, x10, lsl #3]
	b	LBB5_9
LBB5_9:                                 ;   in Loop: Header=BB5_7 Depth=2
	ldr	w8, [sp, #20]
	add	w8, w8, #1
	str	w8, [sp, #20]
	b	LBB5_7
LBB5_10:                                ;   in Loop: Header=BB5_5 Depth=1
	b	LBB5_11
LBB5_11:                                ;   in Loop: Header=BB5_5 Depth=1
	ldr	x8, [sp, #24]
	add	x8, x8, #1
	str	x8, [sp, #24]
	b	LBB5_5
LBB5_12:
	bl	_apple_re_now_ns
	str	x0, [sp, #40]
	ldr	x8, [sp, #56]
	ldr	x9, [sp, #56]
	and	x10, x9, #0x1f
	add	x9, sp, #72
	ldr	x9, [x9, x10, lsl #3]
	eor	x8, x8, x9
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp, #40]
	ldr	x9, [sp, #48]
	subs	x8, x8, x9
	str	x8, [sp]                        ; 8-byte Folded Spill
	ldur	x9, [x29, #-24]
	adrp	x8, ___stack_chk_guard@GOTPAGE
	ldr	x8, [x8, ___stack_chk_guard@GOTPAGEOFF]
	ldr	x8, [x8]
	subs	x8, x8, x9
	b.eq	LBB5_14
	b	LBB5_13
LBB5_13:
	bl	___stack_chk_fail
LBB5_14:
	ldr	x0, [sp]                        ; 8-byte Folded Reload
	ldp	x29, x30, [sp, #352]            ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #336]            ; 16-byte Folded Reload
	add	sp, sp, #368
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_shuffle_dep
_bench_shuffle_dep:                     ; @bench_shuffle_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #80
	stp	x29, x30, [sp, #64]             ; 16-byte Folded Spill
	add	x29, sp, #64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	mov	x8, #52719                      ; =0xcdef
	movk	x8, #35243, lsl #16
	movk	x8, #17767, lsl #32
	movk	x8, #291, lsl #48
	stur	x8, [x29, #-16]
	bl	_apple_re_now_ns
	stur	x0, [x29, #-24]
	str	xzr, [sp, #24]
	b	LBB6_1
LBB6_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB6_3 Depth 2
	ldr	x8, [sp, #24]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB6_8
	b	LBB6_2
LBB6_2:                                 ;   in Loop: Header=BB6_1 Depth=1
	str	wzr, [sp, #20]
	b	LBB6_3
LBB6_3:                                 ;   Parent Loop BB6_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #20]
	subs	w8, w8, #32
	b.hs	LBB6_6
	b	LBB6_4
LBB6_4:                                 ;   in Loop: Header=BB6_3 Depth=2
	ldur	x8, [x29, #-16]
	ldr	w9, [sp, #20]
                                        ; kill: def $x9 killed $w9
	mov	x10, #31765                     ; =0x7c15
	movk	x10, #32586, lsl #16
	movk	x10, #31161, lsl #32
	movk	x10, #40503, lsl #48
	mul	x9, x9, x10
	eor	x8, x8, x9
	str	x8, [sp, #8]
	ldr	w8, [sp, #20]
	and	w8, w8, #0xf
	add	w8, w8, #1
	str	w8, [sp, #4]
	ldr	x8, [sp, #8]
	ldr	w9, [sp, #4]
                                        ; kill: def $x9 killed $w9
	lsr	x8, x8, x9
	ldr	x9, [sp, #8]
	ldr	w11, [sp, #4]
	mov	w10, #64                        ; =0x40
	subs	w10, w10, w11
                                        ; kill: def $x10 killed $w10
	lsl	x9, x9, x10
	orr	x8, x8, x9
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	rev	x8, x8
	stur	x8, [x29, #-16]
	b	LBB6_5
LBB6_5:                                 ;   in Loop: Header=BB6_3 Depth=2
	ldr	w8, [sp, #20]
	add	w8, w8, #1
	str	w8, [sp, #20]
	b	LBB6_3
LBB6_6:                                 ;   in Loop: Header=BB6_1 Depth=1
	b	LBB6_7
LBB6_7:                                 ;   in Loop: Header=BB6_1 Depth=1
	ldr	x8, [sp, #24]
	add	x8, x8, #1
	str	x8, [sp, #24]
	b	LBB6_1
LBB6_8:
	bl	_apple_re_now_ns
	str	x0, [sp, #32]
	ldur	x8, [x29, #-16]
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp, #32]
	ldur	x9, [x29, #-24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	add	sp, sp, #80
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_atomic_add_dep
_bench_atomic_add_dep:                  ; @bench_atomic_add_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #96
	stp	x29, x30, [sp, #80]             ; 16-byte Folded Spill
	add	x29, sp, #80
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	stur	xzr, [x29, #-16]
	mov	x8, #1                          ; =0x1
	str	x8, [sp, #40]
	ldr	x8, [sp, #40]
	adrp	x9, _g_atomic_counter@PAGE
	str	x8, [x9, _g_atomic_counter@PAGEOFF]
	bl	_apple_re_now_ns
	stur	x0, [x29, #-24]
	str	xzr, [sp, #32]
	b	LBB7_1
LBB7_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB7_3 Depth 2
	ldr	x8, [sp, #32]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB7_8
	b	LBB7_2
LBB7_2:                                 ;   in Loop: Header=BB7_1 Depth=1
	str	wzr, [sp, #28]
	b	LBB7_3
LBB7_3:                                 ;   Parent Loop BB7_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #28]
	subs	w8, w8, #32
	b.hs	LBB7_6
	b	LBB7_4
LBB7_4:                                 ;   in Loop: Header=BB7_3 Depth=2
	mov	x8, #1                          ; =0x1
	str	x8, [sp, #16]
	ldr	x8, [sp, #16]
	adrp	x9, _g_atomic_counter@PAGE
	add	x9, x9, _g_atomic_counter@PAGEOFF
	ldadd	x8, x8, [x9]
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	stur	x8, [x29, #-16]
	b	LBB7_5
LBB7_5:                                 ;   in Loop: Header=BB7_3 Depth=2
	ldr	w8, [sp, #28]
	add	w8, w8, #1
	str	w8, [sp, #28]
	b	LBB7_3
LBB7_6:                                 ;   in Loop: Header=BB7_1 Depth=1
	b	LBB7_7
LBB7_7:                                 ;   in Loop: Header=BB7_1 Depth=1
	ldr	x8, [sp, #32]
	add	x8, x8, #1
	str	x8, [sp, #32]
	b	LBB7_1
LBB7_8:
	bl	_apple_re_now_ns
	stur	x0, [x29, #-32]
	ldur	x8, [x29, #-16]
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldur	x8, [x29, #-32]
	ldur	x9, [x29, #-24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #80]             ; 16-byte Folded Reload
	add	sp, sp, #96
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_transcendental_dep
_bench_transcendental_dep:              ; @bench_transcendental_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #80
	stp	x29, x30, [sp, #64]             ; 16-byte Folded Spill
	add	x29, sp, #64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	mov	x8, #25439                      ; =0x635f
	movk	x8, #14137, lsl #16
	movk	x8, #39645, lsl #32
	movk	x8, #16319, lsl #48
	fmov	d0, x8
	stur	d0, [x29, #-16]
	bl	_apple_re_now_ns
	stur	x0, [x29, #-24]
	str	xzr, [sp, #24]
	b	LBB8_1
LBB8_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB8_3 Depth 2
	ldr	x8, [sp, #24]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB8_8
	b	LBB8_2
LBB8_2:                                 ;   in Loop: Header=BB8_1 Depth=1
	str	wzr, [sp, #20]
	b	LBB8_3
LBB8_3:                                 ;   Parent Loop BB8_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #20]
	subs	w8, w8, #4
	b.hs	LBB8_6
	b	LBB8_4
LBB8_4:                                 ;   in Loop: Header=BB8_3 Depth=2
	ldur	d2, [x29, #-16]
	ldr	w8, [sp, #20]
	add	w8, w8, #1
	ucvtf	d0, w8
	mov	x8, #43516                      ; =0xa9fc
	movk	x8, #54001, lsl #16
	movk	x8, #25165, lsl #32
	movk	x8, #16208, lsl #48
	fmov	d1, x8
	fmadd	d0, d0, d1, d2
	bl	_sin
	str	d0, [sp, #8]
	ldr	d0, [sp, #8]
	ldur	d1, [x29, #-16]
	fadd	d0, d0, d1
	bl	_cos
	stur	d0, [x29, #-16]
	b	LBB8_5
LBB8_5:                                 ;   in Loop: Header=BB8_3 Depth=2
	ldr	w8, [sp, #20]
	add	w8, w8, #1
	str	w8, [sp, #20]
	b	LBB8_3
LBB8_6:                                 ;   in Loop: Header=BB8_1 Depth=1
	b	LBB8_7
LBB8_7:                                 ;   in Loop: Header=BB8_1 Depth=1
	ldr	x8, [sp, #24]
	add	x8, x8, #1
	str	x8, [sp, #24]
	b	LBB8_1
LBB8_8:
	bl	_apple_re_now_ns
	str	x0, [sp, #32]
	ldur	d0, [x29, #-16]
	adrp	x8, _g_f64_sink@PAGE
	str	d0, [x8, _g_f64_sink@PAGEOFF]
	ldr	x8, [sp, #32]
	ldur	x9, [x29, #-24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	add	sp, sp, #80
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function apple_re_now_ns
_apple_re_now_ns:                       ; @apple_re_now_ns
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #32
	stp	x29, x30, [sp, #16]             ; 16-byte Folded Spill
	add	x29, sp, #16
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	adrp	x8, _apple_re_now_ns.timebase@PAGE
	add	x8, x8, _apple_re_now_ns.timebase@PAGEOFF
	ldr	w8, [x8, #4]
	cbnz	w8, LBB9_2
	b	LBB9_1
LBB9_1:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
	b	LBB9_2
LBB9_2:
	bl	_mach_absolute_time
	str	x0, [sp, #8]
	ldr	x8, [sp, #8]
	adrp	x10, _apple_re_now_ns.timebase@PAGE
	adrp	x9, _apple_re_now_ns.timebase@PAGE
	add	x9, x9, _apple_re_now_ns.timebase@PAGEOFF
	ldr	w10, [x10, _apple_re_now_ns.timebase@PAGEOFF]
                                        ; kill: def $x10 killed $w10
	mul	x8, x8, x10
	ldr	w9, [x9, #4]
                                        ; kill: def $x9 killed $w9
	udiv	x0, x8, x9
	ldp	x29, x30, [sp, #16]             ; 16-byte Folded Reload
	add	sp, sp, #32
	ret
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

l_.str.7:                               ; @.str.7
	.asciz	"probe,iters,unroll,elapsed_ns,ns_per_op,approx_gops\n"

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

.subsections_via_symbols

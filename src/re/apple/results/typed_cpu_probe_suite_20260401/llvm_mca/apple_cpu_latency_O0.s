	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 26, 0	sdk_version 26, 4
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
	subs	x8, x8, #23
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
	subs	x8, x8, #23
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
	adrp	x0, l_.str.33@PAGE
	add	x0, x0, l_.str.33@PAGEOFF
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
	adrp	x0, l_.str.34@PAGE
	add	x0, x0, l_.str.34@PAGEOFF
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
	.p2align	2                               ; -- Begin function bench_add_dep_u32
_bench_add_dep_u32:                     ; @bench_add_dep_u32
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	mov	w8, #1                          ; =0x1
	stur	w8, [x29, #-12]
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	str	xzr, [sp, #16]
	b	LBB3_1
LBB3_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB3_3 Depth 2
	ldr	x8, [sp, #16]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB3_8
	b	LBB3_2
LBB3_2:                                 ;   in Loop: Header=BB3_1 Depth=1
	str	wzr, [sp, #12]
	b	LBB3_3
LBB3_3:                                 ;   Parent Loop BB3_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #12]
	subs	w8, w8, #32
	b.ge	LBB3_6
	b	LBB3_4
LBB3_4:                                 ;   in Loop: Header=BB3_3 Depth=2
	ldur	w8, [x29, #-12]
	add	w8, w8, #1
	stur	w8, [x29, #-12]
	b	LBB3_5
LBB3_5:                                 ;   in Loop: Header=BB3_3 Depth=2
	ldr	w8, [sp, #12]
	add	w8, w8, #1
	str	w8, [sp, #12]
	b	LBB3_3
LBB3_6:                                 ;   in Loop: Header=BB3_1 Depth=1
	b	LBB3_7
LBB3_7:                                 ;   in Loop: Header=BB3_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #1
	str	x8, [sp, #16]
	b	LBB3_1
LBB3_8:
	bl	_apple_re_now_ns
	str	x0, [sp]
	ldur	w8, [x29, #-12]
                                        ; kill: def $x8 killed $w8
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp]
	ldr	x9, [sp, #24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep_i64
_bench_add_dep_i64:                     ; @bench_add_dep_i64
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
	str	xzr, [sp, #16]
	b	LBB4_1
LBB4_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB4_3 Depth 2
	ldr	x8, [sp, #16]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB4_8
	b	LBB4_2
LBB4_2:                                 ;   in Loop: Header=BB4_1 Depth=1
	str	wzr, [sp, #12]
	b	LBB4_3
LBB4_3:                                 ;   Parent Loop BB4_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #12]
	subs	w8, w8, #32
	b.ge	LBB4_6
	b	LBB4_4
LBB4_4:                                 ;   in Loop: Header=BB4_3 Depth=2
	ldur	x8, [x29, #-16]
	add	x8, x8, #1
	stur	x8, [x29, #-16]
	b	LBB4_5
LBB4_5:                                 ;   in Loop: Header=BB4_3 Depth=2
	ldr	w8, [sp, #12]
	add	w8, w8, #1
	str	w8, [sp, #12]
	b	LBB4_3
LBB4_6:                                 ;   in Loop: Header=BB4_1 Depth=1
	b	LBB4_7
LBB4_7:                                 ;   in Loop: Header=BB4_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #1
	str	x8, [sp, #16]
	b	LBB4_1
LBB4_8:
	bl	_apple_re_now_ns
	str	x0, [sp]
	ldur	x8, [x29, #-16]
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp]
	ldr	x9, [sp, #24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep_i16
_bench_add_dep_i16:                     ; @bench_add_dep_i16
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	mov	w8, #1                          ; =0x1
	sturh	w8, [x29, #-10]
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	str	xzr, [sp, #16]
	b	LBB5_1
LBB5_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB5_3 Depth 2
	ldr	x8, [sp, #16]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB5_8
	b	LBB5_2
LBB5_2:                                 ;   in Loop: Header=BB5_1 Depth=1
	str	wzr, [sp, #12]
	b	LBB5_3
LBB5_3:                                 ;   Parent Loop BB5_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #12]
	subs	w8, w8, #32
	b.ge	LBB5_6
	b	LBB5_4
LBB5_4:                                 ;   in Loop: Header=BB5_3 Depth=2
	ldursh	w8, [x29, #-10]
	add	w8, w8, #1
	sturh	w8, [x29, #-10]
	b	LBB5_5
LBB5_5:                                 ;   in Loop: Header=BB5_3 Depth=2
	ldr	w8, [sp, #12]
	add	w8, w8, #1
	str	w8, [sp, #12]
	b	LBB5_3
LBB5_6:                                 ;   in Loop: Header=BB5_1 Depth=1
	b	LBB5_7
LBB5_7:                                 ;   in Loop: Header=BB5_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #1
	str	x8, [sp, #16]
	b	LBB5_1
LBB5_8:
	bl	_apple_re_now_ns
	str	x0, [sp]
	ldurh	w8, [x29, #-10]
                                        ; kill: def $x8 killed $w8
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp]
	ldr	x9, [sp, #24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep_i8
_bench_add_dep_i8:                      ; @bench_add_dep_i8
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	mov	w8, #1                          ; =0x1
	sturb	w8, [x29, #-9]
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	str	xzr, [sp, #16]
	b	LBB6_1
LBB6_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB6_3 Depth 2
	ldr	x8, [sp, #16]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB6_8
	b	LBB6_2
LBB6_2:                                 ;   in Loop: Header=BB6_1 Depth=1
	str	wzr, [sp, #12]
	b	LBB6_3
LBB6_3:                                 ;   Parent Loop BB6_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #12]
	subs	w8, w8, #32
	b.ge	LBB6_6
	b	LBB6_4
LBB6_4:                                 ;   in Loop: Header=BB6_3 Depth=2
	ldursb	w8, [x29, #-9]
	add	w8, w8, #1
	sturb	w8, [x29, #-9]
	b	LBB6_5
LBB6_5:                                 ;   in Loop: Header=BB6_3 Depth=2
	ldr	w8, [sp, #12]
	add	w8, w8, #1
	str	w8, [sp, #12]
	b	LBB6_3
LBB6_6:                                 ;   in Loop: Header=BB6_1 Depth=1
	b	LBB6_7
LBB6_7:                                 ;   in Loop: Header=BB6_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #1
	str	x8, [sp, #16]
	b	LBB6_1
LBB6_8:
	bl	_apple_re_now_ns
	str	x0, [sp]
	ldurb	w8, [x29, #-9]
                                        ; kill: def $x8 killed $w8
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp]
	ldr	x9, [sp, #24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep_u16
_bench_add_dep_u16:                     ; @bench_add_dep_u16
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	mov	w8, #1                          ; =0x1
	sturh	w8, [x29, #-10]
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	str	xzr, [sp, #16]
	b	LBB7_1
LBB7_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB7_3 Depth 2
	ldr	x8, [sp, #16]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB7_8
	b	LBB7_2
LBB7_2:                                 ;   in Loop: Header=BB7_1 Depth=1
	str	wzr, [sp, #12]
	b	LBB7_3
LBB7_3:                                 ;   Parent Loop BB7_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #12]
	subs	w8, w8, #32
	b.ge	LBB7_6
	b	LBB7_4
LBB7_4:                                 ;   in Loop: Header=BB7_3 Depth=2
	ldurh	w8, [x29, #-10]
	add	w8, w8, #1
	sturh	w8, [x29, #-10]
	b	LBB7_5
LBB7_5:                                 ;   in Loop: Header=BB7_3 Depth=2
	ldr	w8, [sp, #12]
	add	w8, w8, #1
	str	w8, [sp, #12]
	b	LBB7_3
LBB7_6:                                 ;   in Loop: Header=BB7_1 Depth=1
	b	LBB7_7
LBB7_7:                                 ;   in Loop: Header=BB7_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #1
	str	x8, [sp, #16]
	b	LBB7_1
LBB7_8:
	bl	_apple_re_now_ns
	str	x0, [sp]
	ldurh	w8, [x29, #-10]
                                        ; kill: def $x8 killed $w8
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp]
	ldr	x9, [sp, #24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_add_dep_u8
_bench_add_dep_u8:                      ; @bench_add_dep_u8
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	mov	w8, #1                          ; =0x1
	sturb	w8, [x29, #-9]
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	str	xzr, [sp, #16]
	b	LBB8_1
LBB8_1:                                 ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB8_3 Depth 2
	ldr	x8, [sp, #16]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB8_8
	b	LBB8_2
LBB8_2:                                 ;   in Loop: Header=BB8_1 Depth=1
	str	wzr, [sp, #12]
	b	LBB8_3
LBB8_3:                                 ;   Parent Loop BB8_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #12]
	subs	w8, w8, #32
	b.ge	LBB8_6
	b	LBB8_4
LBB8_4:                                 ;   in Loop: Header=BB8_3 Depth=2
	ldurb	w8, [x29, #-9]
	add	w8, w8, #1
	sturb	w8, [x29, #-9]
	b	LBB8_5
LBB8_5:                                 ;   in Loop: Header=BB8_3 Depth=2
	ldr	w8, [sp, #12]
	add	w8, w8, #1
	str	w8, [sp, #12]
	b	LBB8_3
LBB8_6:                                 ;   in Loop: Header=BB8_1 Depth=1
	b	LBB8_7
LBB8_7:                                 ;   in Loop: Header=BB8_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #1
	str	x8, [sp, #16]
	b	LBB8_1
LBB8_8:
	bl	_apple_re_now_ns
	str	x0, [sp]
	ldurb	w8, [x29, #-9]
                                        ; kill: def $x8 killed $w8
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp]
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
	b	LBB9_1
LBB9_1:                                 ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB9_4
	b	LBB9_2
LBB9_2:                                 ;   in Loop: Header=BB9_1 Depth=1
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
	b	LBB9_3
LBB9_3:                                 ;   in Loop: Header=BB9_1 Depth=1
	ldr	x8, [sp]
	add	x8, x8, #1
	str	x8, [sp]
	b	LBB9_1
LBB9_4:
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
	.p2align	2                               ; -- Begin function bench_fadd_f32_dep
_bench_fadd_f32_dep:                    ; @bench_fadd_f32_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	fmov	s0, #1.00000000
	stur	s0, [x29, #-12]
	stur	s0, [x29, #-16]
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	str	xzr, [sp, #8]
	b	LBB10_1
LBB10_1:                                ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB10_4
	b	LBB10_2
LBB10_2:                                ;   in Loop: Header=BB10_1 Depth=1
	ldur	s0, [x29, #-12]
	fmov	s1, #1.00000000
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
	stur	s0, [x29, #-12]
	b	LBB10_3
LBB10_3:                                ;   in Loop: Header=BB10_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #1
	str	x8, [sp, #8]
	b	LBB10_1
LBB10_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #16]
	ldur	s0, [x29, #-12]
	fcvt	d0, s0
	adrp	x8, _g_f64_sink@PAGE
	str	d0, [x8, _g_f64_sink@PAGEOFF]
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #24]
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
	b	LBB11_1
LBB11_1:                                ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB11_4
	b	LBB11_2
LBB11_2:                                ;   in Loop: Header=BB11_1 Depth=1
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
	b	LBB11_3
LBB11_3:                                ;   in Loop: Header=BB11_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #1
	str	x8, [sp, #8]
	b	LBB11_1
LBB11_4:
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
	.p2align	2                               ; -- Begin function bench_bfloat16_roundtrip_proxy
_bench_bfloat16_roundtrip_proxy:        ; @bench_bfloat16_roundtrip_proxy
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	fmov	s0, #1.00000000
	stur	s0, [x29, #-12]
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	str	xzr, [sp, #8]
	b	LBB12_1
LBB12_1:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB12_3 Depth 2
	ldr	x8, [sp, #8]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB12_8
	b	LBB12_2
LBB12_2:                                ;   in Loop: Header=BB12_1 Depth=1
	str	wzr, [sp, #4]
	b	LBB12_3
LBB12_3:                                ;   Parent Loop BB12_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #4]
	subs	w8, w8, #32
	b.ge	LBB12_6
	b	LBB12_4
LBB12_4:                                ;   in Loop: Header=BB12_3 Depth=2
	ldur	s0, [x29, #-12]
	fmov	s1, #0.12500000
	fadd	s0, s0, s1
	bl	_quantize_bfloat16_proxy
	stur	s0, [x29, #-12]
	b	LBB12_5
LBB12_5:                                ;   in Loop: Header=BB12_3 Depth=2
	ldr	w8, [sp, #4]
	add	w8, w8, #1
	str	w8, [sp, #4]
	b	LBB12_3
LBB12_6:                                ;   in Loop: Header=BB12_1 Depth=1
	b	LBB12_7
LBB12_7:                                ;   in Loop: Header=BB12_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #1
	str	x8, [sp, #8]
	b	LBB12_1
LBB12_8:
	bl	_apple_re_now_ns
	str	x0, [sp, #16]
	ldur	s0, [x29, #-12]
	fcvt	d0, s0
	adrp	x8, _g_f64_sink@PAGE
	str	d0, [x8, _g_f64_sink@PAGEOFF]
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_fcvt_f32_to_f16_dep
_bench_fcvt_f32_to_f16_dep:             ; @bench_fcvt_f32_to_f16_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	fmov	s0, #1.00000000
	stur	s0, [x29, #-12]
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	str	xzr, [sp, #8]
	b	LBB13_1
LBB13_1:                                ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB13_4
	b	LBB13_2
LBB13_2:                                ;   in Loop: Header=BB13_1 Depth=1
	ldur	s0, [x29, #-12]
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
	stur	s0, [x29, #-12]
	b	LBB13_3
LBB13_3:                                ;   in Loop: Header=BB13_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #1
	str	x8, [sp, #8]
	b	LBB13_1
LBB13_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #16]
	ldur	s0, [x29, #-12]
	fcvt	d0, s0
	adrp	x8, _g_f64_sink@PAGE
	str	d0, [x8, _g_f64_sink@PAGEOFF]
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
	ret
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__const
	.p2align	1, 0x0                          ; -- Begin function bench_fadd_f16_dep
lCPI14_0:
	.short	0x1419                          ; half 0.0010004
	.section	__TEXT,__text,regular,pure_instructions
	.p2align	2
_bench_fadd_f16_dep:                    ; @bench_fadd_f16_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	fmov	h0, #1.00000000
	stur	h0, [x29, #-10]
	adrp	x8, lCPI14_0@PAGE
	ldr	h0, [x8, lCPI14_0@PAGEOFF]
	stur	h0, [x29, #-12]
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	str	xzr, [sp, #8]
	b	LBB14_1
LBB14_1:                                ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #8]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB14_4
	b	LBB14_2
LBB14_2:                                ;   in Loop: Header=BB14_1 Depth=1
	ldur	h0, [x29, #-10]
	adrp	x8, lCPI14_0@PAGE
	ldr	h1, [x8, lCPI14_0@PAGEOFF]
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
	stur	h0, [x29, #-10]
	b	LBB14_3
LBB14_3:                                ;   in Loop: Header=BB14_1 Depth=1
	ldr	x8, [sp, #8]
	add	x8, x8, #1
	str	x8, [sp, #8]
	b	LBB14_1
LBB14_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #16]
	ldur	h0, [x29, #-10]
	fcvt	s0, h0
	fcvt	d0, s0
	adrp	x8, _g_f64_sink@PAGE
	str	d0, [x8, _g_f64_sink@PAGEOFF]
	ldr	x8, [sp, #16]
	ldr	x9, [sp, #24]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
	ret
	.cfi_endproc
                                        ; -- End function
	.section	__TEXT,__const
	.p2align	1, 0x0                          ; -- Begin function bench_fmla_f16x8_dep
lCPI15_0:
	.short	0x068e                          ; half 1.0002E-4
	.section	__TEXT,__text,regular,pure_instructions
	.p2align	2
_bench_fmla_f16x8_dep:                  ; @bench_fmla_f16x8_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #416
	stp	x28, x27, [sp, #384]            ; 16-byte Folded Spill
	stp	x29, x30, [sp, #400]            ; 16-byte Folded Spill
	add	x29, sp, #400
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w27, -24
	.cfi_offset w28, -32
	add	x8, sp, #176
	str	x8, [sp]                        ; 8-byte Folded Spill
                                        ; implicit-def: $q0
	str	x0, [x8, #200]
	fmov	h1, #1.00000000
	stur	h1, [x29, #-66]
	ldur	h2, [x29, #-66]
                                        ; implicit-def: $q3
	fmov	s3, s2
	mov.16b	v2, v0
	mov.h	v2[0], v3[0]
	ldur	h4, [x29, #-66]
                                        ; implicit-def: $q3
	fmov	s3, s4
	mov.h	v2[1], v3[0]
	ldur	h4, [x29, #-66]
                                        ; implicit-def: $q3
	fmov	s3, s4
	mov.h	v2[2], v3[0]
	ldur	h4, [x29, #-66]
                                        ; implicit-def: $q3
	fmov	s3, s4
	mov.h	v2[3], v3[0]
	ldur	h4, [x29, #-66]
                                        ; implicit-def: $q3
	fmov	s3, s4
	mov.h	v2[4], v3[0]
	ldur	h4, [x29, #-66]
                                        ; implicit-def: $q3
	fmov	s3, s4
	mov.h	v2[5], v3[0]
	ldur	h4, [x29, #-66]
                                        ; implicit-def: $q3
	fmov	s3, s4
	mov.h	v2[6], v3[0]
	ldur	h4, [x29, #-66]
                                        ; implicit-def: $q3
	fmov	s3, s4
	mov.h	v2[7], v3[0]
	str	q2, [x8, #128]
	ldr	q2, [x8, #128]
	str	q2, [x8, #160]
	ldr	q2, [x8, #160]
	str	q2, [x8, #112]
	ldr	q2, [x8, #112]
	str	q2, [x8, #176]
	sub	x9, x29, #146
	str	x9, [sp, #8]                    ; 8-byte Folded Spill
	stur	h1, [x29, #-146]
	mov.16b	v1, v0
	ld1.h	{ v1 }[0], [x9]
	ld1.h	{ v1 }[1], [x9]
	ld1.h	{ v1 }[2], [x9]
	ld1.h	{ v1 }[3], [x9]
	ld1.h	{ v1 }[4], [x9]
	ld1.h	{ v1 }[5], [x9]
	ld1.h	{ v1 }[6], [x9]
	ld1.h	{ v1 }[7], [x9]
	str	q1, [x8, #48]
	ldr	q1, [x8, #48]
	str	q1, [x8, #80]
	ldr	q1, [x8, #80]
	str	q1, [x8, #32]
	ldr	q1, [x8, #32]
	str	q1, [x8, #96]
	add	x9, sp, #174
	str	x9, [sp, #16]                   ; 8-byte Folded Spill
	adrp	x10, lCPI15_0@PAGE
	ldr	h1, [x10, lCPI15_0@PAGEOFF]
	str	h1, [sp, #174]
	ld1.h	{ v0 }[0], [x9]
	ld1.h	{ v0 }[1], [x9]
	ld1.h	{ v0 }[2], [x9]
	ld1.h	{ v0 }[3], [x9]
	ld1.h	{ v0 }[4], [x9]
	ld1.h	{ v0 }[5], [x9]
	ld1.h	{ v0 }[6], [x9]
	ld1.h	{ v0 }[7], [x9]
	str	q0, [sp, #144]
	ldr	q0, [sp, #144]
	str	q0, [x8]
	ldr	q0, [x8]
	str	q0, [sp, #128]
	ldr	q0, [sp, #128]
	str	q0, [x8, #16]
	bl	_apple_re_now_ns
	str	x0, [sp, #120]
	str	xzr, [sp, #104]
	b	LBB15_1
LBB15_1:                                ; =>This Inner Loop Header: Depth=1
	ldr	x9, [sp]                        ; 8-byte Folded Reload
	ldr	x8, [sp, #104]
	ldr	x9, [x9, #200]
	subs	x8, x8, x9
	b.hs	LBB15_4
	b	LBB15_2
LBB15_2:                                ;   in Loop: Header=BB15_1 Depth=1
	ldr	x8, [sp]                        ; 8-byte Folded Reload
	ldr	q0, [x8, #176]
	ldr	q1, [x8, #96]
	ldr	q2, [x8, #16]
	; InlineAsm Start
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	fmla.8h	v0, v1, v2
	; InlineAsm End
	str	q0, [x8, #176]
	b	LBB15_3
LBB15_3:                                ;   in Loop: Header=BB15_1 Depth=1
	ldr	x8, [sp, #104]
	add	x8, x8, #1
	str	x8, [sp, #104]
	b	LBB15_1
LBB15_4:
	bl	_apple_re_now_ns
	ldr	x8, [sp]                        ; 8-byte Folded Reload
	str	x0, [sp, #112]
	ldr	q0, [x8, #176]
	str	q0, [sp, #80]
	ldr	q0, [sp, #80]
	str	q0, [sp, #64]
	ldr	q0, [sp, #64]
	str	q0, [sp, #32]
	ldr	q0, [sp, #32]
                                        ; kill: def $h0 killed $h0 killed $q0
	str	h0, [sp, #60]
	ldrh	w8, [sp, #60]
	strh	w8, [sp, #30]
	ldrh	w8, [sp, #30]
	strh	w8, [sp, #62]
	ldr	h0, [sp, #62]
	str	h0, [sp, #102]
	ldr	h0, [sp, #102]
	str	h0, [sp, #28]
	ldr	h0, [sp, #28]
	fcvt	s0, h0
	fcvt	d0, s0
	adrp	x8, _g_f64_sink@PAGE
	str	d0, [x8, _g_f64_sink@PAGEOFF]
	ldr	x8, [sp, #112]
	ldr	x9, [sp, #120]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #400]            ; 16-byte Folded Reload
	ldp	x28, x27, [sp, #384]            ; 16-byte Folded Reload
	add	sp, sp, #416
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_mul_dep
_bench_mul_dep:                         ; @bench_mul_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #80
	stp	x29, x30, [sp, #64]             ; 16-byte Folded Spill
	add	x29, sp, #64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	mov	x8, #322                        ; =0x142
	movk	x8, #1979, lsl #16
	movk	x8, #10030, lsl #32
	movk	x8, #27746, lsl #48
	str	x8, [sp, #8]                    ; 8-byte Folded Spill
	stur	x0, [x29, #-8]
	mov	x9, #31765                      ; =0x7c15
	movk	x9, #32586, lsl #16
	movk	x9, #31161, lsl #32
	movk	x9, #40503, lsl #48
	stur	x9, [x29, #-16]
	stur	x8, [x29, #-24]
	bl	_apple_re_now_ns
	str	x0, [sp, #32]
	str	xzr, [sp, #16]
	b	LBB16_1
LBB16_1:                                ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #16]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB16_4
	b	LBB16_2
LBB16_2:                                ;   in Loop: Header=BB16_1 Depth=1
	ldr	x9, [sp, #8]                    ; 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	; InlineAsm Start
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	mul	x8, x8, x9
	; InlineAsm End
	stur	x8, [x29, #-16]
	b	LBB16_3
LBB16_3:                                ;   in Loop: Header=BB16_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #1
	str	x8, [sp, #16]
	b	LBB16_1
LBB16_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	ldur	x8, [x29, #-16]
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp, #24]
	ldr	x9, [sp, #32]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	add	sp, sp, #80
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_madd_dep
_bench_madd_dep:                        ; @bench_madd_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #80
	stp	x29, x30, [sp, #64]             ; 16-byte Folded Spill
	add	x29, sp, #64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	mov	x8, #322                        ; =0x142
	movk	x8, #1979, lsl #16
	movk	x8, #10030, lsl #32
	movk	x8, #27746, lsl #48
	str	x8, [sp, #8]                    ; 8-byte Folded Spill
	stur	x0, [x29, #-8]
	mov	x9, #31765                      ; =0x7c15
	movk	x9, #32586, lsl #16
	movk	x9, #31161, lsl #32
	movk	x9, #40503, lsl #48
	stur	x9, [x29, #-16]
	stur	x8, [x29, #-24]
	bl	_apple_re_now_ns
	str	x0, [sp, #32]
	str	xzr, [sp, #16]
	b	LBB17_1
LBB17_1:                                ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #16]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB17_4
	b	LBB17_2
LBB17_2:                                ;   in Loop: Header=BB17_1 Depth=1
	ldr	x9, [sp, #8]                    ; 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	; InlineAsm Start
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	madd	x8, x8, x9, x8
	; InlineAsm End
	stur	x8, [x29, #-16]
	b	LBB17_3
LBB17_3:                                ;   in Loop: Header=BB17_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #1
	str	x8, [sp, #16]
	b	LBB17_1
LBB17_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	ldur	x8, [x29, #-16]
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp, #24]
	ldr	x9, [sp, #32]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	add	sp, sp, #80
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_msub_dep
_bench_msub_dep:                        ; @bench_msub_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	mov	x8, #31765                      ; =0x7c15
	movk	x8, #32586, lsl #16
	movk	x8, #31161, lsl #32
	movk	x8, #40503, lsl #48
	stur	x8, [x29, #-16]
	mov	x8, #3                          ; =0x3
	str	x8, [sp, #24]
	bl	_apple_re_now_ns
	str	x0, [sp, #16]
	str	xzr, [sp]
	b	LBB18_1
LBB18_1:                                ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB18_4
	b	LBB18_2
LBB18_2:                                ;   in Loop: Header=BB18_1 Depth=1
	ldur	x8, [x29, #-16]
	mov	x9, #3                          ; =0x3
	; InlineAsm Start
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	msub	x8, x8, x9, x8
	; InlineAsm End
	stur	x8, [x29, #-16]
	b	LBB18_3
LBB18_3:                                ;   in Loop: Header=BB18_1 Depth=1
	ldr	x8, [sp]
	add	x8, x8, #1
	str	x8, [sp]
	b	LBB18_1
LBB18_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #8]
	ldur	x8, [x29, #-16]
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_umulh_dep
_bench_umulh_dep:                       ; @bench_umulh_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #80
	stp	x29, x30, [sp, #64]             ; 16-byte Folded Spill
	add	x29, sp, #64
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	mov	x8, #58809                      ; =0xe5b9
	movk	x8, #7396, lsl #16
	movk	x8, #18285, lsl #32
	movk	x8, #48984, lsl #48
	str	x8, [sp, #8]                    ; 8-byte Folded Spill
	stur	x0, [x29, #-8]
	mov	x9, #31765                      ; =0x7c15
	movk	x9, #32586, lsl #16
	movk	x9, #31161, lsl #32
	movk	x9, #40503, lsl #48
	stur	x9, [x29, #-16]
	stur	x8, [x29, #-24]
	bl	_apple_re_now_ns
	str	x0, [sp, #32]
	str	xzr, [sp, #16]
	b	LBB19_1
LBB19_1:                                ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp, #16]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB19_4
	b	LBB19_2
LBB19_2:                                ;   in Loop: Header=BB19_1 Depth=1
	ldr	x9, [sp, #8]                    ; 8-byte Folded Reload
	ldur	x8, [x29, #-16]
	; InlineAsm Start
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	umulh	x8, x8, x9
	; InlineAsm End
	stur	x8, [x29, #-16]
	b	LBB19_3
LBB19_3:                                ;   in Loop: Header=BB19_1 Depth=1
	ldr	x8, [sp, #16]
	add	x8, x8, #1
	str	x8, [sp, #16]
	b	LBB19_1
LBB19_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #24]
	ldur	x8, [x29, #-16]
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp, #24]
	ldr	x9, [sp, #32]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #64]             ; 16-byte Folded Reload
	add	sp, sp, #80
	ret
	.cfi_endproc
                                        ; -- End function
	.p2align	2                               ; -- Begin function bench_smull_dep
_bench_smull_dep:                       ; @bench_smull_dep
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	x0, [x29, #-8]
	mov	x8, #31161                      ; =0x79b9
	movk	x8, #40503, lsl #16
	stur	x8, [x29, #-16]
	mov	x8, #10030                      ; =0x272e
	movk	x8, #27746, lsl #16
	str	x8, [sp, #24]
	bl	_apple_re_now_ns
	str	x0, [sp, #16]
	str	xzr, [sp]
	b	LBB20_1
LBB20_1:                                ; =>This Inner Loop Header: Depth=1
	ldr	x8, [sp]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB20_4
	b	LBB20_2
LBB20_2:                                ;   in Loop: Header=BB20_1 Depth=1
	ldur	x8, [x29, #-16]
	mov	x9, #10030                      ; =0x272e
	movk	x9, #27746, lsl #16
	; InlineAsm Start
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	smull	x8, w8, w9
	; InlineAsm End
	stur	x8, [x29, #-16]
	b	LBB20_3
LBB20_3:                                ;   in Loop: Header=BB20_1 Depth=1
	ldr	x8, [sp]
	add	x8, x8, #1
	str	x8, [sp]
	b	LBB20_1
LBB20_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #8]
	ldur	x8, [x29, #-16]
	adrp	x9, _g_u64_sink@PAGE
	str	x8, [x9, _g_u64_sink@PAGEOFF]
	ldr	x8, [sp, #8]
	ldr	x9, [sp, #16]
	subs	x0, x8, x9
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
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
	b	LBB21_1
LBB21_1:                                ; =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #36]
	subs	w8, w8, #32
	b.hs	LBB21_4
	b	LBB21_2
LBB21_2:                                ;   in Loop: Header=BB21_1 Depth=1
	ldr	w8, [sp, #36]
	add	w8, w8, #1
	mov	w8, w8
                                        ; kill: def $x8 killed $w8
	ldr	w9, [sp, #36]
	mov	x10, x9
	add	x9, sp, #72
	str	x8, [x9, x10, lsl #3]
	b	LBB21_3
LBB21_3:                                ;   in Loop: Header=BB21_1 Depth=1
	ldr	w8, [sp, #36]
	add	w8, w8, #1
	str	w8, [sp, #36]
	b	LBB21_1
LBB21_4:
	bl	_apple_re_now_ns
	str	x0, [sp, #48]
	str	xzr, [sp, #24]
	b	LBB21_5
LBB21_5:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB21_7 Depth 2
	ldr	x8, [sp, #24]
	ldr	x9, [sp, #64]
	subs	x8, x8, x9
	b.hs	LBB21_12
	b	LBB21_6
LBB21_6:                                ;   in Loop: Header=BB21_5 Depth=1
	str	wzr, [sp, #20]
	b	LBB21_7
LBB21_7:                                ;   Parent Loop BB21_5 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #20]
	subs	w8, w8, #32
	b.hs	LBB21_10
	b	LBB21_8
LBB21_8:                                ;   in Loop: Header=BB21_7 Depth=2
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
	b	LBB21_9
LBB21_9:                                ;   in Loop: Header=BB21_7 Depth=2
	ldr	w8, [sp, #20]
	add	w8, w8, #1
	str	w8, [sp, #20]
	b	LBB21_7
LBB21_10:                               ;   in Loop: Header=BB21_5 Depth=1
	b	LBB21_11
LBB21_11:                               ;   in Loop: Header=BB21_5 Depth=1
	ldr	x8, [sp, #24]
	add	x8, x8, #1
	str	x8, [sp, #24]
	b	LBB21_5
LBB21_12:
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
	b.eq	LBB21_14
	b	LBB21_13
LBB21_13:
	bl	___stack_chk_fail
LBB21_14:
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
	b	LBB22_1
LBB22_1:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB22_3 Depth 2
	ldr	x8, [sp, #24]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB22_8
	b	LBB22_2
LBB22_2:                                ;   in Loop: Header=BB22_1 Depth=1
	str	wzr, [sp, #20]
	b	LBB22_3
LBB22_3:                                ;   Parent Loop BB22_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #20]
	subs	w8, w8, #32
	b.hs	LBB22_6
	b	LBB22_4
LBB22_4:                                ;   in Loop: Header=BB22_3 Depth=2
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
	b	LBB22_5
LBB22_5:                                ;   in Loop: Header=BB22_3 Depth=2
	ldr	w8, [sp, #20]
	add	w8, w8, #1
	str	w8, [sp, #20]
	b	LBB22_3
LBB22_6:                                ;   in Loop: Header=BB22_1 Depth=1
	b	LBB22_7
LBB22_7:                                ;   in Loop: Header=BB22_1 Depth=1
	ldr	x8, [sp, #24]
	add	x8, x8, #1
	str	x8, [sp, #24]
	b	LBB22_1
LBB22_8:
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
	b	LBB23_1
LBB23_1:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB23_3 Depth 2
	ldr	x8, [sp, #32]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB23_8
	b	LBB23_2
LBB23_2:                                ;   in Loop: Header=BB23_1 Depth=1
	str	wzr, [sp, #28]
	b	LBB23_3
LBB23_3:                                ;   Parent Loop BB23_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #28]
	subs	w8, w8, #32
	b.hs	LBB23_6
	b	LBB23_4
LBB23_4:                                ;   in Loop: Header=BB23_3 Depth=2
	mov	x8, #1                          ; =0x1
	str	x8, [sp, #16]
	ldr	x8, [sp, #16]
	adrp	x9, _g_atomic_counter@PAGE
	add	x9, x9, _g_atomic_counter@PAGEOFF
	ldadd	x8, x8, [x9]
	str	x8, [sp, #8]
	ldr	x8, [sp, #8]
	stur	x8, [x29, #-16]
	b	LBB23_5
LBB23_5:                                ;   in Loop: Header=BB23_3 Depth=2
	ldr	w8, [sp, #28]
	add	w8, w8, #1
	str	w8, [sp, #28]
	b	LBB23_3
LBB23_6:                                ;   in Loop: Header=BB23_1 Depth=1
	b	LBB23_7
LBB23_7:                                ;   in Loop: Header=BB23_1 Depth=1
	ldr	x8, [sp, #32]
	add	x8, x8, #1
	str	x8, [sp, #32]
	b	LBB23_1
LBB23_8:
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
	b	LBB24_1
LBB24_1:                                ; =>This Loop Header: Depth=1
                                        ;     Child Loop BB24_3 Depth 2
	ldr	x8, [sp, #24]
	ldur	x9, [x29, #-8]
	subs	x8, x8, x9
	b.hs	LBB24_8
	b	LBB24_2
LBB24_2:                                ;   in Loop: Header=BB24_1 Depth=1
	str	wzr, [sp, #20]
	b	LBB24_3
LBB24_3:                                ;   Parent Loop BB24_1 Depth=1
                                        ; =>  This Inner Loop Header: Depth=2
	ldr	w8, [sp, #20]
	subs	w8, w8, #4
	b.hs	LBB24_6
	b	LBB24_4
LBB24_4:                                ;   in Loop: Header=BB24_3 Depth=2
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
	b	LBB24_5
LBB24_5:                                ;   in Loop: Header=BB24_3 Depth=2
	ldr	w8, [sp, #20]
	add	w8, w8, #1
	str	w8, [sp, #20]
	b	LBB24_3
LBB24_6:                                ;   in Loop: Header=BB24_1 Depth=1
	b	LBB24_7
LBB24_7:                                ;   in Loop: Header=BB24_1 Depth=1
	ldr	x8, [sp, #24]
	add	x8, x8, #1
	str	x8, [sp, #24]
	b	LBB24_1
LBB24_8:
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
	cbnz	w8, LBB25_2
	b	LBB25_1
LBB25_1:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
	b	LBB25_2
LBB25_2:
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
	.p2align	2                               ; -- Begin function quantize_bfloat16_proxy
_quantize_bfloat16_proxy:               ; @quantize_bfloat16_proxy
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #16
	.cfi_def_cfa_offset 16
	str	s0, [sp, #12]
	ldr	s0, [sp, #12]
	str	s0, [sp, #8]
	ldr	w8, [sp, #8]
	add	w8, w8, #8, lsl #12             ; =32768
	str	w8, [sp, #8]
	ldr	w8, [sp, #8]
	and	w8, w8, #0xffff0000
	str	w8, [sp, #8]
	ldr	w8, [sp, #8]
	str	w8, [sp, #12]
	ldr	s0, [sp, #12]
	add	sp, sp, #16
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

l_.str.7:                               ; @.str.7
	.asciz	"probe,iters,unroll,elapsed_ns,ns_per_op,approx_gops\n"

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

.subsections_via_symbols

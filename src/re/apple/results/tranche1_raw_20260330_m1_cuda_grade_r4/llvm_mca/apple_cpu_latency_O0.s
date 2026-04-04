	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 26, 0	sdk_version 26, 0
	.globl	_main                           ; -- Begin function main
	.p2align	2
_main:                                  ; @main
	.cfi_startproc
; %bb.0:
	sub	sp, sp, #64
	stp	x29, x30, [sp, #48]             ; 16-byte Folded Spill
	add	x29, sp, #48
	.cfi_def_cfa w29, 16
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	stur	wzr, [x29, #-4]
	stur	w0, [x29, #-8]
	stur	x1, [x29, #-16]
	mov	x8, #16960                      ; =0x4240
	movk	x8, #15, lsl #16
	str	x8, [sp, #24]
	strb	wzr, [sp, #23]
	mov	w8, #1                          ; =0x1
	str	w8, [sp, #16]
	b	LBB0_1
LBB0_1:                                 ; =>This Inner Loop Header: Depth=1
	ldr	w8, [sp, #16]
	ldur	w9, [x29, #-8]
	subs	w8, w8, w9
	b.ge	LBB0_13
	b	LBB0_2
LBB0_2:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldur	x8, [x29, #-16]
	ldrsw	x9, [sp, #16]
	ldr	x0, [x8, x9, lsl #3]
	adrp	x1, l_.str@PAGE
	add	x1, x1, l_.str@PAGEOFF
	bl	_strcmp
	cbnz	w0, LBB0_5
	b	LBB0_3
LBB0_3:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1
	ldur	w9, [x29, #-8]
	subs	w8, w8, w9
	b.ge	LBB0_5
	b	LBB0_4
LBB0_4:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldur	x8, [x29, #-16]
	ldr	w9, [sp, #16]
	add	w9, w9, #1
	str	w9, [sp, #16]
	ldr	x0, [x8, w9, sxtw #3]
	mov	x1, #0                          ; =0x0
	mov	w2, #10                         ; =0xa
	bl	_strtoull
	str	x0, [sp, #24]
	b	LBB0_11
LBB0_5:                                 ;   in Loop: Header=BB0_1 Depth=1
	ldur	x8, [x29, #-16]
	ldrsw	x9, [sp, #16]
	ldr	x0, [x8, x9, lsl #3]
	adrp	x1, l_.str.1@PAGE
	add	x1, x1, l_.str.1@PAGEOFF
	bl	_strcmp
	cbnz	w0, LBB0_7
	b	LBB0_6
LBB0_6:                                 ;   in Loop: Header=BB0_1 Depth=1
	mov	w8, #1                          ; =0x1
	strb	w8, [sp, #23]
	b	LBB0_10
LBB0_7:
	ldur	x8, [x29, #-16]
	ldrsw	x9, [sp, #16]
	ldr	x0, [x8, x9, lsl #3]
	adrp	x1, l_.str.2@PAGE
	add	x1, x1, l_.str.2@PAGEOFF
	bl	_strcmp
	cbnz	w0, LBB0_9
	b	LBB0_8
LBB0_8:
	ldur	x8, [x29, #-16]
	ldr	x8, [x8]
	mov	x9, sp
	str	x8, [x9]
	adrp	x0, l_.str.3@PAGE
	add	x0, x0, l_.str.3@PAGEOFF
	bl	_printf
	stur	wzr, [x29, #-4]
	b	LBB0_16
LBB0_9:
	adrp	x8, ___stderrp@GOTPAGE
	ldr	x8, [x8, ___stderrp@GOTPAGEOFF]
	ldr	x0, [x8]
	ldur	x8, [x29, #-16]
	ldrsw	x9, [sp, #16]
	ldr	x8, [x8, x9, lsl #3]
	mov	x9, sp
	str	x8, [x9]
	adrp	x1, l_.str.4@PAGE
	add	x1, x1, l_.str.4@PAGEOFF
	bl	_fprintf
	mov	w8, #1                          ; =0x1
	stur	w8, [x29, #-4]
	b	LBB0_16
LBB0_10:                                ;   in Loop: Header=BB0_1 Depth=1
	b	LBB0_11
LBB0_11:                                ;   in Loop: Header=BB0_1 Depth=1
	b	LBB0_12
LBB0_12:                                ;   in Loop: Header=BB0_1 Depth=1
	ldr	w8, [sp, #16]
	add	w8, w8, #1
	str	w8, [sp, #16]
	b	LBB0_1
LBB0_13:
	ldrb	w8, [sp, #23]
	tbz	w8, #0, LBB0_15
	b	LBB0_14
LBB0_14:
	adrp	x0, l_.str.5@PAGE
	add	x0, x0, l_.str.5@PAGEOFF
	bl	_printf
	b	LBB0_15
LBB0_15:
	ldr	x0, [sp, #24]
	bl	_bench_add_dep
	mov	x1, x0
	ldr	x2, [sp, #24]
	ldrb	w8, [sp, #23]
	adrp	x0, l_.str.6@PAGE
	add	x0, x0, l_.str.6@PAGEOFF
	and	w3, w8, #0x1
	bl	_print_result
	ldr	x0, [sp, #24]
	bl	_bench_fadd_dep
	mov	x1, x0
	ldr	x2, [sp, #24]
	ldrb	w8, [sp, #23]
	adrp	x0, l_.str.7@PAGE
	add	x0, x0, l_.str.7@PAGEOFF
	and	w3, w8, #0x1
	bl	_print_result
	ldr	x0, [sp, #24]
	bl	_bench_fmadd_dep
	mov	x1, x0
	ldr	x2, [sp, #24]
	ldrb	w8, [sp, #23]
	adrp	x0, l_.str.8@PAGE
	add	x0, x0, l_.str.8@PAGEOFF
	and	w3, w8, #0x1
	bl	_print_result
	stur	wzr, [x29, #-4]
	b	LBB0_16
LBB0_16:
	ldur	w0, [x29, #-4]
	ldp	x29, x30, [sp, #48]             ; 16-byte Folded Reload
	add	sp, sp, #64
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
	sturb	w3, [x29, #-25]
	ldur	d0, [x29, #-24]
	ucvtf	d0, d0
	mov	x8, #4629700416936869888        ; =0x4040000000000000
	fmov	d1, x8
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
	ldurb	w8, [x29, #-25]
	tbz	w8, #0, LBB1_8
	b	LBB1_7
LBB1_7:
	ldur	x11, [x29, #-8]
	ldur	x10, [x29, #-24]
	ldur	x9, [x29, #-16]
	ldur	d1, [x29, #-48]
	ldr	d0, [sp, #64]
	mov	x8, sp
	str	x11, [x8]
	str	x10, [x8, #8]
	mov	x10, #32                        ; =0x20
	str	x10, [x8, #16]
	str	x9, [x8, #24]
	str	d1, [x8, #32]
	str	d0, [x8, #40]
	adrp	x0, l_.str.9@PAGE
	add	x0, x0, l_.str.9@PAGEOFF
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
	adrp	x0, l_.str.10@PAGE
	add	x0, x0, l_.str.10@PAGEOFF
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
	cbnz	w8, LBB5_2
	b	LBB5_1
LBB5_1:
	adrp	x0, _apple_re_now_ns.timebase@PAGE
	add	x0, x0, _apple_re_now_ns.timebase@PAGEOFF
	bl	_mach_timebase_info
	b	LBB5_2
LBB5_2:
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
	.asciz	"--csv"

l_.str.2:                               ; @.str.2
	.asciz	"--help"

l_.str.3:                               ; @.str.3
	.asciz	"usage: %s [--iters N] [--csv]\n"

l_.str.4:                               ; @.str.4
	.asciz	"unknown argument: %s\n"

l_.str.5:                               ; @.str.5
	.asciz	"probe,iters,unroll,elapsed_ns,ns_per_op,approx_gops\n"

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
.subsections_via_symbols

| SPDX-License-Identifier: GPL-2.0-or-later
| QRETRIG: the key-LED setter's entry (OS 1.54, 0x400e691c: f(led, colour), colour a palette
| index). For its LED numbers 14-16 ([TRIG 14]-[TRIG 16]; the panel's LEDs 13-15) the colour is qr_led_colour(led, colour)'s, then the
| stock setter runs from its third instruction, the two it replaces done here.
        .section .run, "ax"
        .globl  qr_led_wrap
qr_led_wrap:
        move.l  4(%sp), %d0             | the LED
        moveq   #14, %d1
        cmp.l   %d1, %d0
        blt.s   9f
        moveq   #16, %d1
        cmp.l   %d1, %d0
        bgt.s   9f
        move.l  8(%sp), -(%sp)          | its colour
        move.l  %d0, -(%sp)
        jsr     qr_led_colour
        addq.l  #8, %sp
        move.l  %d0, 8(%sp)
9:      link.w  %fp, #-32
        lea     0x4018e924, %a0
        jmp     0x400e6926

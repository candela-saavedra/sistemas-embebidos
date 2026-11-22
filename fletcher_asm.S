// Fletcher-16 con modulo diferido, escrito a mano para el Cortex-M0+
// (solo Thumb-1).
//
//   uint16_t fletcher16_asm(const uint8_t *data, uint32_t len);
//
// Solo reduce al final, asi que len tiene que ser <= 5802 (ver fletcher.c).
//
// Bucle desenrollado x8: cada byte son 4 ciclos (ldrb 2, adds 1, adds 1) y
// el control del bucle (4 ciclos) se paga cada 8 bytes -> 4.5 ciclos/byte.
//
//   r0 = puntero a los datos   r2 = sum1   r4 = auxiliar
//   r1 = contador del bucle    r3 = sum2   r5 = bytes sueltos (len & 7)

        .syntax unified
        .cpu cortex-m0plus
        .thumb

        .section .text.fletcher16_asm, "ax", %progbits
        .global fletcher16_asm
        .type   fletcher16_asm, %function

// sum1 += data[offset]; sum2 += sum1
        .macro  STEP offset
        ldrb    r4, [r0, #\offset]
        adds    r2, r2, r4
        adds    r3, r3, r2
        .endm

// reg = reg mod 255 sin dividir, igual que mod255() de fletcher.c
        .macro  MOD255 reg
        uxth    r4, \reg
        lsrs    \reg, \reg, #16
        adds    \reg, \reg, r4          // <= 0x1FFFE
        uxtb    r4, \reg
        lsrs    \reg, \reg, #8
        adds    \reg, \reg, r4          // <= 0x2FE
        uxtb    r4, \reg
        lsrs    \reg, \reg, #8
        adds    \reg, \reg, r4          // <= 0x100
        cmp     \reg, #255
        blo     1f
        subs    \reg, #255
1:
        .endm

        .balign 4
fletcher16_asm:
        push    {r4, r5}
        movs    r2, #0                  // sum1
        movs    r3, #0                  // sum2
        movs    r5, #7
        ands    r5, r1                  // r5 = len % 8
        lsrs    r1, r1, #3              // r1 = len / 8
        beq     .Ltail

.Lblock8:
        STEP    0
        STEP    1
        STEP    2
        STEP    3
        STEP    4
        STEP    5
        STEP    6
        STEP    7
        adds    r0, #8
        subs    r1, #1
        bne     .Lblock8

// Los len % 8 bytes que sobran, de uno en uno
.Ltail:
        cmp     r5, #0
        beq     .Lreduce
.Lbyte:
        STEP    0
        adds    r0, #1
        subs    r5, #1
        bne     .Lbyte

.Lreduce:
        MOD255  r2
        MOD255  r3
        lsls    r0, r3, #8
        orrs    r0, r2                  // (sum2 << 8) | sum1
        pop     {r4, r5}
        bx      lr

        .size   fletcher16_asm, . - fletcher16_asm

; Matthew Fabregas, Kane Ibanez, Bowen Li, John Teoxon - GROUP 3

.code
norm_xmm PROC
    vxorpd xmm0, xmm0, xmm0
    xor rax, rax

    mov r8, rdx
    and r8, -2
                                       
; we zero xmm0 using xor strategy - xor itself is 0,sum = 0, then set i to zero in rax.
; Make r8 equal to rdx and round down to the nearest multiple of 2, since we are processing 2 doubles at a time in the SIMD loop


simdLoop:
    cmp rax, r8
    jae simdDone

    vmovupd xmm1, XMMWORD PTR [rcx + rax * 8]
    vmulpd  xmm1, xmm1, xmm1
    vaddpd  xmm0, xmm0, xmm1

    add rax, 2
    jmp simdLoop

; main loop, check first if i is greater than or equal to r8, if so, we jump to simdDone. 
; Otherwise, we load 2 doubles from the array, then index times 8 since a double is 8 bytes and store them in xmm1. 
; We then square both doubles in xmm1 and add the result to the partial sums in xmm0. then, we increment i by 2 and repeat the loop.


simdDone:
    vunpckhpd xmm2, xmm0, xmm0
    vaddsd    xmm0, xmm0, xmm2

; we merge the two sums in xmm0.  We copy the upper sum into the lower position of xmm2, then add xmm2 to the lower position of xmm0.
; xmm0 will contain the sum of squares of all elements processed in the SIMD loop.


tailLoop:
    cmp rax, rdx
    jae finish

    vmovsd xmm1, QWORD PTR [rcx + rax * 8]
    vmulsd xmm1, xmm1, xmm1
    vaddsd xmm0, xmm0, xmm1

    inc rax
    jmp tailLoop

; Identical loop from the previous ones we have but in scalar form, we check if i is greater than or equal to n, if so, we jump to finish. 
; WE NEED THIS or else we will miss the last element if n is odd. so we only process the last element if need be.


finish:
    vsqrtsd xmm0, xmm0, xmm0
    ret
; square root of total in xmm0, final result of Euclid


norm_xmm ENDP

END

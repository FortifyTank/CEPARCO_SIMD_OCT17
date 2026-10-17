; Euclidean norm, SIMD AVX2 using XMM (128-bit = 2 doubles per step)
; double norm_xmm(const double* A, long long n)
; rcx = A, rdx = n, return value in xmm0

.code
norm_xmm PROC

    vxorpd xmm0, xmm0, xmm0             ; 2 partial sums = 0.0
    xor rax, rax                        ; i = 0

    mov r8, rdx
    and r8, -2                          ; r8 = n rounded DOWN to a multiple of 2
                                        ; (the SIMD loop only runs up to here)

simdLoop:
    cmp rax, r8
    jae simdDone

    vmovupd xmm1, XMMWORD PTR [rcx + rax * 8]   ; load 2 doubles
    vmulpd  xmm1, xmm1, xmm1                    ; square both
    vaddpd  xmm0, xmm0, xmm1                    ; add into the 2 partial sums

    add rax, 2
    jmp simdLoop

simdDone:
    ; horizontal add: partial sum lane0 + lane1 -> lane0
    vunpckhpd xmm2, xmm0, xmm0          ; copy high double to low position
    vaddsd    xmm0, xmm0, xmm2

tailLoop:                               ; boundary: leftover 0 or 1 element
    cmp rax, rdx
    jae finish

    vmovsd xmm1, QWORD PTR [rcx + rax * 8]
    vmulsd xmm1, xmm1, xmm1
    vaddsd xmm0, xmm0, xmm1

    inc rax
    jmp tailLoop

finish:
    vsqrtsd xmm0, xmm0, xmm0            ; Z = sqrt(sum)
    ret

norm_xmm ENDP

END

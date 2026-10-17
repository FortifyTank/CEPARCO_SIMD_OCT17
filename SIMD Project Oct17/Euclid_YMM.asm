; Euclidean norm, SIMD AVX2 using YMM (256-bit = 4 doubles per step)
; double norm_ymm(const double* A, long long n)
; rcx = A, rdx = n, return value in xmm0

.code
norm_ymm PROC

    vxorpd ymm0, ymm0, ymm0             ; 4 partial sums = 0.0
    xor rax, rax                        ; i = 0

    mov r8, rdx
    and r8, -4                          ; r8 = n rounded DOWN to a multiple of 4

simdLoop:
    cmp rax, r8
    jae simdDone

    vmovupd ymm1, YMMWORD PTR [rcx + rax * 8]   ; load 4 doubles
    vmulpd  ymm1, ymm1, ymm1                    ; square all 4
    vaddpd  ymm0, ymm0, ymm1                    ; add into the 4 partial sums

    add rax, 4                          ; advance 4 elements per iteration
    jmp simdLoop

simdDone:
    ; horizontal add: 4 lanes -> 1
    vextractf128 xmm1, ymm0, 1          ; xmm1 = upper 2 lanes
    vaddpd       xmm0, xmm0, xmm1       ; 4 -> 2 lanes
    vunpckhpd    xmm1, xmm0, xmm0       ; high lane to low position
    vaddsd       xmm0, xmm0, xmm1       ; 2 -> 1 lane

tailLoop:                               ; boundary: leftover 0 to 3 elements
    cmp rax, rdx
    jae finish

    vmovsd xmm1, QWORD PTR [rcx + rax * 8]
    vmulsd xmm1, xmm1, xmm1
    vaddsd xmm0, xmm0, xmm1

    inc rax
    jmp tailLoop

finish:
    vsqrtsd xmm0, xmm0, xmm0            ; Z = sqrt(sum)
    vzeroupper                          ; clean upper YMM bits before returning to C
    ret

norm_ymm ENDP

END

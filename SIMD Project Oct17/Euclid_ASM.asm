; Euclidean norm, scalar x86-64 (no SIMD)
; double norm_asm(const double* A, long long n)
; rcx = A, rdx = n, return value in xmm0

.code
norm_asm PROC

    xorpd xmm0, xmm0                    ; sum = 0.0
    xor rax, rax                        ; i = 0

loopStart:
    cmp rax, rdx
    jae finish                          ; i >= n, done

    movsd xmm1, QWORD PTR [rcx + rax * 8]   ; doubles are 8 bytes
    mulsd xmm1, xmm1                    ; A[i] * A[i]
    addsd xmm0, xmm1                    ; sum += A[i]^2

    inc rax
    jmp loopStart

finish:
    sqrtsd xmm0, xmm0                   ; Z = sqrt(sum), returned in xmm0
    ret

norm_asm ENDP

END

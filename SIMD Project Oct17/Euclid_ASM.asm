; x86-64 scalar
; rcx = A, rdx = n, result in xmm0

.code
norm_asm PROC
    xorpd xmm0, xmm0
    xor rax, rax

L1:
    cmp rax, rdx
    jge done
    movsd xmm1, qword ptr [rcx + rax*8]
    mulsd xmm1, xmm1
    addsd xmm0, xmm1
    inc rax
    jmp L1

done:
    sqrtsd xmm0, xmm0
    ret
norm_asm ENDP
END

; Matthew Fabregas, Kane Ibanez, Bowen Li, John Teoxon - GROUP 3

.code
norm_asm PROC
    xorpd xmm0, xmm0
    xor rax, rax

; zero xmm0 and rax to 0 for index


L1:
    cmp rax, rdx
    jge done
    movsd xmm1, qword ptr [rcx + rax*8]
    mulsd xmm1, xmm1
    addsd xmm0, xmm1
    inc rax
    jmp L1

; check first if loop is finished, if not then load double into xmm1 using the address, we then square it then add it to xmm0, increment rax and repeat the loop.


done:
    sqrtsd xmm0, xmm0
    ret

; get square root of xmm0, final result of Euclid


norm_asm ENDP
END

; Matthew Fabreas, Kane Ibanez, Bowen Li, John Teoxon - GROUP 3

.code
norm_ymm PROC
    vxorpd ymm0, ymm0, ymm0
    xor rax, rax

    mov r8, rdx
    and r8, -4

; similar to XMM file, ymm0 xor zeroed then sum = 0 for the 4 lanes cause 4 doubles instead of 2 for YMM, rax is our index then r9 rounded down by 4 so we loop in groups of 4.


simdLoop:
    cmp rax, r8
    jae simdDone

    vmovupd ymm1, YMMWORD PTR [rcx + rax * 8]
    vmulpd  ymm1, ymm1, ymm1
    vaddpd  ymm0, ymm0, ymm1

    add rax, 4
    jmp simdLoop

; check if i greater/equal than r8, leave if it is, if not, load 4 double at rcx + rax * 8 addresses, square all 4 using vmulpd then add all together.
; increment rax by 4 to process next.


simdDone:
    ; horizontal add: 4 lanes -> 1
    vextractf128 xmm1, ymm0, 1
    vaddpd       xmm0, xmm0, xmm1
    vunpckhpd    xmm1, xmm0, xmm0
    vaddsd       xmm0, xmm0, xmm1

; ymm0 will hold the 4 sums of squares, same as xmm file, we extract the upper 2 lanes of ymm0 into xmm1, then add xmm1 to the lower 2 lanes of ymm0.
; vunpckhpd copy upper of xmm1 to lower of xmm1, then add xmm1 to lower of xmm0, now we have the sum of squares in the lower lane of xmm0.


tailLoop:
    cmp rax, rdx
    jae finish

    vmovsd xmm1, QWORD PTR [rcx + rax * 8]
    vmulsd xmm1, xmm1, xmm1
    vaddsd xmm0, xmm0, xmm1

    inc rax
    jmp tailLoop

; same as XMM file, check if i is greater/equal than n, if so, leave, if not, load 1 double at rcx + rax * 8 addresses, square it then add to sum in xmm0.
; Need this if for example we still ahve 1-3 indexes that we need to process, we only process the last element if need be.


finish:
    vsqrtsd xmm0, xmm0, xmm0
    vzeroupper
    ret

;Squareroot the total in xmm0, final result of Euclid


norm_ymm ENDP

END

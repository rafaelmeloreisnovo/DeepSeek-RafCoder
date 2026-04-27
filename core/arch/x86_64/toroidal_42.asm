; Toroidal 7-state attractor demo in pure x86_64 NASM (Linux)
; - no libc
; - no malloc
; - direct syscalls only
; - SSE2 scalar FP ops

BITS 64
default rel

global _start

section .rodata
    weight_old       dq 0.75
    weight_new       dq 0.25
    inv_two32        dq 2.3283064365386963e-10   ; 1 / 2^32
    inv_seven        dq 0.14285714285714285      ; 1 / 7

    ; irrational-ish seeds/biases for non-trivial toroidal drift
    seeds            dq 0.6180339887498948, 0.8660254037844386, 0.3333333333333333
                     dq 0.7071067811865475, 0.2718281828459045, 0.1414213562373095
                     dq 0.5772156649015329

    biases           dq 0.00390625, 0.0078125, 0.01171875, 0.015625
                     dq 0.01953125, 0.0234375, 0.02734375

    fnv_offset       dq 0xcbf29ce484222325
    fnv_prime        dq 0x100000001b3

    hex_chars        db '0123456789abcdef'

section .data
    state            times 7 dq 0.0
    phi_value        dq 0.0
    step_count       dq 0
    outbuf           times 16 db '0'
                     db 10

section .text
_start:
    call state_init

    xor r15d, r15d
.loop:
    cmp r15d, 42
    jge .done

    mov [step_count], r15
    call generate_input          ; xmm0 <- input in [0,1)
    call state_update            ; updates state[0..6], phi_value

    inc r15d
    jmp .loop

.done:
    call print_fingerprint

    mov eax, 60                  ; sys_exit
    xor edi, edi                 ; code 0
    syscall

; ------------------------------------------------------------
; state_init: fill 7-state vector with non-zero irrational seeds
; ------------------------------------------------------------
state_init:
    mov rsi, seeds
    mov rdi, state
    mov ecx, 7
.init_loop:
    movsd xmm0, [rsi]
    movsd [rdi], xmm0
    add rsi, 8
    add rdi, 8
    loop .init_loop
    ret

; ------------------------------------------------------------
; generate_input: FNV-1a over 8 bytes of step_count, map to [0,1)
; return xmm0 = normalized input
; ------------------------------------------------------------
generate_input:
    mov rbx, [fnv_offset]
    mov r8, [step_count]
    mov ecx, 8
.gen_loop:
    movzx edx, r8b
    xor rbx, rdx

    mov rdx, [fnv_prime]
    mov rax, rbx
    mul rdx
    mov rbx, rax

    shr r8, 8

    dec ecx
    jnz .gen_loop

    mov eax, ebx                 ; lower 32 bits
    cvtsi2sd xmm0, rax
    mulsd xmm0, [inv_two32]
    ret

; ------------------------------------------------------------
; state_update:
; s_i <- frac(0.75*s_i + 0.25*frac(input + bias_i))
; phi <- (sum_i s_i^2 / 7)^2
; ------------------------------------------------------------
state_update:
    mov rsi, state
    mov rdi, biases
    xor edx, edx

.update_loop:
    movsd xmm1, [rsi + rdx*8]    ; old s_i
    mulsd xmm1, [weight_old]

    movapd xmm2, xmm0            ; input
    addsd xmm2, [rdi + rdx*8]    ; + bias
    call frac_xmm2               ; xmm2 = frac(xmm2)
    mulsd xmm2, [weight_new]

    addsd xmm1, xmm2
    movapd xmm2, xmm1
    call frac_xmm2               ; xmm2 = frac(xmm1)
    movsd [rsi + rdx*8], xmm2

    inc edx
    cmp edx, 7
    jl .update_loop

    ; phi calculation
    xorpd xmm4, xmm4             ; sumsq = 0
    xor edx, edx
.phi_loop:
    movsd xmm1, [rsi + rdx*8]
    mulsd xmm1, xmm1
    addsd xmm4, xmm1
    inc edx
    cmp edx, 7
    jl .phi_loop

    mulsd xmm4, [inv_seven]      ; C = mean(s^2)
    movapd xmm5, xmm4
    mulsd xmm5, xmm4             ; phi = C^2
    movsd [phi_value], xmm5
    ret

; helper: xmm2 <- frac(xmm2) = x - floor(x)
frac_xmm2:
    roundsd xmm3, xmm2, 1        ; floor
    subsd xmm2, xmm3
    ret

; ------------------------------------------------------------
; print_fingerprint:
; FNV-1a over 56 bytes of state and print 16 hex chars + '\n'
; ------------------------------------------------------------
print_fingerprint:
    mov rbx, [fnv_offset]
    mov rsi, state
    mov ecx, 56
.hash_loop:
    movzx eax, byte [rsi]
    xor rbx, rax

    mov rdx, [fnv_prime]
    mov rax, rbx
    mul rdx
    mov rbx, rax

    inc rsi
    dec ecx
    jnz .hash_loop

    ; hex encode hash in outbuf[0..15]
    mov rdi, outbuf
    add rdi, 15
    mov ecx, 16
.hex_loop:
    mov rax, rbx
    and eax, 0x0f
    mov al, [hex_chars + rax]
    mov [rdi], al
    shr rbx, 4
    dec rdi
    dec ecx
    jnz .hex_loop

    mov eax, 1                   ; sys_write
    mov edi, 1                   ; stdout
    mov rsi, outbuf
    mov edx, 17
    syscall
    ret

# toroidal_42.asm (NASM, x86_64 Linux)

Implementação mínima em *assembler puro* de um sistema toroidal de 7 estados:

- estado: `s = (u,v,ψ,χ,ρ,δ,σ) ∈ [0,1)^7`
- atualização: `s_i <- frac(0.75*s_i + 0.25*frac(input + bias_i))`
- entrada interna: hash FNV-1a do `step_count`
- iterações: 42
- saída: fingerprint FNV-1a (16 hex + `\n`)

Sem libc, sem runtime, sem GC, apenas `sys_write` e `sys_exit`.

## Compilar e executar

```bash
nasm -felf64 core/arch/x86_64/toroidal_42.asm -o /tmp/toroidal_42.o
ld /tmp/toroidal_42.o -o /tmp/toroidal_42
/tmp/toroidal_42
```

A saída é um hash hexadecimal de 64 bits representando o estado final após 42 passos.

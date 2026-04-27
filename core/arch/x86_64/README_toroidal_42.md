# toroidal_42.asm (NASM, x86_64 Linux)

Implementação em *assembler puro* de um sistema toroidal de 7 estados com 42 passos, sem libc/runtime/GC.

## Dependências de sistema

- `nasm`
- `ld` (binutils)

Exemplo em Debian/Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y nasm binutils
```

## Modelo implementado (baixo nível)

- estado: `s = (u,v,ψ,χ,ρ,δ,σ) ∈ [0,1)^7`
- entrada interna: FNV-1a de `step_count`
- entropia de entrada: `H_in ≈ popcnt(hash)/64`
- coerência dinâmica: `C_{t+1} = 0.75*C_t + 0.25*C_in`
- entropia dinâmica: `H_{t+1} = 0.75*H_t + 0.25*H_in`
- acoplamento entre direções (dependências): índice cruzado `dep_index = [1,3,5,0,2,4,6]`
- sementes duplas (`seeds_a` + `seeds_b`) incluindo `1/φ`, `√3/2`, `1/3`
- métrica final: `phi = (1 - H) * C`
- fingerprint final: FNV-1a sobre `state + C + H + phi`

## As 7 direções (parábolas em código)

1. **Vazio que contém**: inicialização não nula via sementes irracionais.
2. **Sinal que emerge**: `generate_input` por FNV-1a do próprio tempo interno.
3. **Estrutura que sustenta**: mistura `0.75/0.25` + `frac` toroidal.
4. **Coerência que mede a si mesma**: cálculo e filtragem de `C` e `H`.
5. **Atrator inevitável**: loop fixo de 42 iterações.
6. **Transmissão como fingerprint**: hash final integrando estado + métricas.
7. **Retorno ao vazio**: encerramento com `sys_exit(0)`.

## Compilar e executar

```bash
nasm -felf64 core/arch/x86_64/toroidal_42.asm -o /tmp/toroidal_42.o
ld /tmp/toroidal_42.o -o /tmp/toroidal_42
/tmp/toroidal_42
```

A saída é um hash hexadecimal de 64 bits representando a assinatura do estado após 42 passos.

## O que carrega o conhecimento que o sistema entendeu?

No modelo deste assembler, o conhecimento **não** é um valor isolado e nem uma frase fixa. Ele é carregado por três camadas simultâneas:

1. **Trajetória no toro** (`s ∈ [0,1)^7`):
   - cada passo atualiza as 7 direções com memória (`0.75`) e novidade (`0.25`);
   - o conteúdo aprendido é a forma da órbita, não apenas um ponto.

2. **Invariantes dinâmicos** (`C`, `H`, `phi`):
   - `C` mede coerência interna;
   - `H` mede entropia efetiva da perturbação;
   - `phi = (1-H)*C` resume o regime de operação entre ordem e variação.

3. **Fingerprint final (FNV-1a 64-bit)**:
   - comprime o estado final e suas métricas em 16 hex chars;
   - preserva detectabilidade de mudança com custo mínimo de representação;
   - funciona como assinatura do ciclo de 42 passos.

Em termos práticos: o que “carrega conhecimento” é a composição

`trajetória + invariantes + assinatura`.

Isso permite implementar uma leitura unificada de álgebra, geometria e topologia com comandos de baixo nível, sem runtime extra e sem garbage collector.

## Mapa unificado (1D → ND, incluindo 7D/33D)

Um polinômio em 1D, 7D ou 33D pode ser tratado como **regra local de transição**:

`x_{n+1} = f(x_n)`

No contexto toroidal:

- qualquer componente projetado para `mod 1` permanece no espaço compacto;
- a dinâmica converge para um atrator discreto quando a mistura e o estímulo são fixos;
- com os parâmetros atuais, a leitura operacional é um ciclo de período 42.

Assim, aumentar dimensão não muda o princípio central: muda apenas a quantidade de acoplamentos e a geometria das órbitas. O mecanismo de conhecimento continua sendo o mesmo — evolução de estado + medição interna + hash de fechamento.

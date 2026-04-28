# RafCoder — Auditoria Técnica Extensa

Este documento registra uma análise estrutural do repositório `wojcikiewicz17/RafCoder`, com foco em arquitetura, baixo nível, Android NDK/JNI, portabilidade ARM32/ARM64, CI, documentação, riscos técnicos e próximos passos concretos.

## 1. Diagnóstico central

O `RafCoder` não deve ser lido apenas como um fork do DeepSeek Coder. Ele se tornou um repositório híbrido com três camadas principais:

1. **Camada herdada DeepSeek Coder**
   - README original.
   - Dependências Python.
   - Scripts de demo, avaliação e fine-tuning.
   - Integração com `transformers`, `torch`, `vLLM` e modelos Hugging Face.

2. **Camada RAFAELOS low-level**
   - `rafaelos.asm`.
   - `core/sector.c`.
   - `core/sector.h`.
   - `core/arch/primitives.*`.
   - Documentos matemáticos, operacionais e de roadmap.

3. **Camada Android/JNI/NDK**
   - Projeto Android em `android/`.
   - Build Gradle/Kotlin.
   - CMake/NDK.
   - ABI `armeabi-v7a` e `arm64-v8a`.
   - Workflow GitHub Actions para geração de APKs.

A força real do repositório está menos no material herdado do DeepSeek e mais nos acréscimos próprios: núcleo C/ASM, formalização RAFAELOS, bridge Android e pipeline de build.

## 2. Identidade atual do repositório

O README ainda começa com a apresentação original do DeepSeek Coder. Isso cria um desalinhamento entre a fachada do repositório e o que ele realmente está se tornando.

### Problema

O visitante ou ferramenta automatizada pode interpretar o projeto como apenas uma variação de DeepSeek Coder, quando na prática já existe uma camada própria de runtime low-level.

### Correção recomendada

Criar uma nova introdução no README com esta leitura:

```text
RafCoder = RAFAELOS low-level runtime + Android native bridge + DeepSeek legacy compatibility.
```

E mover ou isolar o conteúdo upstream do DeepSeek para documentação histórica, por exemplo:

```text
docs/upstream/deepseek_coder_readme.md
```

## 3. Camada DeepSeek herdada

A camada herdada mantém utilidade para:

- compatibilidade com scripts de inferência;
- avaliação de modelos;
- fine-tuning;
- uso com Hugging Face;
- benchmark de código;
- documentação de origem.

Porém ela é pesada e orientada a GPU/LLM, enquanto o RAFAELOS aponta para outra direção:

- C;
- assembly;
- Android nativo;
- ARM32/ARM64;
- execução determinística;
- ausência de runtime pesado;
- controle de ciclos, cache, hashing, CRC e invariantes.

### Ponderação

A camada DeepSeek deve continuar existindo, mas como camada de compatibilidade, não como identidade principal do RafCoder.

## 4. Camada `rafaelos.asm`

O arquivo `rafaelos.asm` representa um núcleo x86_64 NASM com foco explícito em execução mínima.

Características:

- sem libc;
- sem garbage collector;
- sem heap;
- syscalls Linux diretas;
- estado em `.bss`;
- constantes em `.rodata`;
- execução de 42 passos;
- cálculo de `phi`;
- agrupamento LOW/MID/HIGH;
- atualização por média exponencial com `alpha = 0.25`.

### Valor técnico

Esse arquivo é importante porque cristaliza a intenção de criar um núcleo executável mínimo, observável e independente de runtime externo.

### Limitação atual

Ele é x86_64 NASM. Portanto, para Android/ARM32, ele serve mais como prova conceitual e referência de comportamento do que como motor final.

### Riscos conhecidos

- Dependência de SSE4.1 por uso de `roundsd`.
- Cálculo de entropia simplificado.
- Conversão decimal simplificada.
- Ausência de fallback SSE2.
- Ainda não existe versão ARM32 equivalente.

## 5. Camada `core/sector.c`

O arquivo `core/sector.c` é o ponto mais importante para portabilidade real.

Ele implementa:

- payload fixo de 32 bytes;
- entropia aproximada por bytes únicos e transições;
- atualização de coerência e entropia em Q16;
- FNV step;
- CRC32 local;
- xorshift64;
- invariante geométrico;
- saída compacta em 8 palavras.

O header `core/sector.h` define o estado principal:

```c
typedef struct state {
    uint32_t coherence_q16;
    uint32_t entropy_q16;
    uint64_t hash64;
    uint32_t crc32;
    uint32_t last_entropy_milli;
    uint32_t last_invariant_milli;
    uint32_t output_words;
    uint32_t reserved;
    uint32_t output[CORE_OUTPUT_WORDS];
} state;
```

E expõe:

```c
void run_sector(struct state* s, uint32_t iterations);
```

### Ponto forte

Esse núcleo já possui a forma correta para virar motor portátil:

- estado explícito;
- saída determinística;
- sem heap visível;
- sem dependência pesada;
- cálculo inteiro/fixo;
- API pequena;
- possibilidade de integração com JNI.

### Gap crítico

`payload` e `scratch` são globais estáticos em `core/sector.c`. Isso impede segurança plena em execução paralela.

Atualmente, chamadas simultâneas a `run_sector()` podem compartilhar memória mutável.

### Correção recomendada

Criar workspace por chamada ou por thread:

```c
typedef struct sector_workspace {
    uint8_t payload[CORE_PAYLOAD_SIZE];
    uint32_t unique_marks[8];
    uint32_t spread_milli;
} sector_workspace;

void run_sector_ex(struct state* s, sector_workspace* ws, uint32_t iterations);
```

Depois manter `run_sector()` como wrapper simples, se necessário.

## 6. Camada `core/arch`

O repositório já possui separação correta entre primitivas genéricas e primitivas por arquitetura.

Arquivos relevantes:

```text
core/arch/primitives.h
core/arch/primitives.c
core/arch/x86_64/primitives.S
core/arch/aarch64/primitives.S
```

### Ponto forte

Essa estrutura permite evoluir para:

- x86_64;
- ARM64;
- ARM32;
- fallback C;
- NEON;
- rotas SIMD;
- seleção por ABI.

### Gap principal

Ainda não há:

```text
core/arch/armv7/primitives.S
core/arch/armv7/neon.S
```

Mesmo que o Android esteja configurado para `armeabi-v7a`, ainda falta uma rota assembly/NEON dedicada para ARM32.

### Consequência

No ARM32, a tendência é cair no fallback C. Isso é correto para compatibilidade, mas não entrega o ganho de performance esperado.

## 7. Camada Android/JNI/NDK

O projeto Android já possui estrutura funcional:

- Gradle Kotlin DSL;
- Android Gradle Plugin;
- Kotlin;
- CMake;
- NDK;
- ABI `armeabi-v7a` e `arm64-v8a`;
- release unsigned;
- release signed condicional por secrets.

O arquivo JNI atual retorna apenas uma mensagem:

```cpp
std::string msg = "RafCoder Native Core OK (armeabi-v7a + arm64-v8a)";
```

### Diagnóstico

O Android prova que a biblioteca nativa carrega, mas ainda não executa o core RAFAELOS real.

O caminho necessário é:

```text
MainActivity.kt
   ↓ JNI
native-lib.cpp
   ↓ C bridge
core/sector.c
   ↓ arch/primitives
ARM32 / ARM64
```

### Próxima correção obrigatória

O `CMakeLists.txt` deve incluir o core:

```cmake
add_library(
    rafcoder_native
    SHARED
    native-lib.cpp
    ../../../../core/sector.c
    ../../../../core/arch/primitives.c
)
```

Depois, condicionar arquivos assembly por ABI.

## 8. CI/CD

O repositório já tem workflow Android:

```text
.github/workflows/android-native-ci.yml
```

Ele executa:

- checkout;
- Java 17;
- Android SDK;
- Gradle;
- build debug;
- build release;
- assinatura opcional;
- upload de APKs.

### Ponto forte

Isso já é uma base DevOps real.

### Gap

O CI ainda só prova que o APK compila. Ele não prova:

- que o core funciona;
- que a saída é determinística;
- que ARM32 usa rota correta;
- que Android chama `run_sector()`;
- que não há regressão nos outputs;
- que Python e C convergem.

### Correções recomendadas

Adicionar:

```text
.github/workflows/core-ci.yml
core/tests/test_sector_snapshot.c
tools/compare_sector_outputs.py
```

O teste mínimo deve rodar:

```c
state s = {0};
run_sector(&s, 42);
```

E comparar:

- `hash64`;
- `crc32`;
- `coherence_q16`;
- `entropy_q16`;
- `last_entropy_milli`;
- `last_invariant_milli`;
- `output[0..7]`.

## 9. Ferramenta Python `cron_fidelity_grouping.py`

Esse script é uma referência de alto nível para validação conceitual.

Ele implementa:

- mapa toroidal `T^7`;
- coerência/entropia com `alpha = 0.25`;
- FNV;
- CRC32;
- invariante geométrico;
- 40 setores;
- 5 grupos;
- saída JSON.

### Uso correto

Ele deve servir como baseline acadêmico/experimental, não como motor de performance.

Fluxo ideal:

1. Python gera baseline JSON.
2. C gera resultado equivalente.
3. Assembly otimizado preserva equivalência.
4. Android executa o core e retorna o mesmo vetor.
5. CI compara tudo.

## 10. Classificação de valor

### Diamante

1. `core/sector.c` como núcleo portátil.
2. `core/arch` como separação de primitivas por arquitetura.
3. CI Android com geração de APK.

### Ouro

1. `rafaelos.asm` como núcleo conceitual x86_64 sem libc.
2. `docs/rafaelos_unified_map.md` como formalização matemática.
3. `docs/rafaelos_bug_and_pending_map.md` como disciplina de roadmap.

### Ródio

1. Android NDK preparado, mas ainda não integrado ao core.
2. Python benchmark como referência de validação.
3. Licenciamento MIT herdado, desde que preservados os avisos originais.

## 11. Riscos técnicos reais

### Risco 1 — identidade confusa

O repo parece DeepSeek na fachada, mas RAFAELOS no núcleo.

**Ação:** reescrever README com identidade RafCoder/RAFAELOS.

### Risco 2 — Android não executa o core

O JNI atual só retorna string.

**Ação:** expor `run_sector()` via JNI.

### Risco 3 — core não reentrante

Globais estáticos impedem paralelismo seguro.

**Ação:** criar workspace por chamada/thread.

### Risco 4 — ARM32 sem assembly próprio

`armeabi-v7a` existe no Gradle, mas falta `armv7/primitives.S`.

**Ação:** implementar rota ARM32 básica e depois NEON.

### Risco 5 — CI sem validação comportamental

Compilar não é provar execução correta.

**Ação:** adicionar snapshot determinístico.

## 12. Ordem recomendada de implementação

### Prioridade 1 — conectar Android ao core

Fazer o app chamar `run_sector()` e exibir os outputs.

### Prioridade 2 — tornar `run_sector()` reentrante

Remover memória global mutável.

### Prioridade 3 — criar ARM32 real

Adicionar `core/arch/armv7/primitives.S`.

### Prioridade 4 — criar testes determinísticos

Comparar outputs conhecidos em CI.

### Prioridade 5 — reorganizar documentação

Separar upstream DeepSeek de RAFAELOS.

## 13. Prompt técnico para Codex

```text
Audite e evolua o repositório wojcikiewicz17/RafCoder com foco em tornar o núcleo RAFAELOS realmente executável no Android via JNI/NDK, preservando compatibilidade com armeabi-v7a e arm64-v8a.

Objetivos obrigatórios:

1. Integrar o core C ao Android:
   - Incluir core/sector.c e core/arch/primitives.c no CMake do Android.
   - Expor uma função JNI que chame run_sector().
   - Retornar os 8 output_words para Kotlin.
   - Atualizar MainActivity.kt para exibir hash64, crc32, coherence, entropy e invariant.

2. Tornar o core reentrante:
   - Remover payload e scratch globais estáticos de core/sector.c.
   - Criar sector_workspace ou mover buffers para struct state.
   - Garantir que múltiplas chamadas paralelas não compartilhem memória mutável indevidamente.

3. Preparar ARM32:
   - Criar core/arch/armv7/primitives.S com primitivas básicas.
   - Ajustar Makefile/CMake para selecionar armv7 quando ABI for armeabi-v7a.
   - Manter fallback C funcional para arquiteturas não cobertas.

4. Criar testes determinísticos:
   - Adicionar core/tests/test_sector_snapshot.c.
   - Rodar run_sector com seed/estado inicial fixo e comparar output_words.
   - Adicionar workflow core-ci.yml para compilar e executar teste em Linux.

5. Melhorar documentação:
   - Criar docs/architecture.md explicando as camadas:
     DeepSeek upstream, RAFAELOS core, C/ASM primitives, Android JNI, CI.
   - Criar tabela equação -> função C/ASM.
   - Atualizar README para deixar claro que DeepSeek é camada herdada e RAFAELOS é camada própria.

Restrições:
- Não adicionar dependências pesadas.
- Não versionar binários.
- Preservar licenças existentes.
- Manter build Android unsigned funcionando mesmo sem secrets.
- Preferir patches pequenos, testáveis e revisáveis.
```

## F DE RESOLVIDO

O repositório já possui:

- núcleo `rafaelos.asm` sem libc/GC/heap;
- núcleo C portátil em `core/sector.c`;
- API compacta em `sector.h`;
- primitivas separadas por arquitetura;
- fallback C;
- Android Gradle/NDK;
- ABI `armeabi-v7a` e `arm64-v8a`;
- CI que gera APK;
- ferramenta Python para benchmark;
- documentação de bugs e roadmap.

## F DE GAP

Ainda falta:

- Android chamar o core real;
- `run_sector()` ficar reentrante/thread-safe;
- ARM32 ter assembly próprio;
- NEON ser implementado;
- CI validar comportamento, não só build;
- README separar upstream DeepSeek de RAFAELOS;
- teste determinístico;
- matriz equação -> código -> teste;
- benchmark C/ASM vs Python;
- threat model operacional.

## F DE NEXT

### Opção 1 — impacto imediato

Acoplar `core/sector.c` ao Android via JNI e exibir os 8 outputs no app.

### Opção 2 — base para paralelismo

Refatorar `run_sector()` para ser reentrante e seguro para execução com múltiplas threads.

### Opção 3 — ARM32/NEON

Criar `core/arch/armv7/primitives.S` e preparar a primeira rota NEON para `xor_block`, CRC/hash e processamento de bloco.

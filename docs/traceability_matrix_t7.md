# T7 Traceability Matrix — Equação → Código → Teste → Métrica

## Escala de status
- ✅ Coberto
- 🟡 Parcial
- ⛔ Não coberto

| Grupo | Equações | Implementação principal | Teste atual | Métrica operacional | Status | Próxima ação |
|---|---|---|---|---|---|---|
| A Topologia/Estado | 1,2,3,9,10,23,45 | `core/arch/x86_64/toroidal_42.asm` | execução manual 42 passos | repetibilidade por seed | 🟡 | adicionar teste automatizado de órbita |
| B Coerência/Entropia | 5,6,7,8,14,26,27,43 | `core/sector.c` (`coherence_update`, `entropy_milli`) | `core/test_sector_snapshot.c` | estabilidade Q16 e faixa de saída | 🟡 | criar property tests com ruído controlado |
| C Integridade | 15,16,17,30,31,32,33 | `core/sector.c`, `core/verb_seed.c` | `core/test_verb_seed.c` | determinismo hash/CRC por estado | ✅ | incluir teste de corrupção de payload |
| D Espectral/Semântico | 11,12,13,41,44,50 | docs e especificações | sem teste automatizado | preservação estrutural cross-língua | ⛔ | definir benchmark multilíngue mínimo |
| E Geometria/Capacidade | 24,25,46,49 | `core/sector.c` (`geometric_invariant`) | snapshot indireto | coerência `bits_geom` vs saída | 🟡 | expor métrica explícita no output |
| F Dinâmica √3/2 | 18,22,39 | `docs/RAFAELIA_3ITEMS_UNIFIED.txt` | sem teste de código | convergência para ponto fixo | ⛔ | protótipo C/Q16 de convergência |
| G Acoplamento angular | 20,37,48 | especificação apenas | nenhum | resposta a variação de fase | ⛔ | inserir módulo experimental com telemetria |
| H Cripto fluxo | 34,35 | especificação apenas | nenhum | variabilidade de keystream | ⛔ | definir API experimental separada |
| I Distâncias não-euclidianas | 42 | especificação apenas | nenhum | separação d_theta vs d_gamma | ⛔ | criar função de distância paramétrica |
| J Hamiltoniano quântico (explorat.) | 47 | especificação apenas | nenhum | energia efetiva de acoplamentos | ⛔ | manter em trilha de pesquisa, fora do core |

## Backlog priorizado (curto prazo)
1. Teste automático de órbita 42 passos no alvo x86_64.
2. Property tests de `coherence_update` com variação de `alpha` e ruído.
3. Teste de corrupção de payload para validar cadeia FNV/CRC.
4. Exportar `bits_geom` e `last_invariant_milli` para auditoria externa.
5. Benchmark multilíngue inicial (PT/EN/JA/ZH/HE/EL/ARC).

## Convenção para PRs futuros
- Toda mudança deve atualizar esta matriz.
- Cada PR precisa declarar: grupo, equações, teste e métrica.
- Sem essa atualização, PR fica incompleto para merge técnico.

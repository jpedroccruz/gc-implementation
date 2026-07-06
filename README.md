# Garbage Collector Implementation

Projeto em C11 para a disciplina de Sistemas Operacionais + Estruturas de Dados, com foco em entender e implementar as bases de um garbage collector a partir de um heap controlado manualmente.

O objetivo central foi resolver um problema clássico: como descobrir, de forma segura e eficiente, quais objetos em memória ainda estão vivos quando o programa não faz a liberação manual. A proposta do projeto é criar uma biblioteca de alocação com `gc_malloc()` e um coletor que consiga identificar raízes na stack, validar ponteiros e remover apenas o que realmente virou lixo.

## O problema que eu queria resolver

O desafio não era só "liberar memória". O problema real era construir uma estratégia para:

1. alocar objetos em um heap próprio;
2. descobrir quais endereços na stack e nos registradores podem representar ponteiros válidos;
3. validar rapidamente se um endereço pertence a algum objeto alocado;
4. marcar os objetos alcançáveis;
5. reaproveitar o que ficou inacessível sem quebrar o programa.

Em outras palavras, eu precisava sair da ideia simples de `malloc`/`free` e chegar em um modelo em que a própria biblioteca toma a decisão sobre o que ainda pode continuar vivo.

## Onde estava a dificuldade de verdade

O projeto ficou difícil porque ele mistura várias camadas de baixo nível ao mesmo tempo:

- heap gerenciado com `mmap`;
- estrutura auxiliar para localizar intervalos de memória em tempo logarítmico;
- varredura conservadora da stack e dos registradores com `setjmp`;
- marcação e varredura de objetos vivos;
- tentativa de evoluir para uma versão geracional com páginas protegidas por `mprotect`.

O maior ponto de fricção foi perceber que remover um objeto da árvore de metadados não significava, necessariamente, devolver espaço físico para a arena do heap. Isso levou ao gargalo mais importante do trabalho: o heap enchia mesmo depois de alguns objetos já terem sido considerados mortos, porque a arena funcionava como um bump allocator e só conseguia ser resetada quando a árvore ficava vazia.

Outro problema importante foi a parte conceitual. Eu passei um bom tempo tentando encaixar, ao mesmo tempo, árvore de intervalos, cabeçalho oculto, canário, raízes da stack, marcação, sweep e depois a ideia de geração jovem/velha. A solução técnica só começou a fazer sentido quando eu simplifiquei a arquitetura e entendi cada bloco isoladamente.

## Ideia de arquitetura

A estrutura do projeto foi pensada em torno destes blocos:

- `gc_malloc()`: interface principal de alocação;
- heap próprio baseado em `mmap`;
- árvore de intervalos para mapear endereços alocados;
- `ObjHeader` para guardar tamanho, geração, marcação e canário;
- varredura conservadora para encontrar raízes;
- fases de mark e sweep;
- tentativa de separar geração jovem e geração velha.

Na prática, a árvore de intervalos virou o ponto central do projeto porque ela resolve a pergunta mais cara do coletor: se um valor encontrado na stack aponta, de fato, para dentro de um objeto alocado.

## O que eu aprendi tentando fazer isso funcionar

Os principais aprendizados do processo foram estes:

- endereços de memória em 64 bits precisam ser tratados com `uintptr_t`, não com `int`;
- passar estruturas por valor foi mais seguro do que manter ponteiros para objetos que viviam na stack;
- uma árvore de intervalos precisa estar coerente com o ciclo de vida dos objetos que ela referencia;
- varredura conservadora é um compromisso: ela prefere manter algo vivo por engano do que liberar algo que ainda pode ser acessado;
- um garbage collector geracional exige mais do que coleta: ele também exige estratégia de promoção, barreira de escrita e controle de páginas sujas.

## Estado do projeto

O repositório contém:

- implementação do heap em `implementation/heap.c`;
- implementação da árvore de intervalos em `implementation/interval-tree.c`;
- implementação principal do GC em `implementation/gc.c`;
- contratos públicos em `lib/`;
- testes em `test/`.

## Observação final

Este repositório não é só sobre ter um GC funcionando. Ele registra o caminho de entendimento do problema, os tropeços de arquitetura e as decisões tomadas para tornar o sistema explicável durante a apresentação.

Mesmo que o código ainda não esteja funcionando por completo, a intenção é voltar a mexer nele mais pra frente e continuar a construção do zero, agora com muito mais base técnica sobre o problema e sobre a arquitetura do coletor.

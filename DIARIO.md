# 02/07 - Entendendo o tema

PROMPT PRINCIPAL:

O arquivo linkado é meu trabalho de SO + ED da faculdade. Quero que me ajude a planejar, estruturar e fazer o trabalho, entendendo todos os pontos para realizar a minha apresentação, que será no dia 06. Para começar, quero que leia o tema 15, do garbage collector (que foi o qual eu escolhi) e me responda:

- qual é o problema central?
- quais componentes posso dividir esse problema?
- quais são os núcleos de SO e ED?

seja direto em cada resposta, dando apenas o necessário para depois eu analisar cada tópico com mais profundidade.

## O problema

Implementar uma biblioteca de alocação de memória (gc_malloc()) que nunca libera objetos manualmente — a liberação é feita por um coletor de lixo (garbage collector) que precisa descobrir sozinho quais objetos ainda estão "vivos" (alcançáveis a partir de raízes conhecidas: pilha, registradores, variáveis globais) e quais podem ser recuperados — incluindo objetos em ciclos de referência, que uma contagem de referência simples não resolveria.

O desafio técnico central é: dado um heap com milhões de objetos alocados via mmap, como decidir em tempo eficiente (O(log n), não O(n)) se um valor qualquer encontrado na pilha/registradores é um ponteiro válido para dentro de algum objeto vivo (varredura conservadora), e como fazer isso com pausas controladas, sem nunca coletar algo vivo nem vazar algo morto.

## Componentes

[] Alocador de heap — gerência de memória via mmap, metadados de cada objeto alocado (tamanho, geração, marcação).
[] Árvore de intervalos (interval tree) — estrutura central: mapeia faixas de endereço → objeto, para resolver "este endereço pertence a algum objeto vivo?" em O(log n).
[] Varredura de raízes (root scanning) — descoberta de ponteiros na pilha e registradores via setjmp/inspeção do stack pointer (conservative scanning).
[] Algoritmo de marcação (mark) — fila/pilha de marcação, percorrendo o grafo de objetos vivos a partir das raízes.
[] Fase de varredura/coleta (sweep) — libera objetos não marcados de volta ao heap.
[] Geracional (2 gerações) — separação jovem/velha, coleta mais frequente na geração jovem, com barreira de escrita via mprotect para detectar referências de velha→jovem.
[] Gatilho de coleta — disparo por pressão de memória ou chamada explícita.
[] Medição de pausas — clock monotônico para medir stop-the-world de cada coleta.
[] Programas-cobaia — listas/árvores/grafos com ciclos, para provar que ref-counting falharia e o GC não.

## Núcleos

### ED (Árvore de Intervalos)

- Estrutura balanceada (AVL/rubro-negra estendida) sobre os intervalos [endereço_início, endereço_fim] de cada objeto alocado.
- Operações: inserir intervalo ao alocar, remover ao coletar, consulta "contém o ponto X?" em O(log n).
- É o que torna a varredura conservadora eficiente em vez de O(n) por ponteiro candidato.

### SO (Gerência de memória)

- Heap gerenciado diretamente via mmap (não malloc do sistema).
- Varredura conservadora de pilha/registradores (setjmp, inspeção de SP).
- Barreira de escrita implementada com mprotect (protege páginas da geração velha para capturar escritas que criam referências para a geração jovem).
- Medição de pausas com clock monotônico.

## Entregas esperadas

- Código-fonte em C11, compilando com gcc -Wall -Wextra -Werror sem warnings.
- Makefile com alvos all, test, stress, clean.
- Bateria de testes própria.
- Scripts de geração dos gráficos do relatório.
- DIARIO.md (decisões de projeto, bugs, prompts de IA usados e o que teve que ser corrigido).

### Fundação

- Estrutura básica do heap gerenciado via mmap (alocação bruta, sem GC ainda).
- Árvore de intervalos implementada e testada isoladamente: inserir intervalo, remover, consultar "ponto X pertence a algum intervalo?" — com testes unitários próprios.
- Metadados por objeto (tamanho, marca, geração) definidos.
- Repositório Git com histórico de commits incrementais desde já (não pode ser "commit único").
- DIARIO.md iniciado.

### Núcleo de ED

- gc_malloc() funcional: cada alocação insere um intervalo na árvore.
- Algoritmo de mark funcionando: varredura conservadora de pilha/registradores (setjmp) encontrando raízes, marcação percorrendo o grafo de objetos vivos usando a árvore de intervalos para validar ponteiros.
- Fase de sweep: libera objetos não marcados.
- Mark-sweep completo e correto rodando em programas simples (sem geração ainda).
- Concorrência (se houver) pode usar mutex global nesta fase.

### Núcleo de SO

- Variante geracional implementada: 2 gerações, coleta mais frequente na jovem.
- Barreira de escrita com mprotect capturando escritas de velha→jovem.
- Disparo de coleta por pressão de memória (não só manual).
- Sem data races reportadas pelo ThreadSanitizer (se a coleta rodar concorrente com mutação).

## Estudos técnicos

### Árvore de Intervalos

- Entendido o funcionamento da árvore de intervalos com ajuda do Gemini e Claude.
- Implementada árvore de intervalos (AVL) com insert, remove, findPoint, findInterval.
- Removida a lógica de duplicatas por `high` no `removeInterval`: como cada objeto alocado ocupa um range de endereço único e disjunto, não há necessidade de tratar múltiplos nós com o mesmo `low`. Simplifica o delete para BST clássico.

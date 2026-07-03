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
- Árvore implementada com `int`, migrado `low`/`high`/`max` para `uintptr_t`, já que endereços de memória de 64 bits não cabem em `int`.
- Criada função `getMaxHeight` separada de `getMaxValue`, pois misturar as duas (comparar alturas `int`, que podem ser -1, usando a função de `uintptr_t`) quebrava silenciosamente o cálculo de altura de nós com filho `NULL` (valor negativo vira número gigante em tipo sem sinal).
- Corrigidos também `printTree` (formato `%ju` no lugar de `%d`) e `findPoint` (parâmetro `point` migrado para `uintptr_t`, evitando erro de comparação de sinal).

### Memória Stack e memória Heap

- Stack: Região de memória gerenciada automaticamente pelo SO/runtime, que cresce e encolhe conforme funções são chamadas e retornam. Cada chamada de função cria um "quadro" (stack frame) com variáveis locais, parâmetros e endereço de retorno; ao retornar, o frame inteiro é descartado de uma vez, sem nenhuma decisão manual de liberação. É rápida (só move um ponteiro de topo) e de vida curta — variáveis locais somem quando a função termina.
  - Valores alocados na stack:
  - Variáveis locais primitivas: int x;, double y;, char c; dentro de uma função.
  - Structs locais declaradas por valor: Point p = {1, 2};.
  - Arrays de tamanho fixo declarados localmente: int buffer[100];.
  - Parâmetros de função (as cópias locais, não o que eles apontam).
  - Ponteiros em si (a variável ponteiro ocupa espaço na stack, mesmo que aponte pra heap): int \*ptr = malloc(sizeof-(int)); — o ptr mora na stack, o int que ele aponta mora na heap.
  - Endereço de retorno e registradores salvos de cada chamada de função (o frame inteiro).

- Heap: Região de memória que não é liberada automaticamente por escopo — permanece alocada até alguém explicitamente devolvê-la (seja com free, seja pela coleta do seu GC). Diferente da stack, tem vida longa e imprevisível: um objeto pode sobreviver muito além da função que o criou.
- Valores alocados na heap:
  - Nós gerados dinamicamente para listas encadeadas, árvores, grafos: qualquer malloc(sizeof(Node)) dentro de uma função como createNode.
  - Arrays de tamanho variável em tempo de execução: int _arr = malloc(n _ sizeof(int));, onde n só é conhecido em runtime.
  - Structs retornadas por funções "fábrica" que precisam sobreviver ao fim da função que as criou: Person \*p = malloc(sizeof(Person)); dentro de create_person(), retornado pro chamador.
  - Buffers redimensionáveis: strings construídas dinamicamente, ou qualquer coisa que use realloc conforme cresce.
  - Regiões grandes mapeadas via mmap() — usado quando se quer controle direto sobre a memória (fora do alocador padrão do sistema), como um heap próprio de uma aplicação.
  - Qualquer estrutura cuja vida útil precisa ultrapassar o escopo da função que a criou — é justamente esse "sobreviver além do escopo" que caracteriza a necessidade de heap em vez de stack.

#### Heap

Para construir as funções da utilização da memória heap, foi utilizado o Claude para entendimento e diferenciação entre memória heap como conceito e memória heap como implementação. Além disso, foi utilizado para ajudar a contruir os testes unitários das implementações.

Diferença entre malloc e funções manipuladoras da heap:

- Heap com malloc:
  - Ele mantém suas próprias estruturas internas (free lists organizadas por faixa de tamanho, metadados escondidos antes de cada bloco, etc.).
  - Ele decide onde exatamente cada alocação vai (busca um buraco livre de tamanho compatível, ou fatia um maior).
  - Você não sabe, e não pode saber, onde um objeto está fisicamente, nem quais objetos estão "vizinhos" na memória, nem varrer essa região de forma organizada — é uma caixa preta.
  - A liberação é manual e individual: você chama free exatamente quando decide que aquele objeto específico não é mais necessário.

- Heap com funções implementadas:
  - Você pede uma região grande e contígua de uma vez (mmap), não um objeto por vez.
  - Você sabe exatamente onde cada objeto está dentro dessa região — é só aritmética sobre base + offset, nada escondido.
  - Como você controla o layout, você consegue varrer a região inteira sequencialmente (percorrer todos os objetos, um atrás do outro) — coisa que é impossível de fazer de forma confiável com memória alocada via malloc espalhada pelo processo.
  - Não existe free individual. Nenhum objeto é liberado sozinho quando você "termina" de usá-lo.

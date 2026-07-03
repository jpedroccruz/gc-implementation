#include "../lib/heap.h"
#include <stdio.h>
#include <assert.h>

static void it_should_create_a_heap(void) {
  Heap *h = createHeap(4096);
  assert(h != NULL);
  assert(h->capacity >= 4096);
  assert(h->offset == 0);

  destroyHeap(h);
  printf("it_should_create_a_heap: OK\n");
}

static void it_should_allocate_sequentially(void) {
  Heap *h = createHeap(4096);

  void *a = allocHeap(h, 100);
  void *b = allocHeap(h, 200);
  void *c = allocHeap(h, 50);

  assert(a != NULL && b != NULL && c != NULL);
  assert((char *)b >= (char *)a + 100);
  assert((char *)c >= (char *)b + 200);

  destroyHeap(h);
  printf("it_should_allocate_sequentially: OK\n");
}

static void it_should_write_and_read_allocated_memory(void) {
  Heap *h = createHeap(4096);
  void *a = allocHeap(h, 100);
  assert(a != NULL);

  int *int_array = (int *)a;
  for (int i = 0; i < 25; i++) int_array[i] = i; // 100 bytes = 25 ints
  for (int i = 0; i < 25; i++) assert(int_array[i] == i);

  destroyHeap(h);
  printf("it_should_write_and_read_allocated_memory: OK\n");
}

static void it_should_not_allocate_more_than_capacity(void) {
  Heap *h = createHeap(4096);

  // consome uma parte do heap primeiro
  void *a = allocHeap(h, 100);
  assert(a != NULL);

  // tenta alocar mais do que resta (pedindo a capacidade inteira)
  void *overflow = allocHeap(h, h->capacity);
  assert(overflow == NULL);

  destroyHeap(h);
  printf("it_should_not_allocate_more_than_capacity: OK\n");
}

static void it_should_return_null_when_heap_is_null(void) {
  void *result = allocHeap(NULL, 10);
  assert(result == NULL);
  printf("it_should_return_null_when_heap_is_null: OK\n");
}

static void it_should_return_null_for_zero_byte_allocation(void) {
  Heap *h = createHeap(4096);
  void *result = allocHeap(h, 0);
  assert(result == NULL);

  destroyHeap(h);
  printf("it_should_return_null_for_zero_byte_allocation: OK\n");
}

static void it_should_reset_cursor_to_beginning(void) {
  Heap *h = createHeap(4096);

  allocHeap(h, 100);
  allocHeap(h, 200);
  assert(h->offset > 0);

  resetHeap(h);
  assert(h->offset == 0);

  destroyHeap(h);
  printf("it_should_reset_cursor_to_beginning: OK\n");
}

static void it_should_allocate_from_base_after_reset(void) {
  Heap *h = createHeap(4096);

  allocHeap(h, 100);
  allocHeap(h, 200);
  resetHeap(h);

  void *d = allocHeap(h, 100);
  assert(d != NULL);
  assert(d == h->base);

  destroyHeap(h);
  printf("it_should_allocate_from_base_after_reset: OK\n");
}

static void it_should_round_capacity_up_to_page_size(void) {
  Heap *h = createHeap(1000);
  assert(h != NULL);
  assert(h->capacity == 4096);

  destroyHeap(h);
  printf("it_should_round_capacity_up_to_page_size: OK\n");
}

int main(void) {
  it_should_create_a_heap();
  it_should_allocate_sequentially();
  it_should_write_and_read_allocated_memory();
  it_should_not_allocate_more_than_capacity();
  it_should_return_null_when_heap_is_null();
  it_should_return_null_for_zero_byte_allocation();
  it_should_reset_cursor_to_beginning();
  it_should_allocate_from_base_after_reset();
  it_should_round_capacity_up_to_page_size();

  printf("\nAll heap tests passed.\n");
  return 0;
}
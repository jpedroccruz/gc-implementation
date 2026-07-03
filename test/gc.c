#include "../lib/gc.h"
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>

#define GC_CANARY_VALUE 0xDEADC0DE

extern Node *global_root;

static int tree_contains(Node *root, uintptr_t low) {
  if (root == NULL) return 0;
  if (root->i.low == low) return 1;
  
  if (low < root->i.low) return tree_contains(root->left, low);
  return tree_contains(root->right, low);
}

static void it_should_initialize_gc_and_allocate_memory(void) {
  void *ptr = gc_malloc(100);
  assert(ptr != NULL);

  printf("it_should_initialize_gc_and_allocate_memory: OK\n");
}

static void it_should_create_hidden_header_with_correct_metadata(void) {
  size_t user_size = 64;
  void *ptr = gc_malloc(user_size);
  assert(ptr != NULL);

  ObjHeader *header = (ObjHeader *)((char *)ptr - sizeof(ObjHeader));

  assert(header->size == user_size);
  assert(header->generation == 0); 
  assert(header->marked == 0);     

  printf("it_should_create_hidden_header_with_correct_metadata: OK\n");
}

static void it_should_preserve_canary_integrity(void) {
  void *ptr = gc_malloc(32);
  assert(ptr != NULL);

  ObjHeader *header = (ObjHeader *)((char *)ptr - sizeof(ObjHeader));
  
  assert(header->canary == GC_CANARY_VALUE);

  printf("it_should_preserve_canary_integrity: OK\n");
}

static void it_should_insert_allocated_interval_into_avl_tree(void) {
  Node *previous_root = global_root;

  void *ptr = gc_malloc(120);
  assert(ptr != NULL);

  assert(global_root != NULL);

  ObjHeader *header = (ObjHeader *)((char *)ptr - sizeof(ObjHeader));
  uintptr_t expected_low = (uintptr_t)header;

  assert(tree_contains(previous_root, expected_low));

  printf("it_should_insert_allocated_interval_into_avl_tree: OK\n");
}

static void it_should_read_and_write_safely_inside_user_space(void) {
  size_t size = 20;
  char *text = (char *)gc_malloc(size);
  assert(text != NULL);

  strcpy(text, "IFES 2026");
  assert(strcmp(text, "IFES 2026") == 0);

  ObjHeader *header = (ObjHeader *)((char *)text - sizeof(ObjHeader));
  assert(header->canary == GC_CANARY_VALUE);

  printf("it_should_read_and_write_safely_inside_user_space: OK\n");
}

static void it_should_return_null_for_zero_byte_gc_allocation(void) {
  void *ptr = gc_malloc(0);
  assert(ptr == NULL);

  printf("it_should_return_null_for_zero_byte_gc_allocation: OK\n");
}

// gc_collect is not implemented, so it should return a overflow of generation 0
static void it_should_return_null_on_heap_overflow(void) {
  size_t giant_size = 5 * 1024 * 1024;
  void *ptr = gc_malloc(giant_size);
  
  assert(ptr == NULL);

  printf("it_should_return_null_on_heap_overflow: OK\n");
}

static void generate_garbage(size_t chunk_size) {
  void *p1 = gc_malloc(chunk_size);
  void *p2 = gc_malloc(chunk_size);
    
  assert(p1 != NULL);
  assert(p2 != NULL);
}

static void it_should_trigger_gc_automatically_when_heap_is_full(void) {
  size_t large_chunk = 400000; 
  generate_garbage(large_chunk);

  volatile char dummy[1024];
  memset((void *)dummy, 0, sizeof(dummy));

  void *new_ptr = gc_malloc(large_chunk);

  assert(new_ptr != NULL); 
  printf("it should trigger gc automatically when heap is full: OK\n", new_ptr);
}

int main(void) {
  it_should_initialize_gc_and_allocate_memory();
  it_should_create_hidden_header_with_correct_metadata();
  it_should_preserve_canary_integrity();
  it_should_insert_allocated_interval_into_avl_tree();
  it_should_read_and_write_safely_inside_user_space();
  it_should_return_null_for_zero_byte_gc_allocation();
  it_should_return_null_on_heap_overflow();

  printf("\nAll gc tests passed.\n");
  return 0;
}
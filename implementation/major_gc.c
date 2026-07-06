#define _GNU_SOURCE

#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <setjmp.h>

#include <sys/mman.h>

#include "../lib/gc.h"

// ================= DEPENDENCIES =================

static void collect_old_dead_intervals(Node *root) {
  if (root == NULL || oldDeadCount >= (MAX_GARBAGE * 1024)) return;

  collect_old_dead_intervals(root->left);

  ObjHeader *header = (ObjHeader *)root->i.low;

  if (header->generation == 1 && header->marked == 0) {
    oldDeadIntervals[oldDeadCount++] = root->i;
  }

  collect_old_dead_intervals(root->right);
}

static void add_to_free_list(uintptr_t address, size_t size) {
  FreeBlock *new_block = NULL;

  if (availableBlocks == NULL) {
    new_block = (FreeBlock *)malloc(sizeof(FreeBlock));

    if (new_block == NULL) {
      perror("Emergency pool block allocation failed.");
      exit(1);
    }
  } else {
    new_block = availableBlocks;
    availableBlocks = availableBlocks->next;
  }

  new_block->address = address;
  new_block->size = size;

  FreeBlock *current = oldFreeList;
  FreeBlock *prev = NULL;
  while (current != NULL && current->address < address) {
    prev = current;
    current = current->next;
  }

  if (prev == NULL) {
    new_block->next = oldFreeList;
    oldFreeList = new_block;
  } else {
    new_block->next = current;
    prev->next = new_block;
  }

  // Fusion Alg (Coalescing): Join neightboor free blocks, reducing fragmentation
  current = oldFreeList;
  while (current != NULL && current->next != NULL) {
    if (current->address + current->size == current->next->address) {
      FreeBlock *duplicate = current->next;
      current->size += duplicate->size;
      current->next = duplicate->next;

      duplicate->next = availableBlocks;
      availableBlocks = duplicate;
    } else {
      current = current->next;
    }
  }
}

// =================== MAJOR GC ===================

void major_gc_sweep() {
  oldDeadCount = 0;
  collect_old_dead_intervals(global_root);

  for (int i = 0; i < oldDeadCount; i++) {
    removeInterval(&global_root, oldDeadIntervals[i]);
    size_t size = oldDeadIntervals[i].high - oldDeadIntervals[i].low;
    add_to_free_list(oldDeadIntervals[i].low, size);
  }
}

void major_gc_collect(void) {
  if (stack_base == NULL) return;

  if (mprotect(oldGeneration->base, oldGeneration->capacity, PROT_READ | PROT_WRITE) == -1) {
    perror("Fail to unprotect Old Generation for Major GC");
    exit(1);
  }

  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  
  reset_all_marks(global_root);

  jmp_buf registers;
  setjmp(registers);

  void *stack_top = &registers;

  uintptr_t *current = (uintptr_t *)stack_top;
  uintptr_t *end = (uintptr_t *)stack_base;

  if (current > end) {
    uintptr_t *tmp = current;
    current = end;
    end = tmp;
  }

  ObjHeader *mark_stack[MAX_STACK_SIZE];
  int mark_stack_top = 0;

  // Stack Mark (roots: stack, registers, global variables)
  while (current < end) {
    uintptr_t pointer = *current;
    Node *node = findPoint(global_root, pointer);

    if (node) {
      ObjHeader *header = (ObjHeader *)node->i.low;
      if (header->marked == 0) {
        header->marked = 1;
        if (mark_stack_top < MAX_STACK_SIZE) mark_stack[mark_stack_top++] = header;
      }
    } 

    current++;
  }

  // Mark objects from other structures, as graphs, trees and lists
  while (mark_stack_top > 0) {
    ObjHeader *header = mark_stack[--mark_stack_top];
    uintptr_t *obj_current = (uintptr_t *)((char *)header + sizeof(ObjHeader));
    uintptr_t *obj_end = (uintptr_t *)((char *)obj_current + header->size);

    while (obj_current < obj_end) {
      uintptr_t internal_pointer = *obj_current;
      Node *internal_node = findPoint(global_root, internal_pointer);

      if (internal_node) {
        ObjHeader *internal_header = (ObjHeader *)internal_node->i.low;
        if (internal_header->marked == 0) {
          internal_header->marked = 1;
          if (mark_stack_top < MAX_STACK_SIZE) mark_stack[mark_stack_top++] = internal_header;
        }
      }
      obj_current++;
    }
  }

  major_gc_sweep();

  if (mprotect(oldGeneration->base, oldGeneration->capacity, PROT_READ) == -1) {
    perror("Fail to lock Old Generation after Major GC.");
    exit(1);
  }

  dirtyPagesCount = 0;

  struct timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &end_time);

  long long time_in_ms = (end_time.tv_sec - start_time.tv_sec) * 1000000000LL + (end_time.tv_nsec - start_time.tv_nsec);
  printf("Major Pause:: %.3f ms\n", time_in_ms / 1000000.0);
}
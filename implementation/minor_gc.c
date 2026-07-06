#define _GNU_SOURCE

#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <setjmp.h>

#include <sys/mman.h>
#include <unistd.h>

#include "../lib/gc.h"

// ================= DEPENDENCIES =================

static void collect_young_intervals(Node *root) {
  if (root == NULL || youngCount >= MAX_GARBAGE) return;

  collect_young_intervals(root->left);

  ObjHeader *header = (ObjHeader *)root->i.low;

  if (header->generation == 0) youngIntervals[youngCount++] = root->i;

  collect_young_intervals(root->right);
}

static void *allocate_from_free_list(size_t size) {
  FreeBlock *current = oldFreeList;
  FreeBlock *prev = NULL;

  while (current != NULL) {
    if (current->size >= size) {
      void *ptr = (void *)current->address;
      size_t remainder = current->size - size;

      if (remainder >= sizeof(ObjHeader) + 8) {
        current->address += size;
        current->size = remainder;
      } else {
        if (prev == NULL) oldFreeList = current->next;
        else prev->next = current->next;

        current->next = availableBlocks;
        availableBlocks = current;
      }
      return ptr;
    }
    prev = current;
    current = current->next;
  }
  return NULL;
}

static void *allocate_old_generation(size_t totalSize) {
  void *mem = allocate_from_free_list(totalSize);
  if (mem != NULL) return mem;
  return allocHeap(oldGeneration, totalSize);
}

// =================== MINOR GC ===================

void minor_gc_collect(void) {
  if (stack_base == NULL) return;

  struct timespec start_time;
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  
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

  uintptr_t young_base = (uintptr_t)youngGeneration->base;
  uintptr_t young_end = young_base + youngGeneration->capacity;

  ObjHeader *mark_stack[MAX_STACK_SIZE];
  int mark_stack_top = 0;

  // Stack Mark (roots: stack and registers)
  while (current < end) {
    uintptr_t pointer = *current;
    
    if (pointer >= young_base && pointer <= young_end) {
      Node *node = findPoint(global_root, pointer);

      if (node) {
        ObjHeader *header = (ObjHeader *)node->i.low;
        if (header->marked == 0) {
          header->marked = 1;
          if (mark_stack_top < MAX_STACK_SIZE) mark_stack[mark_stack_top++] = header;
        }
      } 
    } 

    current++;
  }

  // Mark Old Pages (remembered set)
  long page_size = sysconf(_SC_PAGESIZE);
  for (int i = 0; i < dirtyPagesCount; i++) {
    uintptr_t *page_current = (uintptr_t *)dirtyPages[i];
    uintptr_t *page_end = (uintptr_t *)((char *)dirtyPages[i] + page_size);
    
    while (page_current < page_end) {
      uintptr_t pointer = *page_current;
      if (pointer >= young_base && pointer <= young_end) {
      Node *node = findPoint(global_root, pointer);
  
      if (node) {
        ObjHeader *header = (ObjHeader *)node->i.low;
        if (header->marked == 0) {
          header->marked = 1;
          if (mark_stack_top < MAX_STACK_SIZE) mark_stack[mark_stack_top++] = header;
          }
        } 
      } 

      page_current++;
    }
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

  minor_gc_sweep();
  reset_all_marks(global_root);

  struct timespec end_time;
  clock_gettime(CLOCK_MONOTONIC, &end_time);

  long long time_in_ms = (end_time.tv_sec - start_time.tv_sec) * 1000000000LL + (end_time.tv_nsec - start_time.tv_nsec);
  printf("Minor Pause:: %.3f ms\n", time_in_ms / 1000000.0);
}

void minor_gc_sweep(void) {
  youngCount = 0;
  collect_young_intervals(global_root);

  for (int i = 0; i < youngCount; i++) {
    uintptr_t low = youngIntervals[i].low;
    ObjHeader *header = (ObjHeader *)low;
    size_t totalSize = sizeof(ObjHeader) + header->size;

    if (header->marked == 1) {
      void *new_mem = allocHeap(oldGeneration, totalSize);

      if (new_mem == NULL) {
        mprotect(oldGeneration->base, oldGeneration->capacity, PROT_READ | PROT_WRITE);
        
        major_gc_collect();
        new_mem = allocHeap(oldGeneration, totalSize);

        if (new_mem == NULL) {
          perror("Failed to allocate memory in Old Generation.");
          exit(1);
        }
      }

      memcpy(new_mem, (void *)low, totalSize);

      ObjHeader *new_header = (ObjHeader *)new_mem;
      new_header->generation = 1;
      new_header->marked = 0;
      new_header->forwarded = NULL;

      header->forwarded = (void *)((uintptr_t)new_mem + sizeof(ObjHeader));

      uintptr_t new_low = (uintptr_t)new_mem;
      uintptr_t new_high = new_low + totalSize;
      Interval new_i = {new_low, new_high};

      insert(&global_root, new_i);
    }
  }

  jmp_buf sweep_registers;
  setjmp(sweep_registers);

  void *stack_top = &sweep_registers;
  uintptr_t *current = (uintptr_t *)stack_top;
  uintptr_t *end = (uintptr_t *)stack_base;

  if (current > end) {
    uintptr_t *tmp = current;
    current = end;
    end = tmp;
  }

  uintptr_t young_base = (uintptr_t)youngGeneration->base;
  uintptr_t young_end = young_base + youngGeneration->capacity;

  while (current < end) {
    uintptr_t pointer = *current;
    if (pointer >= young_base && pointer < young_end) {
      Node *node = findPoint(global_root, pointer);
      if (node) {
        ObjHeader *old_header = (ObjHeader *)node->i.low;
        if (old_header->marked == 1 && old_header->forwarded != NULL) {
          *current = (uintptr_t)old_header->forwarded;
        }
      }
    }
    current++;
  }

  long page_size = sysconf(_SC_PAGESIZE);
  for (int p = 0; p < dirtyPagesCount; p++) {
    uintptr_t *pg_current = (uintptr_t *)dirtyPages[p];
    uintptr_t *pg_end = (uintptr_t *)((char *)dirtyPages[p] + page_size);
    while (pg_current < pg_end) {
      uintptr_t pointer = *pg_current;
      if (pointer >= young_base && pointer < young_end) {
        Node *node = findPoint(global_root, pointer);
        if (node) {
          ObjHeader *old_header = (ObjHeader *)node->i.low;
          if (old_header->marked == 1 && old_header->forwarded != NULL) {
            *pg_current = (uintptr_t)old_header->forwarded;
          }
        }
      }
      pg_current++;
    }
  }

  for (int i = 0; i < youngCount; i++) {
    removeInterval(&global_root, youngIntervals[i]);
    ObjHeader *header = (ObjHeader *)youngIntervals[i].low;
    header->marked = 0;
    header->forwarded = NULL;
  }

  resetHeap(youngGeneration);

  if (mprotect(oldGeneration->base, oldGeneration->capacity, PROT_READ) == -1) {
    perror("Fail to lock Old Generation write.");
    exit(1);
  }

  dirtyPagesCount = 0;
}
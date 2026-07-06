#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <string.h>
#include <time.h>

#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>

#include "../lib/gc.h"

#define YOUNG_GEN_SIZE (1024 * 1024)
#define OLD_GEN_SIZE (4 * 1024 * 1024)
#define MAX_GARBAGE 1024
#define MAX_STACK_SIZE 8192
#define MAX_DIRTY_PAGES 1024
#define MAX_FREE_BLOCKS 2048

Heap *youngGeneration = NULL;
Interval youngIntervals[MAX_GARBAGE];
int youngCount = 0;

Heap *oldGeneration = NULL;
Interval oldDeadIntervals[MAX_GARBAGE * 1024];
int oldDeadCount = 0;

void *dirtyPages[MAX_DIRTY_PAGES];
int dirtyPagesCount = 0;

FreeBlock freeBlockPool[MAX_FREE_BLOCKS];
FreeBlock *availableBlocks = NULL;
FreeBlock *oldFreeList = NULL;

Node *global_root = NULL;
void *stack_base = NULL;

// ================= DEPENDENCIES =================

static void sigsegv_handler(int sig, siginfo_t *info, void *ucontext) {
  void *fault_addr = info->si_addr;
  
  uintptr_t old_base = (uintptr_t)oldGeneration->base;
  uintptr_t old_end = old_base + oldGeneration->capacity;
  
  if ((uintptr_t)fault_addr >= old_base && (uintptr_t)fault_addr < old_end) {
    long page_size = sysconf(_SC_PAGESIZE);
    uintptr_t page_start = ((uintptr_t)fault_addr) & ~(page_size - 1);
    
    if (mprotect((void *)page_start, page_size, PROT_READ | PROT_WRITE) == -1) {
      perror("Falha no mprotect do sinal");
      exit(1);
    }

    int exists = 0;
    for (int i = 0; i < dirtyPagesCount; i++) {
      if (dirtyPages[i] == (void *)page_start) {
        exists = 1;
        break;
      }
    }

    if (!exists && dirtyPagesCount < MAX_DIRTY_PAGES) {
      dirtyPages[dirtyPagesCount++] = (void *)page_start;
    }
  } else {
    fprintf(stderr, "Segmentation faul on address %p\n", fault_addr);
    exit(1);
  }
}

static void init_free_block_pool(void) {
  for (int i = 0; i < MAX_FREE_BLOCKS - 1; i++) {
    freeBlockPool[i].next = &freeBlockPool[i + 1];
  }
  freeBlockPool[MAX_FREE_BLOCKS - 1].next = NULL;
  availableBlocks = &freeBlockPool[0];
  oldFreeList = NULL;
}

void reset_all_marks(Node *root) {
  if (root == NULL) return;

  ObjHeader *header = (ObjHeader *)root->i.low;
  header->marked = 0;

  reset_all_marks(root->left);
  reset_all_marks(root->right);
}

// =================== GC MALLOC ==================

void gc_init(void *stack_bottom) {
  stack_base = stack_bottom;

  init_free_block_pool();

  if (youngGeneration == NULL) {
    youngGeneration = createHeap(YOUNG_GEN_SIZE);

    if (!youngGeneration) {
      perror("Failed to init GC Heap.");
      exit(1);
    }
  }

  if (oldGeneration == NULL) {
    oldGeneration = createHeap(OLD_GEN_SIZE);

    if (!oldGeneration) {
      perror("Failed to init Old Gen Heap.");
      exit(1);
    }
  }

  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = sigsegv_handler;
  sigemptyset(&sa.sa_mask);
  if (sigaction(SIGSEGV, &sa, NULL) == -1) {
    perror("Fail to register sigaction.");
    exit(1);
  }
}

void *gc_malloc(size_t size) {
  if (size == 0) return NULL;

  if (!youngGeneration || !oldGeneration) {
    perror("GC was not initialized.");
    exit(1);
  }
  
  size_t totalSize = sizeof(ObjHeader) + size;
  void *mem = allocHeap(youngGeneration, totalSize);
  int 

  if (mem == NULL) { 
    minor_gc_collect();
    mem = allocHeap(youngGeneration, totalSize);

    if (mem == NULL) {
      major_gc_collect();
      mem = allocHeap(oldGeneration, totalSize);

      if (mem == NULL) {
        perror("Out of Memory on Young Generation.");
        exit(1);
      } 
    }
  }

  ObjHeader *header = (ObjHeader *)mem;
  header->size = size;
  header->marked = 0;
  header->canary = 0xDEADC0DE;
  header->forwarded = NULL;

  void *pointer = (char *)mem + sizeof(ObjHeader);

  uintptr_t low = (uintptr_t)mem;
  uintptr_t high = low + totalSize;
  Interval i = {low, high};

  insert(&global_root, i);

  return pointer;
}

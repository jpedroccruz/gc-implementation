#include "../lib/heap.h"
#define _GNU_SOURCE
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

static size_t roundUpToPage(size_t size) {
  long page_size = sysconf(_SC_PAGESIZE);
  if (page_size <= 0) page_size = 4096;

  size_t remainder = size % (size_t)page_size;
  if (remainder == 0) return size;
  return size + ((size_t)page_size - remainder);
}

Heap *createHeap(size_t size) {
  Heap *h = malloc(sizeof(Heap));
  if (!h) return NULL;

  size_t mapped_size = roundUpToPage(size);

  void *memory = mmap(NULL, mapped_size,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS,
                      -1, 0);

  if (memory == MAP_FAILED) {
    perror("mmap");
    free(h);
    return NULL;
  }

  h->base = memory;
  h->capacity = mapped_size;
  h->offset = 0;

  return h;
}

static size_t align(size_t size) {
  return (size + 7) & ~((size_t)7);
}

void *allocHeap(Heap *h, size_t size) {
  if (!h || size == 0) return NULL;

  size_t aligned = align(size);

  if (h->offset + aligned > h->capacity) return NULL;

  void *pointer = (char *)h->base + h->offset;
  h->offset += aligned;

  return pointer;
}

void resetHeap(Heap *h) {
  if (!h) return;
  h->offset = 0;
}

void destroyHeap(Heap *h) {
  if (!h) return;
  munmap(h->base, h->capacity);
}
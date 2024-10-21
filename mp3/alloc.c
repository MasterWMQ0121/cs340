/**
 * Malloc
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct _metadata_t {
  unsigned int size;     // The size of the memory block.
  unsigned char isUsed;  // 0 if the block is free; 1 if the block is used.
  struct _metadata_t *next;  // Pointer to the next metadata_t.
  struct _metadata_t *prev;  // Pointer to the previous metadata_t.
} metadata_t;

metadata_t *startOfHeap = NULL;
metadata_t *endOfHeap = NULL;
metadata_t *startOfFree = NULL;
metadata_t *endOfFree = NULL;


/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
  // implement calloc:
  void *ptr = malloc(num * size);
  if (ptr) {
    memset(ptr, 0, num * size);
  }
  return ptr;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
  if (startOfHeap == NULL) {
    metadata_t *metadata = sbrk(sizeof(metadata_t));
    metadata->size = size;
    metadata->isUsed = 1;
    metadata->next = NULL;
    metadata->prev = NULL;
    startOfHeap = metadata;
    endOfHeap = metadata;
    void *ptr = (void *)metadata + sizeof(metadata_t);
    ptr = sbrk(size);
    return ptr;
  }
  metadata_t *bestFit = NULL;
  metadata_t *metadata = startOfFree;
  while (metadata != NULL) {
    if (metadata->size >= size) {
      if (bestFit == NULL || metadata->size < bestFit->size) {
        bestFit = metadata;
      }
    }
    metadata = metadata->next;
  }
  if (bestFit == NULL) {
    metadata_t *metadata = sbrk(sizeof(metadata_t));
    metadata->size = size;
    metadata->isUsed = 1;
    endOfHeap = metadata;
    metadata->next = NULL;
    metadata->prev = NULL;
    void *ptr = (void *)metadata + sizeof(metadata_t);
    ptr = sbrk(size);
    return ptr;
  }
  if (bestFit->size > size + sizeof(metadata_t)) {
    metadata_t *newMetadata = (metadata_t *)((char *)bestFit + sizeof(metadata_t) + size);
    newMetadata->size = bestFit->size - sizeof(metadata_t) - size;
    newMetadata->isUsed = 0;
    newMetadata->next = bestFit->next;
    if (bestFit->next != NULL) {
      bestFit->next->prev = newMetadata;
    } else {
      endOfFree = newMetadata;
    }
    newMetadata->prev = bestFit->prev;
    if (bestFit->prev != NULL) {
      bestFit->prev->next = newMetadata;
    } else {
      startOfFree = newMetadata;
    }
    if (bestFit == endOfHeap) {
      endOfHeap = newMetadata;
    }
    bestFit->size = size;
  }
  bestFit->isUsed = 1;
  bestFit->next = NULL;
  bestFit->prev = NULL;
  void *ptr = (void *)bestFit + sizeof(metadata_t);
  return ptr;
}


/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
  metadata_t *metadata = (metadata_t*)((char*)ptr - sizeof(metadata_t));
  metadata->isUsed = 0;
  if (startOfFree == NULL) {
    startOfFree = metadata;
    endOfFree = metadata;
    metadata->next = NULL;
    metadata->prev = NULL;
  } else if (metadata < startOfFree) {
    metadata->next = startOfFree;
    metadata->prev = NULL;
    startOfFree->prev = metadata;
    startOfFree = metadata;
  } else if (metadata > endOfFree) {
    metadata->next = NULL;
    metadata->prev = endOfFree;
    endOfFree->next = metadata;
    endOfFree = metadata;
  } else {
    metadata_t *current = startOfFree;
    while (current != NULL) {
      if (metadata < current) {
        metadata->next = current;
        metadata->prev = current->prev;
        current->prev->next = metadata;
        current->prev = metadata;
        break;
      }
      current = current->next;
    }
  }
  if (startOfFree != NULL && metadata != endOfHeap && metadata != endOfFree) {  
    if (metadata == (void*)metadata->next - metadata->size - sizeof(metadata_t)) {
      metadata->size += metadata->next->size + sizeof(metadata_t);
      metadata->next = metadata->next->next;
      if (metadata->next != NULL) {
        metadata->next->prev = metadata;
      } else {
        endOfFree = metadata;
      }
    }
  }
  if (startOfFree != NULL && metadata != startOfHeap && metadata != startOfFree) {
    if (metadata->prev == (void*)metadata - metadata->prev->size - sizeof(metadata_t)) {
      metadata->prev->size += metadata->size + sizeof(metadata_t);
      metadata->prev->next = metadata->next;
      if (metadata->next != NULL) {
        metadata->next->prev = metadata->prev;
      } else {
        endOfFree = metadata->prev;
      }
    }
  }
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
  // implement realloc:
  if (size == 0) {
    return NULL;
  }
  metadata_t *metadata = (metadata_t *)((char *)ptr - sizeof(metadata_t));
  if (metadata->size >= size) {
    return ptr;
  }
  void *newPtr = malloc(size);
  memcpy(newPtr, ptr, metadata->size);
  free(ptr);
  return newPtr;
}
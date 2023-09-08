#ifndef MEM_HPP
#define MEM_HPP

#include "general.hpp"
#include <malloc.h>

namespace sat {

// Simple allocator which is just a wrapper for malloc and free
struct CAllocator {
  template <typename T>
  static T *construct(size n = 1) {
    return (T *)malloc(usize(n) * sizeof(T));
  }

  template <typename T>
  static void destruct(T *ptr) {
    free(ptr);
  }
};

struct BumpAllocator {
  static const i32 capacity = 1024 * 1024;

  void init() {
    INIT_MEM
    offset = 0;
    data   = CAllocator::construct<i8>(capacity);
  }

  void destroy() {
    CAllocator::destruct(data);
    DESTROY_MEM
  }

  template <typename T>
  T *construct(size n = 1) {
    ASSERT_MEM

    // Ensure all allocations are aligned to the size of a pointer
    size pointer_size = size(sizeof(intptr_t));
    i32 bytes         = i32((size(sizeof(T)) * n + pointer_size - 1) & ~(pointer_size - 1));

#if DEBUG
    if (offset + bytes <= capacity) {
#endif
      T *ptr = (T *)(data + offset);
      offset += bytes;
      return ptr;
#if DEBUG
    }
    panic("BumpAllocator cannot exceed total of %d bytes in allocation\n", capacity);
#endif
  }

  i8 *data;
  i32 offset;
  DEFINE_MEM
};

struct Allocator {

  struct Block {
    i8 *data;
    i32 offset;
  };

  DEFINE_MEM
};

} // namespace sat

#endif

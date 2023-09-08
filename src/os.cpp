#include "os.hpp"

#include "mem.hpp"
#include <cstring>

namespace sat {

File read_file(cstr file_path) {
  File file;
  file.data   = nullptr;
  file.length = 0;

  auto *fstream = fopen(file_path, "rb");
  if (!fstream) return file;

  u32 capacity = 0x100;
  u32 length   = 0;

  char *buffer = CAllocator::construct<char>(capacity);
  for (;;) {
    u32 remaining_capacity = capacity - length;
    length += fread(buffer + length, 1, remaining_capacity, fstream);

    if (length != capacity) {
      if (feof(fstream)) {
        file.data   = buffer;
        file.length = i32(length);
      }

      fclose(fstream);
      return file;
    }

    capacity         = (capacity << 1) - (capacity >> 1) + 8;
    char *new_buffer = CAllocator::construct<char>(capacity);
    memcpy(new_buffer, buffer, length);
    CAllocator::destruct(buffer);
    buffer = new_buffer;
  }
}

} // namespace sat

#ifndef OS_HPP
#define OS_HPP

#include "general.hpp"

namespace sat {

struct File {
  char *data;
  i32 length;
};

File read_file(cstr file_path);

} // namespace sat

#endif

#include "general.hpp"
#include <sys/stat.h>
#include <sys/types.h>

char *append_int(char *output, i32 value) {
  char buffer[10];
  i32 i = 0;
  if (value == 0) {
    buffer[0] = 0;
    ++i;
  } else {
    while (value) {
      assert(i < 10);
      buffer[i] = value % 10;
      value /= 10;
      ++i;
    }
  }
  for (i32 k = i - 1; k >= 0; --k) {
    *output = buffer[k] + '0';
    ++output;
  }
  return output;
}

char *append_string(char *output, const char *string) {
  while (*string) {
    *output = *string;
    ++output;
    ++string;
  }
  return output;
}

void generate(i32 test_number, f64 ratio) {
  i32 variable_count = 200;
  i32 clause_count   = variable_count * ratio;

  char buf[64];
  char *dst = buf;
  dst       = append_string(dst, "test_gen/suite/ratio");
  dst       = append_int(dst, (i32)(ratio * 10));
  *(dst++)  = '/';
  dst       = append_int(dst, test_number);
  *(dst++)  = '_';
  dst       = append_int(dst, variable_count);
  *(dst++)  = '_';
  dst       = append_int(dst, clause_count);
  dst       = append_string(dst, ".cnf");
  *dst      = '\0';

  printf("Generating %s...\n", buf);

  FILE *file = fopen(buf, "w");
  if (!file) return;

  fprintf(file, "c Test file generated for ratio %f\n", ratio);
  fprintf(file, "c By Calvin Khiddee-Wu\n");
  fprintf(file, "p cnf %d %d\n", variable_count, clause_count);

  for (int i = 0; i < clause_count; ++i) {
    for (int k = 0; k < 3; ++k) {
      if (fast_random(2) == 1) {
        fprintf(file, "-");
      }
      fprintf(file, "%d ", fast_random(variable_count) + 1);
    }
    fprintf(file, "0\n");
  }
}

i32 main() {
  f64 min_ratio = 3;
  f64 max_ratio = 4;

  f64 ratio = min_ratio;
  for (i32 i = 0; i <= (max_ratio - min_ratio) / 0.2; ++i) {
    char buf[20];
    char *dst = append_string(buf, "test_gen/suite/ratio");
    dst       = append_int(dst, (i32)(ratio * 10));
    *dst      = '\0';
    mkdir(buf, 0777);
    printf(":%s\n", buf);

    for (i32 k = 0; k < 3; ++k) {
      generate(k, ratio);
    }
    ratio += 0.2;
  }

  return 0;
}

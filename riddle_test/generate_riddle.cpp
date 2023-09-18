#include "general.hpp"

enum VariableEncoding : i32 {
  Red = 0,
  Blue,
  Green,
  Yellow,
  White,

  Brit,
  Swede,
  Dane,
  Norwegian,
  German,

  Dog,
  Bird,
  Cat,
  Horse,
  Fish,

  Tea,
  Coffee,
  Milk,
  Beer,
  Water,

  PallMall,
  Dunhill,
  Blends,
  Bluemasters,
  Prince,

  NumElements,
};

static cstr variable_names[25] = {
    "Red",      "Blue",    "Green",  "Yellow",      "White",

    "Brit",     "Swede",   "Dane",   "Norwegian",   "German",

    "Dog",      "Bird",    "Cat",    "Horse",       "Fish",

    "Tea",      "Coffee",  "Milk",   "Beer",        "Water",

    "PallMall", "Dunhill", "Blends", "Bluemasters", "Prince",
};

void print_name(i32 variable_id) {
  i32 variable = (variable_id - 1) / 5;
  i32 position = (variable_id - 1) % 5;
  printf("%s%d\n", variable_names[variable], position);
}

struct Clause {
  i32 *variables;
  i32 variable_count;
  Clause *next;
};

i32 clause_count = 0;
Clause *head     = nullptr;

Clause *new_clause(i32 variable_count) {
  ++clause_count;

  auto *clause      = (Clause *)malloc(sizeof(Clause));
  clause->variables = (i32 *)malloc(sizeof(i32) * (usize)variable_count);

  for (i32 i = 0; i < variable_count; ++i) {
    clause->variables[i] = 0;
  }
  clause->variable_count = variable_count;
  clause->next           = head;
  head                   = clause;
  return clause;
}

i32 var(i32 variable, i32 position) { return (variable * 5) + position + 1; }

void pair(VariableEncoding variable1, VariableEncoding variable2) {
  for (i32 i = 0; i < 5; ++i) {
    for (i32 k = 0; k < 5; ++k) {
      if (i == k) continue;
      auto *clause         = new_clause(2);
      clause->variables[0] = -var(variable1, i);
      clause->variables[1] = -var(variable2, k);
    }
  }
}

void next_to(VariableEncoding variable1, VariableEncoding variable2) {
  for (i32 i = 0; i < 5; ++i) {
    for (i32 k = 0; k < 5; ++k) {
      if ((k - i == 1) || (i - k == 1)) continue;

      auto *clause         = new_clause(2);
      clause->variables[0] = -var(variable1, i);
      clause->variables[1] = -var(variable2, k);
    }
  }
}

void left_of(VariableEncoding variable1, VariableEncoding variable2) {
  for (i32 i = 0; i < 5; ++i) {
    for (i32 k = 0; k < 5; ++k) {
      if (i + 1 == k) continue;

      auto *clause         = new_clause(2);
      clause->variables[0] = -var(variable1, i);
      clause->variables[1] = -var(variable2, k);
    }
  }
}

void force_true(VariableEncoding variable, i32 position) { new_clause(1)->variables[0] = var(variable, position); }

i32 main(i32 argc, char **argv) {
  if (argc != 2) {
    error("Expected usage: generate_riddle [output].cnf\n");
    return err;
  }

  FILE *output = fopen(argv[1], "w");
  if (!output) return err;

  fprintf(output, "c Encoding of Einstein Riddle with 5 Houses\n");
  fprintf(output, "c By Calvin Khiddee-Wu\n");

  // Basic constraint where only 1 and at minimum one can exist: red1 or red2 or red3 or ...
  for (i32 i = 0; i < NumElements; ++i) {
    // Red1 v Red2 v Red3 v Red4 v Rec5
    auto *clause = new_clause(5);
    for (i32 k = 0; k < 5; ++k) {
      clause->variables[k] = var(i, k);
    }

    // (~Red1 v ~Red2) ^ (~Red1 v ~Red3) ^ ... ^ (~Red2 v ~Red3) ^ ...
    for (i32 m = 0; m < 5; ++m) {
      for (i32 n = m + 1; n < 5; ++n) {
        auto *clause         = new_clause(2);
        clause->variables[0] = -var(i, m);
        clause->variables[1] = -var(i, n);
      }
    }
  }

  // Constraint that a position can have at most one of a property red1 or blue1 or green1 or ...
  for (i32 i = 0; i < 5; ++i) {
    for (i32 p = 0; p < 5; ++p) {
      for (i32 m = 0; m < 5; ++m) {
        for (i32 n = m + 1; n < 5; ++n) {
          auto *clause         = new_clause(2);
          clause->variables[0] = -var(p * 5 + m, i);
          clause->variables[1] = -var(p * 5 + n, i);
        }
      }
    }
  }

  // The Brit lives in the red house.
  pair(Brit, Red);

  // The Swede keeps dogs as pets.
  pair(Swede, Dog);

  // The Dane drinks tea.
  pair(Dane, Tea);

  // The green house is on the left of the white house.
  left_of(Green, White);

  // The green house’s owner drinks coffee.
  pair(Green, Coffee);

  // The person who smokes Pall Mall rears birds.
  pair(PallMall, Bird);

  // The owner of the yellow house smokes Dunhill.
  pair(Yellow, Dunhill);

  // The man living in the center house drinks milk.
  force_true(Milk, 2);

  // The Norwegian lives in the first house.
  force_true(Norwegian, 0);

  // The man who smokes Blends lives next to the one who keeps cats.
  next_to(Blends, Cat);

  // The man who keeps the horse lives next to the man who smokes Dunhill.
  next_to(Horse, Dunhill);

  // The owner who smokes Bluemasters drinks beer.
  pair(Bluemasters, Beer);

  // The German smokes Prince.
  pair(German, Prince);

  // The Norwegian lives next to the blue house
  next_to(Norwegian, Blue);

  // The man who smokes Blends has a neighbor who drinks water
  next_to(Blends, Water);

  fprintf(output, "p cnf 125 %d\n", clause_count);
  while (head) {
    assert(head->variable_count > 0);
    for (i32 i = 0; i < head->variable_count; ++i) {
      assert(head->variables[i] != 0);
      if (head->variables[i] > 0) {
        assert(head->variables[i] <= 125);
      } else {
        assert(head->variables[i] >= -125);
      }
      fprintf(output, "%d ", head->variables[i]);
    }
    fprintf(output, "0\n");

    head = head->next;
  }

  for (i32 i = 1; i <= 125; ++i) {
    printf("x%d = ", i);
    print_name(i);
  }

  return ok;
}

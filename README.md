# sat-solver

Most of the logic code is located in `src/solver.cpp` and the main dpll procedure is `ProblemResult dpll_solve(Problem *)`

Quick test run from clean with all heuristics on riddle.cnf:
```
make clean
make generate_riddle
make build
./build/bin/sat r ./build/cnf/riddle.cnf
./build/bin/sat t ./build/cnf/riddle.cnf
./build/bin/sat p ./build/cnf/riddle.cnf
```

## Build Sat-Solver

Tested on WSL2 Ubuntu 22 with:
- make: 4.3
- clang: 14.0.0

Generate `./build/bin/sat`
```
make build
```
Build type can be changed to `Debug` (verification checks and debug print output):
```
make build BUILD_TYPE=Debug
```

## Generate Riddle CNF

```
make generate_riddle
```
Making the target will generate `build/cnf/riddle.cnf`

## Run Sat-Solver

Usage: `sat [r|t|p] [input].cnf`.

Heuristics:
- random (r): splitting rule and truth value is determine randomly
- two-clause (t): select literal with most occurences in two-clauses
- polarity (p): select literal with most occurences of the same polarity

Example usage to run solver with `random` heurisitc on `riddle.cnf`
```
./build/bin/sat r ./build/cnf/riddle.cnf
```

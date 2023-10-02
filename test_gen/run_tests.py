import os
import subprocess
import time
import statistics

directory = "./test_gen/suite"

ratios = ["ratio30", "ratio32", "ratio34", "ratio36", "ratio38",
          "ratio40", "ratio42", "ratio44", "ratio46", "ratio48",
          "ratio50", "ratio52", "ratio54", "ratio56", "ratio58",
          "ratio60"]

#heuristics = ['r', 't', 'p']
heuristics = ['p']

median_splits = []
median_times = []

prob_satisfiable = []

for r in ratios:
    actual_dir = directory + "/" + r
    files = os.listdir(actual_dir)
    print(actual_dir)

    num_sat = 0.0
    num_completed = 0.0

    splits = []
    times = []

    count = 0
    for file in files:
        full_path = actual_dir + '/' + file
        count = count + 1
        print("--- Running " + full_path + "[" + str(count) + "/" + str(len(files)) + "]")

        is_return_determined = False
        rc = 0

        is_completed = True

        for h in heuristics:
            try:
                print(h + ":")
                start = time.time_ns()
                res = subprocess.run(["./build/bin/sat", h, full_path], timeout=180, stderr=subprocess.PIPE)
                end = time.time_ns()

                split_count = int(res.stderr)
                splits.append(split_count)
                print("splits: " + str(split_count))

                elapsed_time = (end - start) / 1000000000.0
                times.append(elapsed_time)
                print("time(s): " + str(elapsed_time))

                if is_return_determined:
                    if rc != res.returncode:
                        exit("==========MISMATCHED ANSWER==========")
                else:
                    rc = res.returncode
                    is_return_determined = True


            except subprocess.TimeoutExpired:
                print("Subprocess timed out after 60 seconds.")
                is_completed = False
                break

        if is_completed:
            num_completed = num_completed + 1
            if rc == 0:
                num_sat = num_sat + 1

        print("median splits: " + str(statistics.median(splits)))
        print(median_splits)

        print("median time: " + str(statistics.median(times)))
        print(median_times)

        print("probability_sat: " + str(num_sat / num_completed) + '[' + str(num_sat) + '/' + str(num_completed) + ']')
        print(prob_satisfiable)

    median_splits.append(statistics.median(splits))
    median_times.append(statistics.median(times))
    prob_satisfiable.append(num_sat / num_completed)

print("===========================")
print(median_splits)
print(median_times)
print(prob_satisfiable)


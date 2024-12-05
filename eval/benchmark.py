import subprocess
import re
import os
from statistics import mean
from statistics import stdev
from statistics import NormalDist
from collections import defaultdict
import sys
import time
import math


eval_dir = os.getcwd()
build_dir = f"{eval_dir}/../build"
profiler_command = f"{build_dir}/ompdataperf"
comparison_prof_command = "nsys profile"

benchmarks = [
    {
        "name": "babelstream",
        "directory": f"{eval_dir}/src/BabelStream",
        "commands": [
            "./build/omp-stream -n 100 -s 1048576",
            "./build/omp-stream -n 500 -s 33554432",
            "./build/omp-stream -n 2500 -s 33554432",
        ],
        "regex": r"",
        "unit": "s",
    },
    {
        "name": "bfs",
        "directory": f"{eval_dir}/src/bfs",
        "commands": [
            "./bfs_offload 4 ../../data/bfs/graph4096.txt",
            "./bfs_offload 4 ../../data/bfs/graph65536.txt",
            "./bfs_offload 4 ../../data/bfs/graph1MW_6.txt",
        ],
        "regex": r"Compute time:\s*([\d.]+)",
        "unit": "s",
    },
    {
        "name": "bfs (fix)",
        "directory": f"{eval_dir}/src/bfs",
        "commands": [
            "./bfs_offload_fix 4 ../../data/bfs/graph4096.txt",
            "./bfs_offload_fix 4 ../../data/bfs/graph65536.txt",
            "./bfs_offload_fix 4 ../../data/bfs/graph1MW_6.txt",
        ],
        "regex": r"Compute time:\s*([\d.]+)",
        "unit": "s",
    },
    {
        "name": "hotspot",
        "directory": f"{eval_dir}/src/hotspot",
        "commands": [
            "./hotspot_offload 64 64 2 4 ../../data/hotspot/temp_64 ../../data/hotspot/power_64 output.out",
            "./hotspot_offload 512 512 2 4 ../../data/hotspot/temp_512 ../../data/hotspot/power_512 output.out",
            "./hotspot_offload 1024 1024 2 4 ../../data/hotspot/temp_1024 ../../data/hotspot/power_1024 output.out",
        ],
        "regex": r"Total time:\s*([\d.]+)",
        "unit": "s",
    },
    {
        "name": "lud",
        "directory": f"{eval_dir}/src/lud",
        "commands": [
            "./omp/lud_omp_offload -s 2000",
            "./omp/lud_omp_offload -s 4000",
            "./omp/lud_omp_offload -s 8000",
        ],
        "regex": r"Time consumed\(ms\):\s*([\d.]+)",
        "unit": "ms",
    },
    {
        "name": "minife",
        "directory": f"{eval_dir}/src/miniFE/src-openmp45-opt",
        "commands": [
            "./miniFE.x -nx 66 -ny 64 -nz 64 && cat $(ls -t miniFE.*.yaml | head -n 1)",
            "./miniFE.x -nx 132 -ny 128 -nz 128 && cat $(ls -t miniFE.*.yaml | head -n 1)",
            "./miniFE.x -nx 264 -ny 256 -nz 256 && cat $(ls -t miniFE.*.yaml | head -n 1)",
        ],
        "regex": r"Total Program Time:\s*([\d.]+)",
        "unit": "s",
    },
    {
        "name": "minife (fix)",
        "directory": f"{eval_dir}/src/miniFE/src-openmp45-opt-fix",
        "commands": [
            "./miniFE.x -nx 66 -ny 64 -nz 64 && cat $(ls -t miniFE.*.yaml | head -n 1)",
            "./miniFE.x -nx 132 -ny 128 -nz 128 && cat $(ls -t miniFE.*.yaml | head -n 1)",
            "./miniFE.x -nx 264 -ny 256 -nz 256 && cat $(ls -t miniFE.*.yaml | head -n 1)",
        ],
        "regex": r"Total Program Time:\s*([\d.]+)",
        "unit": "s",
    },
    {
        "name": "minifmm",
        "directory": f"{eval_dir}/src/minifmm",
        "commands": [
            "./fmm.omptarget -n 100",
            "./fmm.omptarget -n 1000",
            "./fmm.omptarget -n 10000",
        ],
        "regex": r"Total Time \(s\)\s*([\d.]+)",
        "unit": "s",
    },
    {
        "name": "nw",
        "directory": f"{eval_dir}/src/nw",
        "commands": [
            "./needle_offload 512 10 2",
            "./needle_offload 2048 10 2",
            "./needle_offload 8192 10 2",
        ],
        "regex": r"Total time:\s*([\d.]+)",
        "unit": "s",
    },
    {
        "name": "rsbench",
        "directory": f"{eval_dir}/src/RSBench",
        "commands": [
            "./rsbench -m event -s small",
            "./rsbench -m event -s large -l 4250000",
            "./rsbench -m event -s large",
        ],
        "regex": r"Runtime:\s*([\d.]+)",
        "unit": "s",
    },
    {
        "name": "rsbench (fix)",
        "directory": f"{eval_dir}/src/RSBench",
        "commands": [
            "./rsbench_fix -m event -s small",
            "./rsbench_fix -m event -s large -l 4250000",
            "./rsbench_fix -m event -s large",
        ],
        "regex": r"Runtime:\s*([\d.]+)",
        "unit": "s",
    },
    {
        "name": "tealeaf",
        "directory": f"{eval_dir}/src/TeaLeaf",
        "commands": [
            "./build/omp-target-tealeaf --file Benchmarks/tea_bm_1.in",
            "./build/omp-target-tealeaf --file Benchmarks/tea_bm_2.in",
            "./build/omp-target-tealeaf --file Benchmarks/tea_bm_4.in",
        ],
        "regex": r" Wallclock:\s*([\d.]+)s",
        "unit": "s",
    },
    {
        "name": "xsbench",
        "directory": f"{eval_dir}/src/XSBench",
        "commands": [
            "./XSBench -m event -s small",
            "./XSBench -m event -g 1413",
            "./XSBench -m event -s large",
        ],
        "regex": r"Runtime:\s*([\d.]+)",
        "unit": "s",
    },
    {
        "name": "xsbench (fix)",
        "directory": f"{eval_dir}/src/XSBench",
        "commands": [
            "./XSBench_fix -m event -s small",
            "./XSBench_fix -m event -g 1413",
            "./XSBench_fix -m event -s large",
        ],
        "regex": r"Runtime:\s*([\d.]+)",
        "unit": "s",
    },
]

torture_benchmarks = [
    {
        "name": "torture",
        "directory": f"{eval_dir}/src/torture",
        "commands": [
            "./torture 127 1",
            "./torture 127 2",
            "./torture 127 4",
            "./torture 127 8",
            "./torture 127 16",
            "./torture 127 32",
            "./torture 127 64",
            "./torture 127 128",
            "./torture 127 256",
            "./torture 127 512",
            "./torture 127 1024",
            "./torture 127 2048",
            "./torture 127 4096",
            "./torture 127 8192",
            "./torture 127 16384",
            "./torture 127 32768",
            "./torture 127 65536",
            "./torture 127 131072",
            "./torture 127 262144",
            "./torture 127 524288",
            "./torture 127 1048576",
            "./torture 127 2097152",
            "./torture 127 4194304",
            "./torture 127 8388608",
            "./torture 127 16777216",
            "./torture 127 33554432",
            "./torture 127 67108864",
            "./torture 127 134217728",
            "./torture 127 268435456",
            "./torture 127 536870912",
            "./torture 127 1073741824",
        ],
        "regex": r"Elapsed time:\s*([\d.]+) seconds",
        "unit": "s",
    },
    {
        "name": "torture (fix)",
        "directory": f"{eval_dir}/src/torture",
        "commands": [
            "./torture_fix 127 1",
            "./torture_fix 127 2",
            "./torture_fix 127 4",
            "./torture_fix 127 8",
            "./torture_fix 127 16",
            "./torture_fix 127 32",
            "./torture_fix 127 64",
            "./torture_fix 127 128",
            "./torture_fix 127 256",
            "./torture_fix 127 512",
            "./torture_fix 127 1024",
            "./torture_fix 127 2048",
            "./torture_fix 127 4096",
            "./torture_fix 127 8192",
            "./torture_fix 127 16384",
            "./torture_fix 127 32768",
            "./torture_fix 127 65536",
            "./torture_fix 127 131072",
            "./torture_fix 127 262144",
            "./torture_fix 127 524288",
            "./torture_fix 127 1048576",
            "./torture_fix 127 2097152",
            "./torture_fix 127 4194304",
            "./torture_fix 127 8388608",
            "./torture_fix 127 16777216",
            "./torture_fix 127 33554432",
            "./torture_fix 127 67108864",
            "./torture_fix 127 134217728",
            "./torture_fix 127 268435456",
            "./torture_fix 127 536870912",
            "./torture_fix 127 1073741824",
        ],
        "regex": r"Elapsed time:\s*([\d.]+) seconds",
        "unit": "s",
    },
]

hashes = ["CityHash32", "CityHash64", "CityHash128", "CityHashCrc128",
          "FarmHash32", "FarmHash64", "FarmHash128",
          "MeowHash",
          "rapidhash",
          "t1ha0_ia32aes_avx", "t1ha0_ia32aes_avx2", "t1ha0_ia32aes_noavx", "t1ha0_32le", "t1ha1_le", "t1ha2_atonce",
          "XXH32", "XXH64", "XXH3_64bits", "XXH3_128bits",
]

def run_benchmark(directory, command, regexes, unit, profiler_cmd, warmup_cnt, run_cnt, confidence):
    """Run the benchmark command in a specific directory and extract execution times."""
    times = [[] for _ in range(len(regexes))]
    full_command = f"{profiler_cmd} {command}".strip()

    z_score = NormalDist().inv_cdf((1 + confidence) / 2.)

    for i in range(run_cnt + warmup_cnt):
        sys.stdout.flush()
        current_dir = os.getcwd()
        try:
            # Change to the benchmark's directory
            os.chdir(directory)

            # Run the command
            if (i < warmup_cnt):
                print(f"Running: {full_command} (Warmup {i + 1}/{warmup_cnt}) in {directory}")
            else:
                print(f"Running: {full_command} (Run {i + 1 - warmup_cnt}/{run_cnt}) in {directory}")
            sys.stdout.flush()

            start_time = time.time()
            result = subprocess.run(full_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            end_time = time.time()
            execution_time = end_time - start_time
            output = result.stdout + result.stderr

            # Change back to the original directory
            os.chdir(current_dir)

            if (i < warmup_cnt):
                continue


            conf_reached = 0

            for j, regex in enumerate(regexes):
                if (regex == r""):
                    times[j].append(float(execution_time))
                else:
                    #print(f"output: {output}")
                    # Extract the execution time using the regex, last match
                    matches = list(re.finditer(regex, output))
                    match = matches[-1] if matches else None
                    #match = re.search(regex, output)
                    #print(match.group(0))
                    #print(match.group(1))
                    #if (unit == "auto"):
                    #    print(match.group(2))
                    if match:
                        if unit == "s" or (unit == "auto" and match.group(2) == "s"):
                            times[j].append(float(match.group(1)))
                        elif unit == "ms" or (unit == "auto" and match.group(2) == "ms"):
                            times[j].append(float(match.group(1))/1000.0)
                        elif unit == "µs" or (unit == "auto" and match.group(2) == "µs"):
                            times[j].append(float(match.group(1))/1000000.0)
                        elif unit == "ns" or (unit == "auto" and match.group(2) == "ns"):
                            times[j].append(float(match.group(1))/1000000000.0)
                        else:
                            times[j].append(float(match.group(1)))
                    else:
                        print(f"Warning: No match found for output: {output}")


                if j == 0 and confidence > 0.0 and i - warmup_cnt >= 30:
                    # try at least 30 runs before applying this stuff
                    mean_time = mean(times[j])
                    std_dev = stdev(times[j])
                    standard_error = std_dev / math.sqrt(i - warmup_cnt)
                    margin_of_error = z_score * standard_error
                    tolerance = mean_time * 0.01
                    if margin_of_error < confidence:
                        print(f"Mean execution time: {mean_time:.6f} seconds")
                        print(f"95% Confidence Interval: [{mean_time - margin_of_error:.6f}, {mean_time + margin_of_error:.6f}]")
                        conf_reached = conf_reached + 1
            if conf_reached == len(regexes):
                break
        except Exception as e:
            print(f"Error running command '{full_command}' in '{directory}': {e}")
            print(f"output: {output}")
            os.chdir(current_dir)
            exit()
    sys.stdout.flush()
    return times

def build(CMAKE_BUILD_TYPE='Release',
          ENABLE_COLLISION_CHECKING='OFF',
          MEASURE_HASHING_OVERHEAD='OFF',
          HASH_FUNCTION='t1ha0_ia32aes_avx2',
          PRINT_TRANSFER_RATE='OFF'):
    current_dir = os.getcwd()
    os.chdir(build_dir)
    cmake_command = ["cmake", "..", 
                     f"-DCMAKE_BUILD_TYPE=\'{CMAKE_BUILD_TYPE}\'", 
                     f"-DENABLE_COLLISION_CHECKING={ENABLE_COLLISION_CHECKING}", 
                     f"-DMEASURE_HASHING_OVERHEAD={MEASURE_HASHING_OVERHEAD}", 
                     f"-DHASH_FUNCTION=\'{HASH_FUNCTION}\'",
                     f"-DPRINT_TRANSFER_RATE={PRINT_TRANSFER_RATE}", 
                     ]
    make_command = ["make", "-j"]

    try:
        subprocess.run(cmake_command, check=True)
        subprocess.run(make_command, check=True)
        print("CMake command executed successfully.")
    except subprocess.CalledProcessError as e:
        print(f"Error building: {e}")
        os.chdir(current_dir)
        return False

    os.chdir(current_dir)
    return True
 
def benchmark_runtime_overhead():
    warmup_runs = 10
    repetitions = 60
    confidence = 0.00
    success = build()
    if (not success):
        return
    # Collect execution times and compute averages
    results = []
    for benchmark in benchmarks:
        name = benchmark["name"]
        directory = benchmark["directory"]
        regex = [benchmark["regex"]]
        unit = benchmark["unit"]

        small_times = run_benchmark(directory, benchmark["commands"][0], regex, unit, "", warmup_runs, repetitions, confidence)
        medium_times = run_benchmark(directory, benchmark["commands"][1], regex, unit, "", warmup_runs, repetitions, confidence)
        large_times = run_benchmark(directory, benchmark["commands"][2], regex, unit, "", warmup_runs, repetitions, confidence)
    
        small_times_prof = run_benchmark(directory, benchmark["commands"][0], regex, unit, profiler_command, warmup_runs, repetitions, confidence)
        medium_times_prof = run_benchmark(directory, benchmark["commands"][1], regex, unit, profiler_command, warmup_runs, repetitions, confidence)
        large_times_prof = run_benchmark(directory, benchmark["commands"][2], regex, unit, profiler_command, warmup_runs, repetitions, confidence)
    
        #small_times_cprof = run_benchmark(directory, benchmark["commands"][0], regex, unit, comparison_prof_command, warmup_runs, repetitions, confidence)
        #medium_times_cprof = run_benchmark(directory, benchmark["commands"][1], regex, unit, comparison_prof_command, warmup_runs, repetitions, confidence)
        #large_times_cprof = run_benchmark(directory, benchmark["commands"][2], regex, unit, comparison_prof_command, warmup_runs, repetitions, confidence)

        results.append({
            "Program Name": name,
            "Small (np)": mean(small_times[0]),
            "Small (p)": mean(small_times_prof[0]),
            #"Small (nvp)": mean(small_times_cprof[0]),
            "Medium (np)": mean(medium_times[0]),
            "Medium (p)": mean(medium_times_prof[0]),
            #"Medium (nvp)": mean(medium_times_cprof[0]),
            "Large (np)": mean(large_times[0]),
            "Large (p)": mean(large_times_prof[0]),
            #"Large (nvp)": mean(large_times_cprof[0]),
        })
    
    # Print results in a table format
    print(f"RESULTS - Execution Time - (s)")
    header = (
        f"{'Program Name':<15}\t{'Small (np)':<15}\t{'Small (p)':<15}\t"
        f"{'Medium (np)':<15}\t{'Medium (p)':<15}\t"
        f"{'Large (np)':<15}\t{'Large (p)':<15}"
        #f"{'Program Name':<15}\t{'Small (np)':<15}\t{'Small (p)':<15}\t{'Small (nvp)':<15}\t"
        #f"{'Medium (np)':<15}\t{'Medium (p)':<15}\t{'Medium (nvp)':<15}\t"
        #f"{'Large (np)':<15}\t{'Large (p)':<15}\t{'Large (nvp)':<15}"
    )
    separator = "-" * len(header)
    print(header)
    #print(separator)
    
    for result in results:
        row = (
            f"{result['Program Name']:<15}\t"
            f"{result['Small (np)']:<15.6f}\t{result['Small (p)']:<15.6f}\t"
            f"{result['Medium (np)']:<15.6f}\t{result['Medium (p)']:<15.6f}\t"
            f"{result['Large (np)']:<15.6f}\t{result['Large (p)']:<15.6f}"
            #f"{result['Small (np)']:<15.6f}\t{result['Small (p)']:<15.6f}\t{result['Small (nvp)']:<15.6f}\t"
            #f"{result['Medium (np)']:<15.6f}\t{result['Medium (p)']:<15.6f}\t{result['Medium (nvp)']:<15.6f}\t"
            #f"{result['Large (np)']:<15.6f}\t{result['Large (p)']:<15.6f}\t{result['Large (nvp)']:<15.6f}"
        )
        print(row)


def benchmark_runtime_overhead_torture():
    warmup_runs = 10
    repetitions = 30
    confidence = 0.00
    success = build()
    if (not success):
        return
    # Collect execution times and compute averages
    results_prof = defaultdict(lambda: defaultdict(lambda: None))
    results_noprof = defaultdict(lambda: defaultdict(lambda: None))
    for benchmark in torture_benchmarks:
        name = benchmark["name"]
        #if "(fix)" in name:
        #    continue
        directory = benchmark["directory"]
        regex = [benchmark["regex"]]
        unit = benchmark["unit"]
    
        for command in benchmark["commands"]: 
            numbers = re.findall(r'\d+', command)
            size = numbers[-1] if numbers else 0
            times_prof = run_benchmark(directory, command, regex, unit, profiler_command, warmup_runs, repetitions, confidence)
            times_noprof = run_benchmark(directory, command, regex, unit, "", warmup_runs, repetitions, confidence)
            results_noprof[size][name] = mean(times_noprof[0])
            results_prof[size][name] = mean(times_prof[0])
            print(f"  result np ({size})  : {results_noprof[size][name]:<20.6f}")
            print(f"  result p ({size})  : {results_prof[size][name]:<20.6f}")
    
    for benchmark in torture_benchmarks:
        name = benchmark["name"]
        #if "(fix)" in name:
        #    continue
        print(f"RESULTS - Execution Time - ({name}) - (s)")
        # Print results in a table format
        header = f"{'Input Size':<15}\t{'No Prof':<15}\t{'Prof':<15}"
        separator = "-" * len(header)
        print(header)
        #print(separator)
        
        for command in benchmark["commands"]:
            numbers = re.findall(r'\d+', command)
            size = numbers[-1] if numbers else 0
            row = f"{size:<15}\t"
            row += f"{results_noprof[size][name]:<15.6f}\t{results_prof[size][name]:<15.6f}"
            print(row)
        print()


def benchmark_hash_overhead():
    # this takes a long time. 19 hash functions * 8 benchmarks * 3 problem sizes * 11 runs
    warmup_runs = 3
    repetitions = 30
    confidence = 0.95
    # Collect execution times and compute averages
    results = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: None)))
    for hash_fn in hashes:
        success = build(MEASURE_HASHING_OVERHEAD='ON',
                        HASH_FUNCTION=hash_fn)
        if (not success):
            continue

        for benchmark in benchmarks:
            name = benchmark["name"]
            if "(fix)" in name:
                continue
            directory = benchmark["directory"]
            regex = [r"avg hash rate\s*([\d.]+)(GB/s)"]
            unit = "GB/s"
    
            small_times_prof = run_benchmark(directory, benchmark["commands"][0], regex, unit, profiler_command, warmup_runs, repetitions, confidence)
            medium_times_prof = run_benchmark(directory, benchmark["commands"][1], regex, unit, profiler_command, warmup_runs, repetitions, confidence)
            large_times_prof = run_benchmark(directory, benchmark["commands"][2], regex, unit, profiler_command, warmup_runs, repetitions, confidence)
        
            results["small"][hash_fn][name] = mean(small_times_prof[0])
            results["medium"][hash_fn][name] = mean(medium_times_prof[0])
            results["large"][hash_fn][name] = mean(large_times_prof[0])

            print(f"  result (small)  : {results['small'][hash_fn][name]:<20.3f}")
            print(f"  result (medium) : {results['medium'][hash_fn][name]:<20.3f}")
            print(f"  result (large)  : {results['large'][hash_fn][name]:<20.3f}")
    
    for size in ["small", "medium", "large"]:
        print(f"RESULTS - Hash Overhead - ({size}) - GB/s")
        # Print results in a table format
        header = f"{'Program Name':<15}\t"
        for hash_fn in hashes:
            header += f"{hash_fn:<20}\t"
        separator = "-" * len(header)
        print(header)
        #print(separator)
        
        for benchmark in benchmarks:
            name = benchmark["name"]
            if "(fix)" in name:
                continue
            row = f"{name:<15}\t"
            for hash_fn in hashes:
                row += f"{results[size][hash_fn][name]:<20.3f}\t"
            print(row)
        print()


def benchmark_hash_overhead_torture():
    warmup_runs = 1
    repetitions = 3
    confidence = 0.95

    # Collect execution times and compute averages
    results = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: None)))
    for hash_fn in hashes:
        success = build(MEASURE_HASHING_OVERHEAD='ON',
                        HASH_FUNCTION=hash_fn)
        if (not success):
            continue

        for benchmark in torture_benchmarks:
            name = benchmark["name"]
            if "(fix)" in name:
                continue
            directory = benchmark["directory"]
            regex = [r"avg hash rate\s*([\d.]+)(GB/s)"]
            unit = "GB/s"
    
            for command in benchmark["commands"]: 
                numbers = re.findall(r'\d+', command)
                size = numbers[-1] if numbers else 0
                times_prof = run_benchmark(directory, command, regex, unit, profiler_command, warmup_runs, repetitions, confidence)
                results[size][hash_fn][name] = mean(times_prof[0])
                print(f"  result ({size})  : {results[size][hash_fn][name]:<20.3f}")
    
    for benchmark in torture_benchmarks:
        name = benchmark["name"]
        if "(fix)" in name:
            continue
        print(f"RESULTS - Hash Overhead - ({name}) - GB/s")
        # Print results in a table format
        header = f"{'Input Size':<15}\t"
        for hash_fn in hashes:
            header += f"{hash_fn:<20}\t"
        separator = "-" * len(header)
        print(header)
        #print(separator)
        
        for command in benchmark["commands"]:
            numbers = re.findall(r'\d+', command)
            size = numbers[-1] if numbers else 0
            row = f"{size:<15}\t"
            for hash_fn in hashes:
                row += f"{results[size][hash_fn][name]:<20.3f}\t"
            print(row)
        print()
    
def benchmark_hash_collisions():
    warmup_runs = 0
    repetitions = 1
    confidence = 0.00
    # Collect execution times and compute averages
    results = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: None)))
    for hash_fn in hashes:
        success = build(ENABLE_COLLISION_CHECKING='ON',
                        HASH_FUNCTION=hash_fn)
        if (not success):
            continue

        for benchmark in benchmarks:
            name = benchmark["name"]
            if "(fix)" in name:
                continue
            directory = benchmark["directory"]
            #regex = r"collision rate of\s+([\d.]+)%"
            regex = [r"Found\s+([\d.]+) collisions"]
            unit = ""
    
            small_times_prof = run_benchmark(directory, benchmark["commands"][0], regex, unit, profiler_command, warmup_runs, repetitions, confidence)
            medium_times_prof = run_benchmark(directory, benchmark["commands"][1], regex, unit, profiler_command, warmup_runs, repetitions, confidence)
            large_times_prof = run_benchmark(directory, benchmark["commands"][2], regex, unit, profiler_command, warmup_runs, repetitions, confidence)
        
            results["small"][hash_fn][name] = mean(small_times_prof[0])
            results["medium"][hash_fn][name] = mean(medium_times_prof[0])
            results["large"][hash_fn][name] = mean(large_times_prof[0])

            print(f"  result (small)  : {results['small'][hash_fn][name]:<20.2f}")
            print(f"  result (medium) : {results['medium'][hash_fn][name]:<20.2f}")
            print(f"  result (large)  : {results['large'][hash_fn][name]:<20.2f}")
    
    for size in ["small", "medium", "large"]:
        print(f"RESULTS - COLLISION RATES - ({size})")
        # Print results in a table format
        header = f"{'Program Name':<15}\t"
        for hash_fn in hashes:
            header += f"{hash_fn:<20}\t"
        separator = "-" * len(header)
        print(header)
        #print(separator)
        
        for benchmark in benchmarks:
            name = benchmark["name"]
            if "(fix)" in name:
                continue
            row = f"{name:<15}\t"
            for hash_fn in hashes:
                row += f"{results[size][hash_fn][name]:<20.2f}\t"
            print(row)
        print()

def benchmark_prediction_accuracy():
    warmup_runs = 3
    repetitions = 30
    confidence = 0.95
    success = build()
    if (not success):
        return
    # Collect execution times and compute averages
    results = []
    regex = ["    time             \s+([\d.]+)(s|ms|µs|ns)"]
    for i, b in enumerate(benchmarks):
        name = benchmarks[i]["name"]
        if "(fix)" not in name:
            continue
        # assume non-fix version directly preceeds fix version
        directory = benchmarks[i]["directory"]
        fix_directory = benchmarks[i-1]["directory"]


        small_times = run_benchmark(directory, benchmarks[i-1]["commands"][0], [""], "s", "", warmup_runs, repetitions, confidence)
        medium_times = run_benchmark(directory, benchmarks[i-1]["commands"][1], [""], "s", "", warmup_runs, repetitions, confidence)
        large_times = run_benchmark(directory, benchmarks[i-1]["commands"][2], [""], "s", "", warmup_runs, repetitions, confidence)

        small_times_fix = run_benchmark(fix_directory, benchmarks[i]["commands"][0], [""], "s", "", warmup_runs, repetitions, confidence)
        medium_times_fix = run_benchmark(fix_directory, benchmarks[i]["commands"][1], [""], "s", "", warmup_runs, repetitions, confidence)
        large_times_fix = run_benchmark(fix_directory, benchmarks[i]["commands"][2], [""], "s", "", warmup_runs, repetitions, confidence)
    
        small_times_prof = run_benchmark(directory, benchmarks[i-1]["commands"][0], regex, "auto", profiler_command, warmup_runs, repetitions, confidence)
        medium_times_prof = run_benchmark(directory, benchmarks[i-1]["commands"][1], regex, "auto", profiler_command, warmup_runs, repetitions, confidence)
        large_times_prof = run_benchmark(directory, benchmarks[i-1]["commands"][2], regex, "auto", profiler_command, warmup_runs, repetitions, confidence)
    
        small_actual = mean(small_times_fix[0])
        medium_actual = mean(medium_times_fix[0])
        large_actual = mean(large_times_fix[0])
        small_predicted = mean(small_times[0]) - mean(small_times_prof[0])
        medium_predicted = mean(medium_times[0]) - mean(medium_times_prof[0])
        large_predicted = mean(large_times[0]) - mean(large_times_prof[0])
        small_acc = 100.0 * (1.0 - (abs(small_predicted - small_actual) / small_actual))
        medium_acc = 100.0 * (1.0 - (abs(medium_predicted - medium_actual) / medium_actual))
        large_acc = 100.0 * (1.0 - (abs(large_predicted - large_actual) / large_actual))
        results.append({
            "Program Name": name,
            "Small": small_acc,
            "Medium": medium_acc,
            "Large": large_acc,
        })
    
    # Print results in a table format
    print(f"RESULTS - Execution Time Speedup Prediction Accuracy - (%)")
    header = (
        f"{'Program Name':<15}\t{'Small':<15}\t"
        f"{'Medium':<15}\t"
        f"{'Large':<15}"
    )
    separator = "-" * len(header)
    print(header)
    #print(separator)
    
    for result in results:
        row = (
            f"{result['Program Name']:<15}\t"
            f"{result['Small']:<15.6f}\t"
            f"{result['Medium']:<15.6f}\t"
            f"{result['Large']:<15.6f}"
        )
        print(row)

def benchmark_identify_issues():
    warmup_runs = 0
    repetitions = 1
    confidence = 0.00
    success = build()
    if (not success):
        return
    # Collect execution times and compute averages
    results = []
    regex = ["([\d.]+) potential duplicate data transfer\(s\)",
             "([\d.]+) potential round trip data transfer\(s\)",
             "([\d.]+) potential repeated device memory allocation\(s\)"]
    for benchmark in benchmarks:
        name = benchmark["name"]
        # assume non-fix version directly preceeds fix version
        directory = benchmark["directory"]

        small_times_prof = run_benchmark(directory, benchmark["commands"][0], regex, "", profiler_command, warmup_runs, repetitions, confidence)
        medium_times_prof = run_benchmark(directory, benchmark["commands"][1], regex, "", profiler_command, warmup_runs, repetitions, confidence)
        large_times_prof = run_benchmark(directory, benchmark["commands"][2], regex, "", profiler_command, warmup_runs, repetitions, confidence)
    
        results.append({
            "Program Name": name,
            "Small (dd)": int(mean(small_times_prof[0])),
            "Small (rt)": int(mean(small_times_prof[1])),
            "Small (ad)": int(mean(small_times_prof[2])),
            "Medium (dd)": int(mean(medium_times_prof[0])),
            "Medium (rt)": int(mean(medium_times_prof[1])),
            "Medium (ad)": int(mean(medium_times_prof[2])),
            "Large (dd)": int(mean(large_times_prof[0])),
            "Large (rt)": int(mean(large_times_prof[1])),
            "Large (ad)": int(mean(large_times_prof[2])),
        })
    
    # Print results in a table format
    print(f"RESULTS - Execution Time Speedup Prediction Accuracy - (%)")
    header = (
        f"{'Program Name':<15}\t"
        f"{'Small (dd)':<15}\t"
        f"{'Small (rt)':<15}\t"
        f"{'Small (ad)':<15}\t"
        f"{'Medium (dd)':<15}\t"
        f"{'Medium (rt)':<15}\t"
        f"{'Medium (ad)':<15}\t"
        f"{'Large (dd)':<15}\t"
        f"{'Large (rt)':<15}\t"
        f"{'Large (ad)':<15}"
    )
    separator = "-" * len(header)
    print(header)
    #print(separator)
    
    for result in results:
        row = (
            f"{result['Program Name']:<15}\t"
            f"{result['Small (dd)']:<15d}\t"
            f"{result['Small (rt)']:<15d}\t"
            f"{result['Small (ad)']:<15d}\t"
            f"{result['Medium (dd)']:<15d}\t"
            f"{result['Medium (rt)']:<15d}\t"
            f"{result['Medium (ad)']:<15d}\t"
            f"{result['Large (dd)']:<15d}\t"
            f"{result['Large (rt)']:<15d}\t"
            f"{result['Large (ad)']:<15d}"
        )
        print(row)

def benchmark_transfer_rate_torture():
    warmup_runs = 3
    repetitions = 10
    confidence = 0.00

    # Collect execution times and compute averages
    results = defaultdict(lambda: defaultdict(lambda: None))
    success = build(PRINT_TRANSFER_RATE='ON')
    for benchmark in torture_benchmarks:
        name = benchmark["name"]
        if "(fix)" in name:
            continue
        directory = benchmark["directory"]
        regex = [r"avg transfer rate\s*([\d.]+)(GB/s)"]
        unit = "GB/s"
    
        for command in benchmark["commands"]: 
            numbers = re.findall(r'\d+', command)
            size = numbers[-1] if numbers else 0
            times_prof = run_benchmark(directory, command, regex, unit, profiler_command, warmup_runs, repetitions, confidence)
            results[size][name] = mean(times_prof[0])
            print(f"  result ({size})  : {results[size][name]:<20.3f}")
    
    for benchmark in torture_benchmarks:
        name = benchmark["name"]
        if "(fix)" in name:
            continue
        print(f"RESULTS - Transfer Overhead - ({name}) - GB/s")
        # Print results in a table format
        header = f"{'Input Size':<15}\t"
        header += f"{'Transfer Rate':<20}\t"
        separator = "-" * len(header)
        print(header)
        #print(separator)
        
        for command in benchmark["commands"]:
            numbers = re.findall(r'\d+', command)
            size = numbers[-1] if numbers else 0
            row = f"{size:<15}\t"
            row += f"{results[size][name]:<20.3f}\t"
            print(row)
        print()


def benchmark_runtime_overhead_full():
    warmup_runs = 10
    repetitions = 30
    confidence = 0.00
    success = build()
    if (not success):
        return
    # Collect execution times and compute averages
    results = []
    for benchmark in benchmarks:
        name = benchmark["name"]
        directory = benchmark["directory"]
        regex = [""]
        unit = "s"

        small_times = run_benchmark(directory, benchmark["commands"][0], regex, unit, "", warmup_runs, repetitions, confidence)
        medium_times = run_benchmark(directory, benchmark["commands"][1], regex, unit, "", warmup_runs, repetitions, confidence)
        large_times = run_benchmark(directory, benchmark["commands"][2], regex, unit, "", warmup_runs, repetitions, confidence)
    
        small_times_prof = run_benchmark(directory, benchmark["commands"][0], regex, unit, profiler_command, warmup_runs, repetitions, confidence)
        medium_times_prof = run_benchmark(directory, benchmark["commands"][1], regex, unit, profiler_command, warmup_runs, repetitions, confidence)
        large_times_prof = run_benchmark(directory, benchmark["commands"][2], regex, unit, profiler_command, warmup_runs, repetitions, confidence)
    
        #small_times_cprof = run_benchmark(directory, benchmark["commands"][0], regex, unit, comparison_prof_command, warmup_runs, repetitions, confidence)
        #medium_times_cprof = run_benchmark(directory, benchmark["commands"][1], regex, unit, comparison_prof_command, warmup_runs, repetitions, confidence)
        #large_times_cprof = run_benchmark(directory, benchmark["commands"][2], regex, unit, comparison_prof_command, warmup_runs, repetitions, confidence)

        results.append({
            "Program Name": name,
            "Small (np)": mean(small_times[0]),
            "Small (p)": mean(small_times_prof[0]),
            #"Small (nvp)": mean(small_times_cprof[0]),
            "Medium (np)": mean(medium_times[0]),
            "Medium (p)": mean(medium_times_prof[0]),
            #"Medium (nvp)": mean(medium_times_cprof[0]),
            "Large (np)": mean(large_times[0]),
            "Large (p)": mean(large_times_prof[0]),
            #"Large (nvp)": mean(large_times_cprof[0]),
        })
    
    # Print results in a table format
    print(f"RESULTS - Execution Time - (s)")
    header = (
        f"{'Program Name':<15}\t{'Small (np)':<15}\t{'Small (p)':<15}\t"
        f"{'Medium (np)':<15}\t{'Medium (p)':<15}\t"
        f"{'Large (np)':<15}\t{'Large (p)':<15}"
        #f"{'Program Name':<15}\t{'Small (np)':<15}\t{'Small (p)':<15}\t{'Small (nvp)':<15}\t"
        #f"{'Medium (np)':<15}\t{'Medium (p)':<15}\t{'Medium (nvp)':<15}\t"
        #f"{'Large (np)':<15}\t{'Large (p)':<15}\t{'Large (nvp)':<15}"
    )
    separator = "-" * len(header)
    print(header)
    #print(separator)
    
    for result in results:
        row = (
            f"{result['Program Name']:<15}\t"
            f"{result['Small (np)']:<15.6f}\t{result['Small (p)']:<15.6f}\t"
            f"{result['Medium (np)']:<15.6f}\t{result['Medium (p)']:<15.6f}\t"
            f"{result['Large (np)']:<15.6f}\t{result['Large (p)']:<15.6f}"
            #f"{result['Small (np)']:<15.6f}\t{result['Small (p)']:<15.6f}\t{result['Small (nvp)']:<15.6f}\t"
            #f"{result['Medium (np)']:<15.6f}\t{result['Medium (p)']:<15.6f}\t{result['Medium (nvp)']:<15.6f}\t"
            #f"{result['Large (np)']:<15.6f}\t{result['Large (p)']:<15.6f}\t{result['Large (nvp)']:<15.6f}"
        )
        print(row)

#benchmark_runtime_overhead()
#benchmark_runtime_overhead_full()
#benchmark_prediction_accuracy()
#benchmark_identify_issues()
#benchmark_hash_overhead()
benchmark_hash_overhead_torture()
#benchmark_runtime_overhead_torture()
#benchmark_transfer_rate_torture()
#benchmark_hash_collisions()

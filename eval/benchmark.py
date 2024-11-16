import subprocess
import re
import os
from statistics import mean
from collections import defaultdict

eval_dir = os.getcwd()
build_dir = f"{eval_dir}/../build"
profiler_command = f"{build_dir}/ompdataprof"
warmup_runs = 3
repetitions = 30

benchmarks = [
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
        "name": "cfd",
        "directory": f"{eval_dir}/src/cfd",
        "commands": [
            "./euler3d_cpu_offload ../../data/cfd/fvcorr.domn.097K",
            "./euler3d_cpu_offload ../../data/cfd/fvcorr.domn.193K",
            "./euler3d_cpu_offload ../../data/cfd/missile.domn.0.2M",
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

hashes = ["CityHash32", "CityHash64", "CityHash128", "CityHashCrc128",
          "FarmHash32", "FarmHash64", "FarmHash128",
          "MeowHash",
          "rapidhash",
          "t1ha0_ia32aes_avx", "t1ha0_ia32aes_avx2", "t1ha0_ia32aes_noavx", "t1ha0_32le", "t1ha1_le", "t1ha2_atonce",
          "XXH32", "XXH64", "XXH3_64bits", "XXH3_128bits",
          ]

def run_benchmark(directory, command, regex, unit, profiler=False):
    """Run the benchmark command in a specific directory and extract execution times."""
    times = []
    profiler_prefix = profiler_command if profiler else ""
    full_command = f"{profiler_prefix} {command}".strip()

    for i in range(repetitions + warmup_runs):
        current_dir = os.getcwd()
        try:
            # Change to the benchmark's directory
            os.chdir(directory)

            # Run the command
            if (i < warmup_runs):
                print(f"Running: {full_command} (Warmup {i + 1}/{warmup_runs}) in {directory}")
            else:
                print(f"Running: {full_command} (Run {i + 1 - warmup_runs}/{repetitions}) in {directory}")

            result = subprocess.run(full_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            output = result.stdout + result.stderr

            # Change back to the original directory
            os.chdir(current_dir)

            if (i < warmup_runs):
                continue

            #print(f"output: {output}")
            # Extract the execution time using the regex
            match = re.search(regex, output)
            if match:
                if unit == "s" or (unit == "auto" and match.group(2) == "s"):
                    times.append(float(match.group(1)))
                elif unit == "ms" or (unit == "auto" and match.group(2) == "ms"):
                    times.append(float(match.group(1))/1000.0)
                elif unit == "µs" or (unit == "auto" and match.group(2) == "µs"):
                    times.append(float(match.group(1))/1000000.0)
                elif unit == "ns" or (unit == "auto" and match.group(2) == "ns"):
                    times.append(float(match.group(1))/1000000000.0)
                else:
                    times.append(float(match.group(1)))
            else:
                print(f"Warning: No match found for output: {output}")
        except Exception as e:
            print(f"Error running command '{full_command}' in '{directory}': {e}")
            # Ensure we return to the original directory in case of error
            os.chdir(current_dir)
    return times

def benchmark_runtime_overhead():
    # Collect execution times and compute averages
    results = []
    for benchmark in benchmarks:
        name = benchmark["name"]
        directory = benchmark["directory"]
        regex = benchmark["regex"]
        unit = benchmark["unit"]

        small_times = run_benchmark(directory, benchmark["commands"][0], regex, unit, profiler=False)
        medium_times = run_benchmark(directory, benchmark["commands"][1], regex, unit, profiler=False)
        large_times = run_benchmark(directory, benchmark["commands"][2], regex, unit, profiler=False)
    
        small_times_prof = run_benchmark(directory, benchmark["commands"][0], regex, unit, profiler=True)
        medium_times_prof = run_benchmark(directory, benchmark["commands"][1], regex, unit, profiler=True)
        large_times_prof = run_benchmark(directory, benchmark["commands"][2], regex, unit, profiler=True)
    
        results.append({
            "Program Name": name,
            "Small (np)": mean(small_times) if small_times else float('nan'),
            "Small (p)": mean(small_times_prof) if small_times_prof else float('nan'),
            "Medium (np)": mean(medium_times) if medium_times else float('nan'),
            "Medium (p)": mean(medium_times_prof) if medium_times_prof else float('nan'),
            "Large (np)": mean(large_times) if large_times else float('nan'),
            "Large (p)": mean(large_times_prof) if large_times_prof else float('nan'),
        })
    
    # Print results in a table format
    header = (
        f"{'Program Name':<15} {'Small (np)':<15} {'Small (p)':<15} "
        f"{'Medium (np)':<15} {'Medium (p)':<15} "
        f"{'Large (np)':<15} {'Large (p)':<15}"
    )
    separator = "-" * len(header)
    print(header)
    print(separator)
    
    for result in results:
        row = (
            f"{result['Program Name']:<15} {result['Small (np)']:<15.6f} {result['Small (p)']:<15.6f} "
            f"{result['Medium (np)']:<15.6f} {result['Medium (p)']:<15.6f} "
            f"{result['Large (np)']:<15.6f} {result['Large (p)']:<15.6f}"
        )
        print(row)


def benchmark_hash_overhead():
    # Collect execution times and compute averages
    results = defaultdict(lambda: defaultdict(lambda: defaultdict(lambda: None)))
    for hash_fn in hashes:
        current_dir = os.getcwd()
        os.chdir(build_dir)
        cmake_command = ["cmake", "..", 
                         "-DCMAKE_BUILD_TYPE=\'Release\'", 
                         "-DENABLE_COLLISION_CHECKING=OFF", 
                         "-DMEASURE_HASHING_OVERHEAD=ON", 
                         f"-DHASH_FUNCTION=\'{hash_fn}\'"
                         ]
        make_command = ["make", "-j"]

        try:
            subprocess.run(cmake_command, check=True)
            subprocess.run(make_command, check=True)
            print("CMake command executed successfully.")
        except subprocess.CalledProcessError as e:
            print(f"Error building: {e}")
            os.chdir(current_dir)
            continue

        os.chdir(current_dir)

        for benchmark in benchmarks:
            name = benchmark["name"]
            if "(fix)" in name:
                continue
            directory = benchmark["directory"]
            regex = r"avg hash rate\s*([\d.]+)(GB/s)"
            unit = "GB/s"
    
            small_times_prof = run_benchmark(directory, benchmark["commands"][0], regex, unit, profiler=True)
            medium_times_prof = run_benchmark(directory, benchmark["commands"][1], regex, unit, profiler=True)
            large_times_prof = run_benchmark(directory, benchmark["commands"][2], regex, unit, profiler=True)
        
            results["small"][hash_fn][name] = mean(small_times_prof) if small_times_prof else float('nan')
            results["medium"][hash_fn][name] = mean(medium_times_prof) if medium_times_prof else float('nan')
            results["large"][hash_fn][name] = mean(large_times_prof) if large_times_prof else float('nan')
    
    for size in ["small", "medium", "large"]:
        print(f"RESULTS ({size})")
        # Print results in a table format
        header = f"{'Program Name':<15} "
        for hash_fn in hashes:
            header += f"{hash_fn:<20} "
        separator = "-" * len(header)
        print(header)
        print(separator)
        
        for benchmark in benchmarks:
            name = benchmark["name"]
            if "(fix)" in name:
                continue
            row = f"{name:<15} "
            for hash_fn in hashes:
                row += f"{results[size][hash_fn][name]:<20.3f} "
            print(row)
        print()
    
#benchmark_runtime_overhead()
benchmark_hash_overhead()

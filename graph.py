import subprocess
import csv
import re
from concurrent.futures import ThreadPoolExecutor, as_completed

def parse_input(input_string):
    pattern = r'\d+'
    match = re.search(pattern, input_string)
    if match:
        return int(match.group())
    else:
        return None

def run_program(parameters):
    cmd = ['./build/MyImageApplication'] + parameters + ['n']
    result = subprocess.run(cmd, stdout=subprocess.PIPE, text=True)
    return parse_input(result.stdout)

def run_program_wrapper(args):
    mode, b = args
    output = run_program(['Lena_512_512.rgb', mode, str(b**3)])
    return {'Buckets': b, 'Error': output}  # Removed 'Mode'

# Still using ThreadPoolExecutor to run tasks in parallel
results_mode_1 = []
results_mode_2 = []
with ThreadPoolExecutor(max_workers=4) as executor:
    # Separate argument preparation for each mode
    all_args_mode_1 = [('1', b) for b in range(2, 257)]
    all_args_mode_2 = [('2', b) for b in range(2, 257)]
    
    # Schedule the execution for Mode 1
    futures_mode_1 = {executor.submit(run_program_wrapper, args): args for args in all_args_mode_1}
    for future in as_completed(futures_mode_1):
        res = future.result()
        results_mode_1.append(res)
    
    # Schedule the execution for Mode 2
    futures_mode_2 = {executor.submit(run_program_wrapper, args): args for args in all_args_mode_2}
    for future in as_completed(futures_mode_2):
        res = future.result()
        results_mode_2.append(res)


with open('results_mode_1.csv', 'w', newline='') as csvfile:
    fieldnames = ['Buckets', 'Error']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    writer.writeheader()
    for row in results_mode_1:  # Directly use results_mode_1
        writer.writerow({'Buckets': row['Buckets'], 'Error': row['Error']})

# Write results for Mode 2 to its own CSV
with open('results_mode_2.csv', 'w', newline='') as csvfile:
    fieldnames = ['Buckets', 'Error']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    writer.writeheader()
    for row in results_mode_2:  # Directly use results_mode_2
        writer.writerow({'Buckets': row['Buckets'], 'Error': row['Error']})

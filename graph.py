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
    return {'B': b, 'Error': output, 'Mode': mode}

# Use ThreadPoolExecutor to run tasks in parallel
results = []
with ThreadPoolExecutor(max_workers=4) as executor:  # Adjust max_workers based on your machine
    # Prepare arguments for both modes for each 'b' value
    all_args = [(mode, b) for b in range(2, 257) for mode in ['1', '2']]
    # Schedule the execution of run_program for each set of parameters
    future_to_params = {executor.submit(run_program_wrapper, args): args for args in all_args}
    
    for future in as_completed(future_to_params):
        res = future.result()
        results.append(res)

# Separate results based on mode
err_mode_1 = [r for r in results if r['Mode'] == '1']
err_mode_2 = [r for r in results if r['Mode'] == '2']

# Write results to CSV
with open('results.csv', 'w', newline='') as csvfile:
    fieldnames = ['B', 'Error', 'Mode']
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    writer.writeheader()
    for row in results:  # Directly write combined results
        writer.writerow(row)


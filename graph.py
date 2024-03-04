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
    output = run_program(['./dataset/test5.rgb', mode, str(b**3)])
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

import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

df_mode_1 = pd.read_csv('results_mode_1.csv')
df_mode_2 = pd.read_csv('results_mode_2.csv')

df_mode_1 = df_mode_1.sort_values(by='Buckets')
df_mode_2 = df_mode_2.sort_values(by='Buckets')

# Assuming your CSV has columns named 'Buckets' and 'Error'
plt.figure(figsize=(13, 7))

plt.plot(df_mode_1['Buckets'], df_mode_1['Error'], label='Mode 1', color='purple')
plt.plot(df_mode_2['Buckets'], df_mode_2['Error'], label='Mode 2', color='red')

# Add a legend
plt.legend()

# Add labels and title
plt.xlabel('Number of Buckets Per Channel')
plt.ylabel('Absolute Sum Error')
plt.title(f'Absolute Sum Error with Respect to Number of Buckets')

# Set y-axis to plain style (disable scientific notation)
plt.gca().yaxis.set_major_formatter(ticker.StrMethodFormatter('{x:,.0f}'))

plt.grid(True, which='both', linestyle='--', linewidth=0.5)

# Optionally, you can rotate the y-axis labels for better readability
plt.yticks(rotation=45)

# plt.savefig('./plot.png') Lena
# plt.savefig('./plot1.png') # Test1
# plt.savefig('./plot2.png') # Test2
# plt.savefig('./plot3.png') # Test3
# plt.savefig('./plot4.png') # Test4
plt.savefig('./plot5.png') # Test5
# Close the plot to avoid memory issues
plt.close()
#ReadWrite.py
#### Read Write Protocol Python ###
#Calls ReadWrite.c to initialize the FPGA, and begin streaming FPGA results to the console; then re-consumes those results and passes them to python


import sys
import subprocess



def open_c_program():
    command = ["./your_c_program"]

    try:
        process = subprocess.Popen(command, stdout=subprocess.PIPE, bufsize=1, universal_newlines=True)
        return process

    except Exception as e:
        print(f"Error executing C program: {str(e)}")
        return None

def stream_c_program_output(process):
    if process is None:
        return

    for line in process.stdout:
        line = line.strip()

        # Process the line as needed
        # Example: Print the line to the console
        print(line)

    process.wait()  # Wait for the process to complete

# Open the C program
c_program_process = open_c_program()
    

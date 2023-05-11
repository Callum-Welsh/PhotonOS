#ReadWrite.py
#### Read Write Protocol Python ###
#Calls ReadWrite.c to initialize the FPGA, and begin streaming FPGA results to the console; then re-consumes those results and passes them to python


import sys
import subprocess
import asyncio
from PyQt5.QtWidgets import QApplication, QMainWindow, QTextEdit
from PyQt5.QtCore import Qt, QThread, pyqtSignal



'''
# Multi-Threaded Implementation of Non-Blocking Data Streaming. Too scary for me!
class DataStreamingThread(QThread):
    dataReceived = pyqtSignal(str)

    def run(self):
        cmd = "./c_script"  # Replace with the actual C script command
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE)

        while True:
            data = proc.stdout.readline().decode().strip()
            if not data:
                break

            self.dataReceived.emit(data)

        proc.wait()
'''
######## ASYNCHRONOUS FUNCTION FOR READING DATA FROM FPGA VIA C CODE ##################
async def read_data(proc, text_edit):
    while True:
        data = await proc.stdout.readline()
        if not data:
            break

        # Process the received data
        text = data.decode().strip()
        text_edit.append(f"Received data: {text}")

        # Perform other tasks concurrently
        # ...
########################################################################################

##################### QT5 GUI GRAPHICS AND FUNCTIONALITY #################################################
class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()

        self.text_edit = QTextEdit(self)
        self.setCentralWidget(self.text_edit)


    # Define the async call to open the c program on launch
    async def start_count_data_streaming(self):
        cmd = "./c_script"  # Replace with the actual C script command
        proc = await asyncio.create_subprocess_shell(cmd, stdout=subprocess.PIPE) #Opens the C program

        await read_data(proc, self.text_edit)

        await proc.wait()

################################################################################################


####### ACTUALLY RUN THE PROGRAM ###################
if __name__ == "__main__":
    app = QApplication(sys.argv)

    window = MainWindow() #LAUNCH THE GUI

    # Create an asyncio event loop
    loop = asyncio.get_event_loop()
    loop.create_task(window.start_count_data_streaming())

    window.show() #SHOW THE GUI

    # Enter the asyncio event loop
    loop.run_forever()
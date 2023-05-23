import sys
import pyqtgraph as pg
from PyQt5.QtWidgets import QMainWindow, QApplication
from PyQt5.QtCore import QTimer
from collections import deque

# Create empty lists to store the data
x_data = deque(maxlen=100)  # Maximum number of data points to display
y_data = deque(maxlen=100)  # Maximum number of data points to display

class RealTimePlot(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle('Real-time Data Stream')
        self.setGeometry(100, 100, 800, 600)

        self.canvas = pg.GraphicsLayoutWidget()
        self.setCentralWidget(self.canvas)

        self.plot = self.canvas.addPlot(title='Real-time Data Stream')
        self.plot.setLabel('left', 'Value')
        self.plot.setLabel('bottom', 'Time')

        self.curve = self.plot.plot(pen='b')

        self.show()

    def update_plot(self):
        # Update the data
        try:
            # Read a line of input from the console
            line = input()

            # Split the line into individual numbers
            numbers = line.split(',')

            # Convert the numbers to floats
            data = [float(num) for num in numbers]

            # Update the data lists
            x_data.append(data[0])  # Assuming the time value is at index 0
            y_data.append(data[1])  # Assuming the data value is at index 1

            # Update the plot
            self.curve.setData(list(x_data), list(y_data))

        except KeyboardInterrupt:
            # Exit the program when Ctrl+C is pressed
            QApplication.quit()

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = RealTimePlot()

    timer = QTimer()
    timer.timeout.connect(window.update_plot)
    timer.start(1)  # Update interval in milliseconds

    sys.exit(app.exec_())

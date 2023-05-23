import sys
import pyqtgraph as pg
from PyQt5.QtWidgets import QMainWindow, QApplication
from PyQt5.QtCore import QTimer
from collections import deque

# Create empty lists to store the data
x_data_counts1 = deque(maxlen=1000)  # Maximum number of data points to display
y_data_counts1 = deque(maxlen=1000)  # Maximum number of data points to display # Maximum number of data points to display
y_data_counts2 = deque(maxlen=1000)  # Maximum number of data points to display
y_data_coin = deque(maxlen=1000)  # Maximum number of data points to display # Maximum number of data points to display

pg.setConfigOptions(antialias=True)

class RealTimePlot(QMainWindow):
    def __init__(self):
        super().__init__()

        self.setWindowTitle('Real-time Data Stream')
        self.setGeometry(100, 100, 800, 600)

        self.canvas = pg.GraphicsLayoutWidget()
        self.setCentralWidget(self.canvas)

        self.counts1Plot = self.canvas.addPlot(title='Detector 1 Counts')
        self.counts1Plot.setLabel('left', 'Counts')
        self.counts1Plot.setLabel('bottom', 'Time')
        self.counts1Curve = self.counts1Plot.plot(pen='r')

        self.counts2Plot = self.canvas.addPlot(title='Detector 2 Counts')
        self.counts2Plot.setLabel('left', 'Counts')
        self.counts2Plot.setLabel('bottom', 'Time')
        self.counts2Curve = self.counts2Plot.plot(pen='b')

        self.coinPlot = self.canvas.addPlot(title='Coincident Detections')
        self.coinPlot.setLabel('left', 'Coincidences')
        self.coinPlot.setLabel('bottom', 'Time')
        self.coinCurve = self.coinPlot.plot(pen='g')



        

        self.show()

    def update_plot(self):
        # Update the data
        try:
            # Read a line of input from the console
            line = input()

            # Split the line into individual numbers
            numbers = line.split(',')

            # Convert the numbers to floats
            data = [float(num) for num in numbers[0:-1]]

            # Update the data lists
            if len(x_data_counts1) == 1000:
                x_data_counts1.popleft()
                y_data_counts1.popleft()
                y_data_counts2.popleft()
                y_data_coin.popleft()

            x_data_counts1.append(len(y_data_counts1))  # Assuming the time value is at index 0
            y_data_counts1.append(data[1])  # Assuming the data value is at index 1

            y_data_counts2.append(data[3])  # Assuming the data value is at index 1

            y_data_coin.append(data[6])  # Assuming the data value is at index 1

            # Update the plot
            self.counts1Curve.setData(list(x_data_counts1), list(y_data_counts1))

            self.counts2Curve.setData(list(x_data_counts1), list(y_data_counts2))
            
            self.coinCurve.setData(list(x_data_counts1), list(y_data_coin))

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

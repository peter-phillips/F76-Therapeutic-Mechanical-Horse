import serial

#Class for communicating with ClearCore over USB
class CCinterface():

    def __init__(self, port, baudrate=9600):
        self.clearCore = serial.Serial(port=port, baudrate=baudrate, timeout=.1)

    def send(self, message):
        self.clearCore.write(bytes(message + '\n', 'utf-8'))

    def receive(self):
        if self.clearCore.in_waiting > 0:
            line = self.clearCore.readline().decode('utf-8').rstrip()
            print("Incoming from ClearCore: " + line)
            return line
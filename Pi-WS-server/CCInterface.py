import serial

#Class for communicating with ClearCore over USB
class CCinterface():
    
    #Initialize serial communication with clearcore motor controller
    def __init__(self, port, baudrate=9600):
        try:
            self.clearCore = serial.Serial(port=port, baudrate=baudrate, timeout=.1)
        except:
            self.clearCore = False

    #Send message to ClearCore Motor Controller
    def send(self, message):
        if not(self.clearCore):
            return
        self.clearCore.write(bytes(message + '\n', 'utf-8'))

    #Recieve message to ClearCore Motor Controller, all allows you to pull multiple in case of stat_h
    def receive(self, all=False):
        if not(self.clearCore):
            return "No connection to ClearCore and motors, try powering horse off and on again"
        line =[]
        while self.clearCore.in_waiting > 0:
            line = self.clearCore.readline().decode('utf-8').rstrip()
            print("Incoming from ClearCore: " + line)
            return line

        if len(line) == 0:
            return "No new status"
        if all:
            return "\n".join(line)
        else:
            return line[-1]
        
    
    
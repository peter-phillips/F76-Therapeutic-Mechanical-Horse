import time
from machine import Pin

motor1Pin1=Pin(22,Pin.OUT)
motor1Pin2=Pin(24,Pin.OUT)
motor2Pin1=Pin(26,Pin.OUT)
motor2Pin2=Pin(28,Pin.OUT)


while True:
    time.sleep(5)
    #Pitch up to full
    pitchup()
    time.sleep(1)
    #Pitch center
    pitchdown()
    time.sleep(1)
    #pitchdown
    pitchdown()
    time.sleep(1)
    #pitch up to center
    pitchup()
    time.sleep(1)
    #roll left
    rollleft()
    time.sleep(1)
    #roll center
    rollright()
    time.sleep(1)
    #rollright
    rollright()
    time.sleep(1)
    #roll center
    rollleft()





def pitchup():
    motor2Pin2.value(1)
    motor1Pin1.value(1)
    time.sleep(.08)
    motor2Pin2.value(0)
    motor1Pin1.value(0)

def pitchdown():
    motor2Pin1.value(1)
    motor1Pin2.value(1)
    time.sleep(.08)
    motor2Pin1.value(0)
    motor1Pin2.value(0)

def rollright():
    motor2Pin2.value(1)
    motor1Pin2.value(1)
    time.sleep(.08)
    motor2Pin2.value(0)
    motor1Pin2.value(0)

def rollleft():
    motor2Pin1.value(1)
    motor1Pin1.value(1)
    time.sleep(.08)
    motor2Pin1.value(0)
    motor1Pin1.value(0)
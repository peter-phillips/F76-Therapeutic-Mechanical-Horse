import network
import time
import machine
from network import WLAN
import uwebsockets.client


class WiFiManager:
    ssid = 'MechHorseP2P'
    password = '12345678910'

    def __init__(self):
        print("Connecting to WiFi")
        # self.__connect_static("192.169.50.13")
        self.__connect_dynamic()
        self.__connect_socket()

    def __connect_dynamic(self):
        # print("attempting connection")
        # ip 192.168.50.13
        sta_if = WLAN(network.STA_IF)

        # ap_if = network.WLAN(network.AP_IF)
        # # IP addr, netmask, gateway, DNS
        # print("ap config:", ap_if.ifconfig())
        # ap_if.active(False)

        sta_if.active(True)

        while not sta_if.isconnected():
            sta_if.connect(self.ssid, self.password)
            time.sleep(.5)

        # IP addr, netmask, gateway, DNS
        self.gateway = sta_if.ifconfig()[2]
        print("WiFi Connected:", sta_if.ifconfig())

    def __connect_static(self, ip_addr):
        wlan = WLAN(network.STA_IF)
        wlan.active(True)
        wlan.ifconfig((ip_addr, '255.255.255.0', '192.168.50.1', '8.8.8.8'))

        if not wlan.isconnected():
            # change the line below to match your network ssid, security and password
            wlan.connect(self.ssid, self.password)
            while not wlan.isconnected():
                machine.idle()  # save power while waiting

        self.gateway = wlan.ifconfig()[2]
        # print("WiFi Connected:", wlan.ifconfig())

    def __connect_socket(self):
        self.websocket = uwebsockets.client.connect("ws://" + self.gateway + ":80/ws")

        connected = False
        while not connected:
            try:
                self.websocket.send("ping" + "\r\n")
                connected = True
            except:
                time.sleep(0.5)
                self.websocket = uwebsockets.client.connect("ws://" + self.gateway + ":80/ws")

    def send_code(self, mesg):
        self.websocket.send(mesg + "\r\n")

    def disconnect(self):
        self.websocket.close()
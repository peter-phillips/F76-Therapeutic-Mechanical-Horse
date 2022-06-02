#! /usr/bin/python

from asyncio.log import logger
import os.path
import tornado.httpserver
import tornado.websocket
import tornado.ioloop
import tornado.web
import logging

import CCInterface as CC
import time

# Logging config
logging.basicConfig(filename="serverLog.txt",
                    filemode='a',
                    format='%(asctime)s,%(msecs)d %(name)s %(levelname)s %(message)s',
                    datefmt='%H:%M:%S',
                    level=logging.DEBUG)


#Tornado Folder Paths
settings = dict(
    template_path = os.path.join(os.path.dirname(__file__), "templates"),
    static_path = os.path.join(os.path.dirname(__file__), "static"))

#Tornado server port and listen address
PORT = 80

# sudo nano /etc/udev/rules.d/99_usbdevices.rules
# should contain 
# SUBSYSTEM=="tty", ATTRS{idVendor}=="2890", ATTRS{idProduct}=="8022", SYMLINK+="clearCore"
# This makes sure that port is always the same for the motor controller 

inter = CC.CCinterface("/dev/clearCore")

# Handler for web application, hosts index.html at local host
# When connected to raspberry pi this is 192.168.50.1 or Horse.io
class MainHandler(tornado.web.RequestHandler):
    # Renders index.html on get on /
    def get(self):
        print("[HTTP](MainHandler) User Connected.")
        self.render("index.html")
        print("rendered")

# Web Socket handler on /ws for 192.168.50.1 or Horse.io on raspberry pi
class WSHandler(tornado.websocket.WebSocketHandler):
    # On open only print to log
    def open(self):
        print ('[WS] Connection was opened.')
        logger.info('[WS] Connection was opened.')

    # On message switch on message and send to ClearCore through CCinterface
    def on_message(self, message):
        print('[WS] Incoming message:' + message)
        logger.info('[WS] Incoming message:' + message)

        # Default return message
        horse_stat = "No communication from horse"

        # Message on_h turns on horse
        if message == "on_h":
            print("Sending on message to horse")
            logger.info("Sending on message to horse")
            inter.send(message)
            time.sleep(.1)
            horse_stat = inter.receive()

        # Message off_h turns off horse
        if message == "off_h":
            print("Sending off message to horse")
            logger.info("Sending off message to horse")
            inter.send(message)
            time.sleep(.1)
            horse_stat = inter.receive()
        
        # Message em_stop turns off horse immediately, disables motors and wait for power cycle to turn back on 
        if message == "em_stop":
            print("Sending emergency stop message to horse")
            logger.info("Sending emergency stop message to horse")
            inter.send(message)
            time.sleep(.1)
            horse_stat = inter.receive()
        
        # Message stat_h retrieves current status of the horse
        if message == "stat_h":
            inter.send(message)
            time.sleep(.1)
            horse_stat = inter.receive(True)
        # Send status back to horse    
        self.write_message(horse_stat)

    # on close of web socket send turn off signal to horse
    def on_close(self):
        print ('[WS] Connection was closed.')
        logger.info('[WS] Connection was closed.')
        inter.send("off_h")


application = tornado.web.Application([
    (r'/', MainHandler),
    (r'/ws', WSHandler),
    ], **settings)


if __name__ == "__main__":
    try:
        http_server = tornado.httpserver.HTTPServer(application)
        http_server.listen(PORT)
        main_loop = tornado.ioloop.IOLoop.instance()

        print ("Tornado Server started")
        logger.info("Tornado Server started")
        main_loop.start()

    except Exception as e:
        print ("Exception triggered - Tornado Server stopped.")
        print (e)
        logger.info("Exception triggered - Tornado Server stopped.")
        logger.info(e)

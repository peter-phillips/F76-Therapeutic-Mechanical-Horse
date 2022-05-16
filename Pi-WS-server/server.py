#! /usr/bin/python

import os.path
import tornado.httpserver
import tornado.websocket
import tornado.ioloop
import tornado.web

import CCInterface as CC
import time


#Tornado Folder Paths
settings = dict(
    template_path = os.path.join(os.path.dirname(__file__), "templates"),
    static_path = os.path.join(os.path.dirname(__file__), "static"))

#Tornado server port and listen address
PORT = 80
ADDRESS = "horse.local"

# sudo nano /etc/udev/rules.d/99_usbdevices.rules
# should contain 
# SUBSYSTEM=="tty", ATTRS{idVendor}=="2890", ATTRS{idProduct}=="8022", SYMLINK+="clearCore"

inter = CC.CCinterface("/dev/clearCore")


class MainHandler(tornado.web.RequestHandler):
    def get(self):
        print("[HTTP](MainHandler) User Connected.")
        self.render("index.html")
        print("rendered")


class WSHandler(tornado.websocket.WebSocketHandler):
    def open(self):
        print ('[WS] Connection was opened.')

    def on_message(self, message):
        print('[WS] Incoming message:' + message)

        horse_stat = "No communication from horse"

        if message == "on_h":
            print("Sending on message to horse")
            inter.send(message)
            time.sleep(.1)
            horse_stat = inter.receive()


        if message == "off_h":
            print("Sending off message to horse")
            inter.send(message)
            time.sleep(.1)
            horse_stat = inter.receive()
        
        if message == "em_stop":
            print("Sending emergency stop message to horse")
            inter.send(message)
            time.sleep(.1)
            horse_stat = inter.receive()
        self.write_message(horse_stat)


    def on_close(self):
        print ('[WS] Connection was closed.')


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
        main_loop.start()

    except Exception as e:
        print ("Exception triggered - Tornado Server stopped.")
        print (e)

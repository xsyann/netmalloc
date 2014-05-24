#!/usr/bin/env python
##
## server.py
##
## Made by xsyann
## Contact <contact@xsyann.com>
##
## Started on  Thu May  8 12:10:57 2014 xsyann
## Last update Sat May 24 21:53:59 2014 xsyann
##

"""
Netmalloc server.

Authors: Nicolas de Thore, Yann Koeth
"""

import socket
import threading
import SocketServer
import time
import ctypes
from ctypes import *

pages = {}

class Protocol(Structure):

    PAGE_SHIFT = 12
    PAGE_SIZE = 1 << PAGE_SHIFT

    PUT = 1
    GET = 2

    _fields_ = [("command", c_int),
                ("pid", c_int),
                ("address", c_ulong)]

    def pack(self, buf):
        fit = min(len(buf), ctypes.sizeof(self))
        ctypes.memmove(ctypes.addressof(self), buf, fit)


class ThreadedTCPRequestHandler(SocketServer.StreamRequestHandler):

    def handle(self):
        currentThread = threading.current_thread()
        print "New Client", currentThread.name

        while True:
            p = Protocol()
            data = self.request.recv(ctypes.sizeof(p))
            if not data:
                break
            p.pack(data)

            if p.command == Protocol.PUT:
                print "PUT pid = {}, address = {}".format(p.pid, hex(p.address))
                data = self.request.recv(Protocol.PAGE_SIZE)
                print "Received {} bytes".format(len(data))
                if p.pid in pages:
                    pages[p.pid][p.address] = data
                else:
                    pages[p.pid] = { p.address: data }

            if p.command == Protocol.GET:
                print "GET pid = {}, address = {}".format(p.pid, hex(p.address))
                page = chr(0) * Protocol.PAGE_SIZE
                if p.pid in pages:
                    pid_pages = pages[p.pid]
                    if p.address in pid_pages:
                        page = pid_pages[p.address]
                self.request.send(page)

        print "Client quit", currentThread.name

class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):

    daemon_threads = True
    allow_reuse_address = True

class FileTriggerServer:
    def __init__(self, host, port):
        self.server = ThreadedTCPServer((host, port), ThreadedTCPRequestHandler)
        self.ip, self.port = self.server.server_address

    def run(self):
        # Start a thread with the server -- that thread will then start one
        # more thread for each request
        serverThread = threading.Thread(target=self.server.serve_forever)
        # Exit the server thread when the main thread terminates
        serverThread.daemon = True
        serverThread.start()
        print "Server loop running in thread:", serverThread.name

        try:
            while True:
                time.sleep(0.2)
        except KeyboardInterrupt:
            print "Bye."
            self.server.shutdown()

if __name__ == "__main__":
    import argparse, sys

    default = {'l': "localhost", 'p': 12345 }

    parser = argparse.ArgumentParser(description="File Trigger server.")
    parser.add_argument("-l", "--location", type=str,
                        help="Host", default=default['l'])
    parser.add_argument("-p", "--port", type=int,
                        help= "Port", default=default['p'])
    args = parser.parse_args()

    print __doc__

    fts = FileTriggerServer(args.location, args.port)
    fts.run()



#!/usr/bin/env python
##
## server.py
##
## Made by xsyann
## Contact <contact@xsyann.com>
##
## Started on  Thu May  8 12:10:57 2014 xsyann
## Last update Mon May 26 17:03:14 2014 xs_yann
##

"""
Netmalloc server.

Authors: Nicolas de Thore, Yann Koeth
"""

import socket
from socket import error as SocketError
import errno
import threading
import SocketServer
import time
import ctypes
from ctypes import *

class Protocol(Structure):

    PAGE_SHIFT = 12
    PAGE_SIZE = 1 << PAGE_SHIFT

    PUT = 1
    GET = 2
    RM = 3
    RELEASE = 4

    _fields_ = [("command", c_int),
                ("pid", c_int),
                ("address", c_ulong)]

    def pack(self, buf):
        fit = min(len(buf), ctypes.sizeof(self))
        ctypes.memmove(ctypes.addressof(self), buf, fit)


class ThreadedTCPRequestHandler(SocketServer.StreamRequestHandler):

    pages = {}

    def recvall(self, size):
        data = ""
        while len(data) < size:
            packet = self.request.recv(size - len(data))
            if not packet:
                return None
            data += packet
        return data

    def handle(self):
        currentThread = threading.current_thread()
        print "New Client", currentThread.name

        while True:
            p = Protocol()
            data = ""
            try:
                data = self.recvall(ctypes.sizeof(p))
            except SocketError as e:
                if e.errno != errno.ECONNRESET:
                    raise
                print "Connection closed", currentThread.name
                break

            if not data:
                break
            p.pack(data)

            if p.command == Protocol.PUT:
                print "PUT pid = {}, address = {}".format(p.pid, hex(p.address))
                data = self.recvall(Protocol.PAGE_SIZE)
                print "Received {} bytes".format(len(data))
                if not data:
                    break
                if p.pid in self.pages:
                    self.pages[p.pid][p.address] = data
                else:
                    self.pages[p.pid] = { p.address: data }

            elif p.command == Protocol.GET:
                print "GET pid = {}, address = {}".format(p.pid, hex(p.address))
                page = chr(0) * Protocol.PAGE_SIZE
                if p.pid in self.pages:
                    pid_pages = self.pages[p.pid]
                    if p.address in pid_pages:
                        page = pid_pages[p.address]
                self.request.send(page)

            elif p.command == Protocol.RM:
                print "RM pid = {}, address = {}".format(p.pid, hex(p.address))
                if p.pid in self.pages:
                    pid_pages = self.pages[p.pid]
                    if p.address in pid_pages:
                        del pid_pages[p.address]
            elif p.command == Protocol.RELEASE:
                print "RELEASE"
                self.pages.clear()
            else:
                print "Unknown command",
                print p.command, p.pid, hex(p.address), len(data)
                print repr(data)

            print "Pages stored: ", sum([len(add) for add in self.pages.values()])

        print "Client quit", currentThread.name

class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):

    daemon_threads = True
    allow_reuse_address = True

class NetMallocServer:
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

    default = {'l': "0.0.0.0", 'p': 12345 }

    parser = argparse.ArgumentParser(description="Net Malloc server.")
    parser.add_argument("-l", "--location", type=str,
                        help="Host", default=default['l'])
    parser.add_argument("-p", "--port", type=int,
                        help= "Port", default=default['p'])
    args = parser.parse_args()

    print __doc__

    nms = NetMallocServer(args.location, args.port)
    nms.run()

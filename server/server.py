#!/usr/bin/env python
##
## server.py
##
## Made by xsyann
## Contact <contact@xsyann.com>
##
## Started on  Thu May  8 12:10:57 2014 xsyann
## Last update Sat May 24 03:18:07 2014 xsyann
##

"""
Netmalloc server.

Authors: Nicolas de Thore, Yann Koeth
"""

import socket
import threading
import SocketServer

class ThreadedTCPRequestHandler(SocketServer.StreamRequestHandler):

    def handle(self):
        currentThread = threading.current_thread()
        print "New Client", currentThread.name
        data = self.rfile.readline().rstrip('\r\n')
        print "{} ({})".format(data, currentThread.name)
        response = "{}: {}".format(currentThread.name, data)
        self.wfile.write("{}\r\n".format(response))

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
            while (True):
                pass
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



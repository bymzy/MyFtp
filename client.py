#! /usr/bin/env python
# -*- coding: utf-8 -*-

import cmd
import socket
import struct

SERVERIP = "127.0.0.1"
SERVERPORT = 3333


class MyCmd(cmd.Cmd):
    def __init__(self):
        cmd.Cmd.__init__(self)
        self.prompt = "->"
        self.Connect()

    def Connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect((SERVERIP, 3333))

    def _send(self, msg):
        self.sock.sendall(msg)

    def _recv(self, size):
        return self.sock.recv(size)

    def recv_int(self):
        data = self.sock.recv(4)
        val, = struct.unpack("!I", data)
        return val

    def do_quit(self, arg):
        return True

    def help_quit(self, arg):
        print "quit this program"

    def do_list(self, arg):
        cmd = "list"
        totalLen = 4 + len(cmd)
        msg = struct.pack("!II", totalLen, len(cmd)) + cmd
        self._send(msg)

        head = self.recv_int()
        dataLen = self.recv_int()
        listData = self._recv(dataLen)
        print listData

    def help_list(self):
        print "list files on current dir!"

    def help_help(self):
        print "show help info"


if __name__ == "__main__":
    cli = MyCmd();
    cli.cmdloop("welcome To MyFtp!!!");



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

    ## util
    def recv_int(self):
        data = self.sock.recv(4)
        val, = struct.unpack("!I", data)
        return val

    def recv_str(self):
        length = self.recv_int()
        if length == 0:
            return ""
        return self._recv(length)

    def pack_int(self, val):
        return struct.pack("!I", val)

    def pack_str(self, string):
        return self.pack_int(len(string)) + string

    def recv_err_errstr(self):
        err = self.recv_int()
        errStr = self.recv_str()
        return err, errStr

    ## quit
    def do_quit(self, arg):
        return True

    def help_quit(self, arg):
        print "quit this program"

    ## list
    def do_list(self, arg):
        # send command
        cmd = "list"
        totalLen = 4 + len(cmd)
        totalLen += 4 + len(arg)

        msg = self.pack_int(totalLen)
        msg += self.pack_str(cmd)
        msg += self.pack_str(arg)
        self._send(msg)

        # recv command reply
        totalLen = self.recv_int()
        err, errStr = self.recv_err_errstr()
        if err == 0:
            dirCount = self.recv_int()
            print 'dirs: '
            i = 0
            while i < dirCount:
                d = self.recv_str()
                print d
                i += 1

            fileCount = self.recv_int()
            print 'files: '
            i  = 0
            while i < fileCount:
                f = self.recv_str()
                print f
                i += 1
        else:
            print err, "failed: " + errStr

    def help_list(self):
        print "list files on current dir!"

    ## lock
    def try_lock(self, arg):
        cmd = "lock"
        fileName = arg
        lockType = 2
        fileType = 2

        totalLen = 4 + len(cmd)
        totalLen += 4;
        totalLen += 4;
        totalLen += 4 + len(fileName)

        msg = self.pack_int(totalLen)
        msg += self.pack_str(cmd)
        msg += self.pack_int(lockType)
        msg += self.pack_int(fileType)
        msg += self.pack_str(fileName)
        self._send(msg)

        totalLen = self.recv_int()
        err, errStr = self.recv_err_errstr()
        print err, errStr
        return err

    ## download file
    def do_get(self, arg):
        err = self.try_lock(arg)
        if err != 0:
            return

        #

    def help_help(self):
        print "show help info"


if __name__ == "__main__":
    cli = MyCmd();
    cli.cmdloop("welcome To MyFtp!!!");



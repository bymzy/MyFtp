#! /usr/bin/env python
# -*- coding: utf-8 -*-

import cmd
import socket
import struct
import os
import hashlib

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

    def pack_64int(self, val):
        return struct.pack("!Q", val)

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

    def calc_md5(self, fileName):
        fp = open(fileName, 'r')
        m = hashlib.md5()
        while 1:
            chunk = fp.read(4096)
            if chunk:
                m.update(chunk)
            else:
                break
        fp.close()
        return m.hexdigest()

    def get_md5(self, arg):
        cmd = "md5"
        fileName = arg

        totalLen = 4 + len(cmd)
        totalLen += 4 + len(fileName)

        msg = self.pack_int(totalLen)
        msg += self.pack_str(cmd)
        msg += self.pack_str(fileName)
        self._send(msg)

        totalLen = self.recv_int()
        err, errstr = self.recv_err_errstr()
        md5 = ""
        if err == 0:
            md5 = self.recv_str()
            print md5
        else:
            print err, "md5 failed", errstr
        return err, md5

    def calc_temp_file_name(self, arg ,md5):
        return ".%s_%s" % (arg, md5)

    ## recv file and write file
    def start_with_offset(self, fileName, md5):
        offset = 0
        temp_file_name = self.calc_temp_file_name(os.path.basename(fileName), md5) 
        print 'temp file name is %s ' % temp_file_name
        if os.path.isfile(temp_file_name):
            offset = os.path.getsize(temp_file_name)
        else:
            print 'mknode %s ' % temp_file_name
            os.mknod(temp_file_name, 0644)

        # open file for write
        fp = open(temp_file_name, 'w')
        fp.seek(offset)
        err = 0
        errStr = ''

        while 1:
            req = self._generate_read_req(fileName, offset, 64 << 10)
            self._send(req)

            recvLen = self.recv_int()
            err, errStr = self.recv_err_errstr()
            if err != 0:
                print 'read file failed %s, %s ' % (err, errStr)
                break;

            dataLen = self.recv_int()
            print 'data len %d' % dataLen
            if dataLen > 0: 
                data = self._recv(dataLen)
                fp.write(data)
            offset += dataLen

            if dataLen != 64 << 10:
                print 'read file end !'
                break

        fp.close()
        return err

    def _generate_read_req(self, fileName, offset, size):
        '''
        protocol:
        string req type
        string file name
        8 bytes offset
        4 bytes len
        '''
        cmd = "read"
        totalLen = 4 + len(cmd)
        totalLen += 4 + len(fileName)
        totalLen += 8  
        totalLen += 4  

        msg = self.pack_int(totalLen)
        msg += self.pack_str(cmd)
        msg += self.pack_str(fileName)
        msg += self.pack_64int(offset)
        msg += self.pack_int(size)

        return msg

    ## download file
    def do_get(self, arg):
        err = self.try_lock(arg)
        if err != 0:
            return

        # ask for file md5 
        md5 = ""
        err, md5 = self.get_md5(arg)
        if err != 0:
            return

        print "file md5 %s" % md5
        err = self.start_with_offset(arg, md5)
        if err != 0:
            print 'get file failed!'
            return
        
        # check md5
        print 'base name %s ' % os.path.basename(arg)
        temp_file_name = self.calc_temp_file_name(os.path.basename(arg), md5) 
        localMd5 = self.calc_md5(temp_file_name)

        print localMd5, md5

        if localMd5 == md5:
            print 'download file %s success!' % arg
            os.rename(temp_file_name, os.path.basename(arg))
        else:
            print 'download file %s failed!' % arg
            #os.unlink(temp_file_name)
        return 

    def help_help(self):
        print "show help info"


if __name__ == "__main__":
    cli = MyCmd();
    cli.cmdloop("welcome To MyFtp!!!");



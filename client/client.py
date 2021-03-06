#! /usr/bin/env python
# -*- coding: utf-8 -*-

import cmd
import socket
import struct
import os
import hashlib
from optparse import OptionParser, OptionGroup
import sys

SERVERIP = '127.0.0.1'
SERVERPORT = 3333

class MyParser(OptionParser):
    def __init__(self):
        OptionParser.__init__(self)

class MyCmd(cmd.Cmd):
    currentDir = '/'
    lastDir = '/'

    def __init__(self):
        cmd.Cmd.__init__(self)
        self.prompt = '->'
        self.Connect()

    def Connect(self):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #self.sock.setblocking(True)
        #self.sock.settimeout(None)
        self.sock.connect((SERVERIP, 3333))

    def _send(self, msg):
        self.sock.sendall(msg)

    def _recv(self, size):
        toRead = size
        data = ''
        while toRead > 0:
            temp = self.sock.recv(toRead)
            if len(temp) == 0:
                print 'remote close the socket !!!'
                break
            toRead -= len(temp)
            data += temp
        if len(data) != size:
            print 'recv size not match , req size %d, to read %d, real size %d !!!!!' % (size, toRead, len(data))
            exit(1)
        return data

    ## util
    def recv_int(self):
        data = self._recv(4)
        val, = struct.unpack('!I', data)
        return val

    def recv_64int(self):
        data = self._recv(8)
        val, = struct.unpack('!Q', data);
        return val

    def recv_str(self):
        length = self.recv_int()
        if length == 0:
            return ''
        return self._recv(length)

    def pack_int(self, val):
        return struct.pack('!I', val)

    def pack_64int(self, val):
        return struct.pack('!Q', val)

    def pack_str(self, string):
        return self.pack_int(len(string)) + string

    def recv_err_errstr(self):
        err = self.recv_int()
        errStr = self.recv_str()
        return err, errStr

    def print_blue(self, msg):
        print '\033[1;34m%s \033[0m' % msg

    def print_red(self, msg):
        print '\033[1;31m%s \033[0m' % msg

    def print_green(self, msg):
        print '\033[1;32m%s \033[0m' % msg

    def get_real_dir(self, dirName):
        if dirName[-1] != '/':
            dirName = '%s/' % dirName

        if dirName[0] != '/':
            dirName = ''.join((self.currentDir, dirName))

        return dirName

    ## quit
    def do_quit(self, arg):
        return True

    def help_quit(self):
        print 'quit this program!!!'

    ## list
    def do_ls(self, dirName):
        if len(dirName) > 0 and dirName[-1] != '/':
            dirName = '%s/' % dirName
        
        if len(dirName) == 0:
            dirName = self.currentDir
        elif dirName[0] != '/':
            dirName = ''.join((self.currentDir, dirName))

        self._do_list(dirName)

    def _do_list(self, arg):
        # send command
        cmd = 'list'
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
            self.print_green('dirs count %s : ' % dirCount)
            i = 0
            while i < dirCount:
                d = self.recv_str()
                self.print_blue(d)
                i += 1

            print ''
            fileCount = self.recv_int()
            self.print_green('file count %s :' % fileCount)
            i  = 0
            while i < fileCount:
                f = self.recv_str()
                print f
                i += 1
        else:
            self.print_red('failed: ' + errStr)
        return err

    def help_ls(self):
        print 'list files on current dir, eg: list /root'

    ## lock
    def try_lock(self, arg, upload = 0):
        cmd = 'lock'
        fileName = arg
        lockType = 2
        fileType = 2

        totalLen = 4 + len(cmd)
        totalLen += 4
        totalLen += 4
        totalLen += 4 + len(fileName)
        totalLen += 4

        msg = self.pack_int(totalLen)
        msg += self.pack_str(cmd)
        msg += self.pack_int(lockType)
        msg += self.pack_int(fileType)
        msg += self.pack_str(fileName)
        msg += self.pack_int(upload)
        self._send(msg)

        totalLen = self.recv_int()
        err, errStr = self.recv_err_errstr()
        if err != 0:
            self.print_red(errStr)
        return err

    def try_unlock(self, arg, upload = 0):
        cmd = 'unlock'
        fileName = arg
        lockType = 2
        fileType = 2

        totalLen = 4 + len(cmd)
        totalLen += 4;
        totalLen += 4;
        totalLen += 4 + len(fileName)
        totalLen += 4

        msg = self.pack_int(totalLen)
        msg += self.pack_str(cmd)
        msg += self.pack_int(lockType)
        msg += self.pack_int(fileType)
        msg += self.pack_str(fileName)
        msg += self.pack_int(upload)
        self._send(msg)

        totalLen = self.recv_int()
        err, errStr = self.recv_err_errstr()
        if err != 0:
            self.print_red(errStr)
        return err

    def calc_md5(self, fileName):
        fp = open(fileName, 'rb')
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
        cmd = 'md5'
        fileName = arg

        totalLen = 4 + len(cmd)
        totalLen += 4 + len(fileName)

        msg = self.pack_int(totalLen)
        msg += self.pack_str(cmd)
        msg += self.pack_str(fileName)
        self._send(msg)

        totalLen = self.recv_int()
        err, errstr = self.recv_err_errstr()
        md5 = ''
        if err == 0:
            md5 = self.recv_str()
            print md5
        else:
            self.print_red('get md5 failed, err: %d, errstr: %s' % (err, errstr))
        return err, md5

    def calc_temp_file_name(self, arg ,md5):
        return '.%s_%s' % (arg, md5)

    ## recv file and write file
    def start_with_offset(self, fileName, md5):
        offset = 0
        temp_file_name = self.calc_temp_file_name(os.path.basename(fileName), md5) 
        print 'temp file name is %s ' % temp_file_name
        if os.path.isfile(temp_file_name):
            offset = os.path.getsize(temp_file_name)
        else:
            print 'mknode %s ' % temp_file_name
            #os.mknod(temp_file_name, 0644) #mknode is not support on Windows

        # open file for write
        fp = open(temp_file_name, 'wb')
        fp.seek(offset)
        err = 0
        errStr = ''

        while 1:
            req = self._generate_read_req(fileName, offset, 64 << 10)
            self._send(req)

            recvLen = self.recv_int()
            while recvLen == 0:
                recvLen = self.recv_int()

            err, errStr = self.recv_err_errstr()
            if err != 0:
                self.print_red('read file failed %s, %s ' % (err, errStr))
                break;

            dataLen = self.recv_int()
            if dataLen > 0: 
                data = self._recv(dataLen)
                fp.seek(offset)
                fp.write(data)
            offset += dataLen

            if dataLen != 64 << 10:
                print 'read file end !'
                break

        fp.flush()
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
        cmd = 'read'
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
        fileName = arg
        if fileName is None or len(fileName) == 0:
            return

        if fileName[0] != '/':
            fileName = self.currentDir + fileName

        err = self.try_lock(fileName)
        if err != 0:
            return

        # ask for file md5 
        md5 = ''
        err, md5 = self.get_md5(fileName)
        if err != 0:
            return

        print 'file md5 %s' % md5
        err = self.start_with_offset(fileName, md5)
        if err != 0:
            print 'get file failed!'
            return

        # check md5
        print 'base name :%s ' % os.path.basename(fileName)
        temp_file_name = self.calc_temp_file_name(os.path.basename(fileName), md5)
        localMd5 = self.calc_md5(temp_file_name)

        print localMd5, md5

        if localMd5 == md5:
            print 'download file %s success!' % fileName
            os.rename(temp_file_name, os.path.basename(fileName))
        else:
            print 'download file %s failed!' % fileName
            os.unlink(temp_file_name)

        self.try_unlock(fileName)
        return

    def help_get(self):
        print 'download file from server, eg: get /root/file'

    ## upload file
    def try_create(self, md5, size, filePath):
        cmd = 'create'
        totalLen = 4 + len(cmd)
        totalLen += 4 + len(filePath)
        totalLen += 4 + len(md5)
        totalLen += 8

        msg = self.pack_int(totalLen)
        msg += self.pack_str(cmd)
        msg += self.pack_str(filePath)
        msg += self.pack_str(md5)
        msg += self.pack_64int(size)
        self._send(msg)

        totalLen = self.recv_int()
        err, errstr = self.recv_err_errstr()
        offset = 0
        print err, errstr
        if err != 0:
            self.print_red(errstr)
        else:
            offset = self.recv_64int()
        return err, offset
    
    def _read_file(self, f, size):
        toRead = size
        chunk = ''
        data = ''
        while toRead > 0:
            chunk = f.read(toRead)
            if not chunk:
                break
            data += chunk
            toRead -= len(chunk)

        return data

    def put_file(self, srcFileName, destFileName, md5, offset, size):
        err = 0
        chunkSize = 256 * 1024
        with open(srcFileName, 'rb') as f:
            while size > 0:
                f.seek(offset)
                chunk = self._read_file(f, chunkSize)

                cmd = 'write'
                totalLen = 4 + len(cmd)
                totalLen += 4 + len(destFileName)
                totalLen += 4 + len(md5)
                totalLen += 8
                totalLen += 4
                totalLen += len(chunk)

                msg = self.pack_int(totalLen)
                msg += self.pack_str(cmd)
                msg += self.pack_str(destFileName)
                msg += self.pack_str(md5)
                msg += self.pack_64int(offset)
                msg += self.pack_int(len(chunk))
                msg += chunk
                self._send(msg)

                totalLen = self.recv_int()
                err, errstr = self.recv_err_errstr()
                if err != 0:
                    self.print_red(errstr)
                    break

                offset += len(chunk)
                size -= len(chunk)
                if len(chunk) < chunkSize or size <= 0:
                    print 'send file end!!!'
                    break
        return err

    def put_file_end(self, destFileName, md5):
        cmd = 'put_file_end'

        totalLen = 4 + len(cmd)
        totalLen += 4 + len(destFileName)
        totalLen += 4 + len(md5)

        msg = self.pack_int(totalLen)
        msg += self.pack_str(cmd)
        msg += self.pack_str(destFileName)
        msg += self.pack_str(md5)
        self._send(msg)

        totalLen = self.recv_int()
        err, errstr = self.recv_err_errstr()
        if err != 0:
            self.print_red(errstr)
        return err

    # put file /dir
    def do_put(self, arg):
        if len(arg) == 0:
            return False

        argList = arg.split(' ')
        if len(argList) > 2:
            self.print_red("invalid args!!!");
            return False
        elif len(argList) == 1:
            dirName = self.currentDir
        elif len(argList) == 2:
            dirName = argList[1]

        srcFilePath = argList[0]

        self._do_put(srcFilePath, dirName)

    def _do_put(self, srcFilePath, dirName):
        destFilePath = ''

        if dirName[-1] != '/':
            dirName += '/'

        fileName = os.path.basename(srcFilePath)
        destFilePath = dirName + fileName
        offset = 0

        err = self.try_lock(destFilePath, 1)
        if err != 0:
            return

        while 1:
            md5 = self.calc_md5(srcFilePath)
            size = os.path.getsize(srcFilePath)

            # put file begin
            err, offset = self.try_create(md5, size, destFilePath)
            if err != 0:
                break 

            # try send data
            err = self.put_file(srcFilePath, destFilePath, md5, offset, size)
            if err != 0:
                break

            # put file end
            err = self.put_file_end(destFilePath, md5)
            if err != 0:
                break
            break

        self.try_unlock(destFilePath, 1)

    def help_put(self):
        print 'upload file to server, eg: put file /dir'

    # del file
    def do_del(self, arg):
        argList = arg.split(' ')

        for i in range(len(argList)):
            if argList[i][0] != '/':
                argList[i] = self.currentDir + argList[i]
        
        self._do_del(' '.join(argList))

    def _do_del(self, arg):
        argList = arg.split(' ')
        if len(argList) < 1:
            self.print_red('invalid args %s' % arg)
            return 2

        err = 0
        
        for fileName in argList:
            print 'del file', fileName
            err = self.try_lock(fileName)
            if err != 0:
                return 1

            err = self.del_file(fileName)
            if err != 0:
                return 1

    def del_file(self, fileName):
        cmd = 'delete'

        totalLen = 4 + len(cmd)
        totalLen += 4 + len(fileName)

        msg = self.pack_int(totalLen)
        msg += self.pack_str(cmd)
        msg += self.pack_str(fileName)
        self._send(msg)

        totalLen = self.recv_int()
        err, errstr = self.recv_err_errstr()
        if err != 0:
            self.print_red(errstr)
        return err

    def help_del(self):
        print 'del file on server, eg: del /root/file'

    def op_dir(self, dirName, opCode):
        cmd = 'opdir'

        totalLen = 4 + len(cmd)
        totalLen += 4 + len(dirName)
        totalLen += 4 + len(opCode)

        msg = self.pack_int(totalLen)
        msg += self.pack_str(cmd)
        msg += self.pack_str(dirName)
        msg += self.pack_str(opCode)
        self._send(msg)

        totalLen = self.recv_int()
        err, errstr = self.recv_err_errstr()
        if err != 0:
            self.print_red(errstr)
        return err

    #mkdir on server 
    #return success if dir exists , create dir if not
    def do_mkdir(self, dirName):
        if len(dirName) == 0:
            print 'invalid dirName'
            return None

        dirName = self.get_real_dir(dirName)
        err = self._do_mkdir(dirName) 

    def _do_mkdir(self, dirName):
        err = self.op_dir(dirName, 'mkdir') 
        return err

    def help_mkdir(self):
        print 'mkdir on server'

    #del dir on server
    #return success if dir exists and dir empty
    def do_rmdir(self, dirName):
        if len(dirName) == 0:
            print 'invalid dirName'
            return None

        dirName = self.get_real_dir(dirName)
        err = self._do_rmdir(dirName) 

    def _do_rmdir(self, dirName):
        err = self.op_dir(dirName, 'rmdir') 
        return err

    def help_rmdir(self):
        print 'rmdir on server'
        
    #change dir on server
    #return success if dir exists
    def do_cd(self, dirName):
        if len(dirName) == 0:
            dirName = '/'
        elif dirName == '..':
            if self.currentDir == '/':
                return None
            dirName = os.path.dirname(self.currentDir[:-1])
        elif dirName == '-':
            dirName = self.lastDir

        if dirName == self.currentDir:
            return None

        dirName = self.get_real_dir(dirName)
        err = self.op_dir(dirName, 'chdir') 
        if err == 0:
            self.lastDir = self.currentDir
            self.currentDir = dirName

    def do_pwd(self, arg):
        self.print_green(self.currentDir)

    def help_pwd(self):
        print 'show current dir'

    def help_cd(self):
        print 'chdir on server'

    def help_help(self):
        print 'show help info'

def command_list():
    print_green('avaliable list: put, list , get , del, mkdir, rmdir, inter')

def print_blue(msg):
    print '\033[1;34m%s \033[0m' % msg

def print_red(msg):
    print '\033[1;31m%s \033[0m' % msg

def print_green(msg):
    print '\033[1;32m%s \033[0m' % msg

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print 'not enough arg count!'
        exit(1)

    mod = 'cmd'
    command = sys.argv[1]
    parser = MyParser()
    parser.add_option('-i', '--server_ip', help='server ip', )
    parser.add_option('-p', '--server_port', default=3333, )

    if command == 'put':
        parser.add_option('-f', '--file', help='src file try put to server')
        parser.add_option('-d', '--dir', help='dest dir on server', default='/')

    elif command == 'list':
        parser.add_option('-d', '--dir', help='dest dir on server', default='/')

    elif command == 'get':
        parser.add_option('-f', '--file', help='dest dir on server')

    elif command == 'del':
        parser.add_option('-f', '--file', help='dest dir on server, --file file1,file2,file3')

    elif command == 'mkdir':
        parser.add_option('-d', '--dir', help='dest dir on server', default='/')

    elif command == 'rmdir':
        parser.add_option('-d', '--dir', help='dest dir on server', default='/')

    elif command == 'inter':
        mod = 'inter'

    else:
        print_red('invalid command !!!')
        print ''
        command_list()
        exit(1)

    options, args = parser.parse_args(sys.argv[2:])

    if options.server_ip == None:
        print_red('server ip not assigned!!!')
        print ''
        parser.print_help()
        exit(1)

    SERVERIP = options.server_ip
    SERVERPORT = options.server_port

    cli = MyCmd();
    if command == 'put':
        srcFile = options.file
        destDir = options.dir
        cli._do_put(srcFile, destDir)

    elif command == 'list':
        destDir = options.dir
        if destDir ==  '' or destDir == None:
            destDir =  '/'
        cli._do_list(destDir)

    elif command == 'get':
        srcFile = options.file
        cli.do_get(srcFile)

    elif command == 'del':
        srcFile = options.file
        cli.do_del(' '.join(srcFile.split(',')))

    elif command == 'mkdir':
        destDir = options.dir
        cli._do_mkdir(destDir)

    elif command == 'rmdir':
        destDir = options.dir
        cli._do_rmdir(destDir)

    elif command == 'inter':
        cli.cmdloop('welcome To MyFtp!!!');



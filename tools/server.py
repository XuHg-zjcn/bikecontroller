#!/usr/bin/env python3
#######################################################################
# Copyright (C) 2024  徐瑞骏(科技骏马)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
########################################################################
import socket
import time
import os
from enum import Enum

DEV_BASEPATH = '/littlefs'
LOCAL_BASEPATH = 'data/littlefs'

class Cmd(Enum):
    FileOpen = 0
    FileClose = 1
    FileRead = 2
    FileDelete = 3
    DirOpen = 4
    DirClose = 5
    DirRead = 6

class Device:
    def __init__(self, sock, addr):
        self.sock = sock
        self.addr = addr
        print('msg', self.sock.recv(256))

    def lsdir(self, path):
        flst = []
        cmd = bytes([Cmd.DirOpen.value])+path.encode()+b'\0'
        self.sock.send(cmd)
        ret = self.sock.recv(256)
        assert ret == b'\x01\x00'

        while True:
            cmd = bytes([Cmd.DirRead.value, 20])
            self.sock.send(cmd)
            data = self.sock.recv(256)
            if len(data) <= 2:
                assert data == b'\x00' or data == b'\x01\x00'
                break
            flst.extend(map(bytes.decode, filter(lambda x:len(x)!=0, data[1:].split(b'\0'))))
            if data[-2:] == b'\x00\x00':
                break

        cmd = bytes([Cmd.DirClose.value])
        self.sock.send(cmd)
        ret = self.sock.recv(256)
        assert ret == b'\x01\x00'
        return flst

    def readfile(self, path):
        cmd = bytes([Cmd.FileOpen.value])+path.encode()+b'\0'
        self.sock.send(cmd)
        ret = self.sock.recv(256)
        assert ret == b'\x01\x00'

        ba = bytearray()
        while True:
            cmd = bytes([Cmd.FileRead.value, 255])
            self.sock.send(cmd)
            data = self.sock.recv(256)
            assert len(data) == data[0]+1
            ba.extend(data[1:])
            if data[0] < 255:
                break

        cmd = bytes([Cmd.FileClose.value])
        self.sock.send(cmd)
        ret = self.sock.recv(256)
        assert ret == b'\x01\x00'
        return ba

    def downloadfile(self, path, save_path):
        save_dir = os.path.dirname(save_path)
        if not os.path.isdir(save_dir):
            os.makedirs(save_dir)
        f = open(save_path, 'wb')
        cmd = bytes([Cmd.FileOpen.value])+path.encode()+b'\0'
        self.sock.send(cmd)
        ret = self.sock.recv(256)
        assert ret == b'\x01\x00'

        while True:
            cmd = bytes([Cmd.FileRead.value, 255])
            self.sock.send(cmd)
            data = self.sock.recv(256)
            assert len(data) == data[0]+1
            f.write(data[1:])
            if data[0] < 255:
                break

        nb = f.tell()
        f.close()
        cmd = bytes([Cmd.FileClose.value])
        self.sock.send(cmd)
        ret = self.sock.recv(256)
        assert ret == b'\x01\x00'
        return nb


sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind(('0.0.0.0', 3234))
sock.listen()
s, a = sock.accept()
dev = Device(s, a)
files = dev.lsdir(DEV_BASEPATH)
print('files:', files)
for i in files:
    dev_path = DEV_BASEPATH + '/' + i
    local_path = os.path.join(LOCAL_BASEPATH, i)
    print(dev_path)
    print(dev.downloadfile(dev_path, local_path), 'bytes')
    print()

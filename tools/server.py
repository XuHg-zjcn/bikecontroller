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
from enum import Enum

BASEPATH = '/littlefs'

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
        cmd = bytes([Cmd.DirOpen.value])+path.encode()+b'\0'
        self.sock.send(cmd)
        ret = self.sock.recv(256)
        assert ret == b'\x01\x00'

        cmd = bytes([Cmd.DirRead.value, 10])
        self.sock.send(cmd)
        data = self.sock.recv(256)

        cmd = bytes([Cmd.DirClose.value])
        self.sock.send(cmd)
        ret = self.sock.recv(256)
        assert ret == b'\x01\x00'
        return list(map(bytes.decode, filter(lambda x:len(x)!=0, data[1:].split(b'\0'))))

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


sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind(('0.0.0.0', 3234))
sock.listen()
s, a = sock.accept()
dev = Device(s, a)
files = dev.lsdir(BASEPATH)
print('files:', files)
for i in files:
    path = BASEPATH + '/' + i
    print(path)
    print(dev.readfile(path))
    print()

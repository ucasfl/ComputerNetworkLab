#!/usr/bin/python

import sys
import string
import socket
from time import sleep

data = string.digits + string.lowercase + string.uppercase

s = socket.socket()
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

port = int(sys.argv[1])

s.bind(('0.0.0.0', port))
s.listen(3)

cs, addr = s.accept()

sleep(2)

s.close()

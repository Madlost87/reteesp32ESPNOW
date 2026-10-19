"""Capture a short serial boot/status check after an authorized upload."""
import sys
import time
from pathlib import Path
import serial
port, destination = sys.argv[1:3]
s = serial.Serial(port=None, baudrate=115200, timeout=0.2)
s.dtr = False
s.rts = False
s.port = port
s.open()
try:
    s.dtr = False
    s.rts = True
    time.sleep(0.1)
    s.rts = False
    end = time.monotonic() + 12
    data = bytearray()
    sent = False
    while time.monotonic() < end:
        data.extend(s.read(4096))
        if not sent and time.monotonic() > end - 9:
            s.write(b'help\nstats\n')
            sent = True
    with Path(destination).open('xb') as f:
        f.write(data)
    print(data.decode('utf-8', errors='replace'))
finally:
    s.close()

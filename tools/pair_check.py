"""Prova seriale breve su due schede gia programmate; reset e comandi di simulazione."""
import argparse
from pathlib import Path
import time
import serial

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--tx',required=True)
parser.add_argument('--rx',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
out=Path(args.output)
out.mkdir(parents=True,exist_ok=True)
ports=[]
logs=[]
try:
    for role,device in [('tx',args.tx),('rx',args.rx)]:
        log=(out/f'{role}-serial.log').open('xb'); logs.append(log)
        port=serial.Serial(port=None,baudrate=115200,timeout=0)
        port.dtr=False; port.rts=False; port.port=device; port.open();ports.append(port)
    for port in ports:
        port.rts=True
    time.sleep(.1)
    for port in ports:
        port.rts=False
    start=time.monotonic()
    schedule=[(62,b'test off\n'),(65,b'alarm\n'),(68,b'alarm\n'),(71,b'alarm\n'),(74,b'stats\n')]
    while time.monotonic()-start < 77:
        elapsed=time.monotonic()-start
        if schedule and elapsed>=schedule[0][0]:
            _,command=schedule.pop(0)
            ports[0].write(command)
            if command==b'stats\n':ports[1].write(command)
            print(f'{elapsed:.1f}s TX {command.decode().strip()}',flush=True)
        for port,log in zip(ports,logs):
            data=port.read(8192)
            if data:log.write(data);log.flush()
        time.sleep(.01)
    print('Acquisizione completata: analizzare entrambi i log.',flush=True)
finally:
    for port in ports:port.close()
    for log in logs:log.close()

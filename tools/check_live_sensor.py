"""Reset esplicito, acquisizione seriale e polling HTTP durante telemetria continua."""
import argparse
import json
from pathlib import Path
import threading
import time
import urllib.request
import serial

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--tx', required=True)
p.add_argument('--rx', required=True)
p.add_argument('--url', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output)
out.mkdir(parents=True, exist_ok=True)
stop = threading.Event()

def poll():
    with (out / 'http.jsonl').open('x') as f:
        while not stop.wait(.25):
            start = time.monotonic()
            try:
                with urllib.request.urlopen(a.url + '/api/status', timeout=1.5) as r:
                    data = json.load(r)
                row = {'elapsed': start - began, 'ms': (time.monotonic()-start)*1000, 'status': data}
            except Exception as e:
                row = {'elapsed': start-began, 'error': str(e)}
            f.write(json.dumps(row)+'\n')
            f.flush()

ports, logs = [], []
worker = None
try:
    for role, device in [('tx', a.tx), ('rx', a.rx)]:
        logs.append((out / f'{role}-serial.log').open('xb'))
        port = serial.Serial(port=None, baudrate=115200, timeout=0)
        port.dtr = False
        port.rts = False
        port.port = device
        port.open()
        ports.append(port)
    for port in ports:
        port.rts = True
    time.sleep(.1)
    for port in ports:
        port.rts = False
    began = time.monotonic()
    schedule = [(15, b'alarm\n'), (30, b'alarm\n'), (45, b'alarm\n'), (60, b'stats\n')]
    while time.monotonic()-began < 63:
        elapsed = time.monotonic()-began
        if elapsed > 5 and worker is None:
            worker = threading.Thread(target=poll)
            worker.start()
        if schedule and elapsed >= schedule[0][0]:
            _, command = schedule.pop(0)
            ports[0].write(command)
            if command == b'stats\n':
                ports[1].write(command)
            print(f'{elapsed:.1f}s {command.decode().strip()}', flush=True)
        for port, log in zip(ports, logs):
            data = port.read(8192)
            if data:
                log.write(data)
                log.flush()
        time.sleep(.005)
finally:
    stop.set()
    if worker:
        worker.join()
    for port in ports:
        port.close()
    for log in logs:
        log.close()
print('Acquisizione completata.', flush=True)

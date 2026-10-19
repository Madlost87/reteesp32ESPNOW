"""Prova bidirezionale reale: richiede entrambe le ESP32 con firmware 0.6.0."""
import argparse
import concurrent.futures
import json
import time
import urllib.request
import urllib.error
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--tx',default='192.168.1.117')
p.add_argument('--rx',default='192.168.1.176')
p.add_argument('--report',type=Path,required=True)
a=p.parse_args()
hosts=[a.tx,a.rx]
def status(host):
    with urllib.request.urlopen(f'http://{host}/api/radio-test',timeout=3) as r:return json.load(r)
def send(pair):
    host,text=pair
    req=urllib.request.Request(f'http://{host}/api/radio-test',data=text.encode('utf-8'),headers={'X-Radio-Test':'1','Content-Type':'text/plain; charset=utf-8'},method='POST')
    with urllib.request.urlopen(req,timeout=3) as r:
        assert r.status==202
        return json.load(r)
for host in hosts:
    with urllib.request.urlopen(f'http://{host}/radio',timeout=3) as r:
        assert r.read()==Path('shared/web/radio.html').read_bytes()
    try:
        urllib.request.urlopen(urllib.request.Request(f'http://{host}/api/radio-test',data=b'',method='POST'),timeout=3)
    except urllib.error.HTTPError as e:
        assert e.code==400
    else:raise AssertionError('POST senza header non rifiutato')
    for invalid in (b'',b' ',b'a'*49,b'\xff',b'a\0b',b'a\nb'):
        try:
            req=urllib.request.Request(f'http://{host}/api/radio-test',data=invalid,headers={'X-Radio-Test':'1'},method='POST')
            urllib.request.urlopen(req,timeout=3)
        except urllib.error.HTTPError as e:assert e.code==400
        else:raise AssertionError('Testo non valido accettato')
before=[status(h) for h in hosts]
texts=['Ciao dalla scheda','Caffè già pronto?','Virgolette "ciao" e slash \\', '<b>Testo, non HTML</b>', '🙂'*48]
rounds=[]
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
    for i in range(10):
        payloads=[texts[i%5],texts[(i+1)%5]]
        list(pool.map(send,zip(hosts,payloads)))
        deadline=time.monotonic()+5
        while True:
            states=list(pool.map(status,hosts))
            if all(s['state']==3 for s in states):break
            assert time.monotonic()<deadline,states
            time.sleep(.1)
        for old,new in zip(before,states):
            assert new['received_count']==old['received_count']+i+1,states
        assert states[0]['received_text']==payloads[1] and states[1]['received_text']==payloads[0],states
        assert states[0]['sent_text']==payloads[0] and states[1]['sent_text']==payloads[1],states
        rounds.append(states)
        print(f'Prova {i+1}: ricevuto e confermato in entrambe le direzioni',flush=True)
        time.sleep(.2)
with urllib.request.urlopen(f'http://{a.rx}/api/status',timeout=3) as r:central=json.load(r)
assert central['sensor_state']==0 and central['alarm_count']==0,central
with a.report.open('x') as f:json.dump({'before':before,'rounds':rounds,'central':central},f,indent=2)
print('SUPERATO: 20 testi identici tra mittente e destinatario, 20 conferme; nessun allarme generato.')

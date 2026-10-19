"""Confronta log CSV TX/RX della prova breve, senza accedere ai dispositivi."""
import argparse
import csv
import json
from pathlib import Path

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('directory')
args=parser.parse_args()
root=Path(args.directory)
def rows(role,kind):
    with (root/f'{role}-{kind}.csv').open() as f:
        return list(csv.DictReader(f))
txe,rxe=rows('tx','events'),rows('rx','events')
txs,rxs=rows('tx','summary'),rows('rx','summary')
assert txs and rxs, 'Riepiloghi mancanti'
tx,rx=txs[-1],rxs[-1]
confirmed={(r['ref_session'],r['sequence'],r['event']) for r in txe if r['type']=='confirmed'}
received={(r['packet_session'],r['sequence'],r['event']) for r in rxe if r['type'] in ('probe','alarm_simulated')}
assert confirmed==received, 'Eventi confermati e ricevuti non corrispondono'
assert int(tx['originals'])==int(tx['confirmed'])==len(confirmed), 'Originali non tutti confermati'
alarms = sum(r['type']=='alarm_simulated' for r in rxe)
assert alarms>=3, 'Attesi almeno tre allarmi (sensore opzionale puo aggiungerne altri)'
for role,s in [('tx',tx),('rx',rx)]:
    for key in ('unconfirmed','invalid','send_errors','queue_drops','ack_dropped'):
        assert int(s[key])==0, f'{role}: {key} non zero'
    text=(root/f'{role}-serial.log').read_text(errors='replace')
    assert '# READY cifratura=CCMP' in text and 'FATAL' not in text and 'CONFIG_REQUIRED' not in text, f'{role}: avvio non operativo'
result={'esito':'SUPERATO (prova breve, non qualifica radio)', 'tx':tx,'rx':rx,
        'eventi_correlati':len(confirmed),'allarmi_ricevuti':alarms}
with (root/'analysis.json').open('x') as f:json.dump(result,f,indent=2)
print(json.dumps(result,indent=2))

#!/usr/bin/env python3
"""Upload OTA autenticato, controllo ruolo/MAC e verifica riavvio nello slot alternato."""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import socket
import sys
import time
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
MACS = {'tx': '7C:9E:BD:62:12:94', 'rx': 'D4:D4:DA:E4:3F:A4'}

def validate_image(data, role):
    expected = f'CANTIERE_OTA_ROLE_{role.upper()}_V1'.encode()
    other = f'CANTIERE_OTA_ROLE_{"RX" if role == "tx" else "TX"}_V1'.encode()
    if not data or data[0] != 0xE9 or not 288 < len(data) <= 0x140000:
        raise ValueError('Immagine ESP32 non valida o troppo grande per lo slot')
    if expected not in data or other in data:
        raise ValueError('Immagine con ruolo sbagliato o priva del marcatore OTA')

def status(ip):
    with urllib.request.urlopen(f'http://{ip}/api/device', timeout=3) as response:
        return json.load(response)

def validate_device(info, role):
    if info.get('role') != role.upper() or info.get('mac', '').upper() != MACS[role]:
        raise ValueError('Ruolo/MAC della destinazione non corrispondono alla scheda prevista')
    if not info.get('ota_ready') or info.get('ota_active'):
        raise ValueError('OTA non disponibile o aggiornamento già attivo')

def reject_wrong_password(ip):
    # Challenge only: no TCP transfer and no flash write on correct rejection.
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        sock.settimeout(4)
        sock.connect((ip, 3232))
        sock.send(b'0 45678 1024 d41d8cd98f00b204e9800998ecf8427e\n')
        challenge = sock.recv(128).decode()
        if not challenge.startswith('AUTH '):
            raise RuntimeError('Autenticazione obbligatoria non verificata: '+challenge)
        sock.send(b'200 00000000000000000000000000000000 00000000000000000000000000000000\n')
        reply = sock.recv(128).decode()
        if 'Authentication Failed' not in reply:
            raise RuntimeError('Credenziale errata non rifiutata esplicitamente: '+reply)
    return reply.strip()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--role', choices=MACS, required=True)
    parser.add_argument('--ip', required=True, help='IPv4 o nome risolvibile nella LAN/VPN')
    parser.add_argument('--host-ip', default='0.0.0.0', help='Indirizzo locale di ascolto TCP')
    parser.add_argument('--host-port', type=int, default=45678)
    parser.add_argument('--credentials', type=Path, default=ROOT/'secrets/ota.json')
    parser.add_argument('--check-auth', action='store_true', help='Solo verifica rifiuto password errata')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    ip = socket.gethostbyname(args.ip)
    before = status(ip)
    validate_device(before, args.role)
    if args.check_auth:
        reply = reject_wrong_password(ip)
        after = status(ip)
        if after['boot_id'] != before['boot_id'] or after['partition'] != before['partition']:
            raise RuntimeError('Riavvio inatteso durante test autenticazione')
        result = {'authentication_rejected': reply, 'before': before, 'after': after}
    else:
        image = ROOT/f'espnow_{args.role}/.pio/build/{args.role}/firmware.bin'
        data = image.read_bytes()
        validate_image(data, args.role)
        password = json.loads(args.credentials.read_text())[args.role]
        if len(password) < 24:
            raise ValueError('Credenziale OTA assente o troppo corta')
        spec = importlib.util.spec_from_file_location('espota', ROOT/'.pio-core/packages/framework-arduinoespressif32/tools/espota.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        module.TIMEOUT = 3
        module.PROGRESS = False
        # Password passed in memory, never in command-line arguments or logs.
        module.update_progress = lambda progress: None
        code = module.serve(ip, args.host_ip, 3232, args.host_port, password, str(image), module.FLASH)
        if code:
            raise RuntimeError('Trasferimento OTA fallito; verificare stato e connettività')
        deadline = time.monotonic()+45
        after = None
        while time.monotonic() < deadline:
            time.sleep(1)
            try:
                candidate = status(ip)
                validate_device(candidate, args.role)
                if candidate['boot_id'] != before['boot_id'] and candidate['partition'] != before['partition']:
                    after = candidate
                    break
            except (OSError, ValueError):
                continue
        if after is None:
            raise RuntimeError('Trasferimento completato, ma riavvio nello slot alternato NON verificato')
        result = {'result': 'OTA e riavvio verificati', 'bytes': len(data), 'sha256': hashlib.sha256(data).hexdigest(), 'before': before, 'after': after}
    if args.report:
        with args.report.open('x') as f:
            json.dump(result, f, indent=2)
    print(json.dumps(result, indent=2))

if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        print(f'ERRORE: {error}', file=sys.stderr)
        sys.exit(1)

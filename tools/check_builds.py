#!/usr/bin/env python3
"""Build di entrambi i ruoli anche con configurazione locale temporanea. Mai upload."""
import os
from pathlib import Path
import secrets
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    os.chdir(ROOT)
    configs = [ROOT / f'espnow_{role}/include/config.local.hpp' for role in ('tx', 'rx')]
    if any(p.exists() for p in configs):
        raise SystemExit('Configurazioni locali presenti: non verranno alterate. Compilare con pio run.')
    env = dict(os.environ, PLATFORMIO_CORE_DIR=str(ROOT / '.pio-core'))
    (ROOT / 'build').mkdir(exist_ok=True)
    template = (ROOT / 'shared/config.example.hpp').read_text()
    # Chiavi usa-e-getta, mai stampate; MAC sintetici per sola compilazione.
    for key in ('pmk', 'lmk'):
        template = template.replace(f'{key}[16] = {{0}}', f'{key}[16] = {{' +
                                    ','.join(str(b) for b in secrets.token_bytes(16)) + '}')
    created = []
    failures = []
    def build(role, label):
        with open(ROOT / f'build/build-{role}-{label}.log', 'w') as log:
            result = subprocess.run([str(ROOT / '.venv/bin/pio'), 'run', '-d', f'espnow_{role}', '-e', role],
                                    env=env, stdout=log, stderr=subprocess.STDOUT)
        print(f'{role} {label}: ' + ('SUPERATO' if result.returncode == 0 else 'FALLITO'), flush=True)
        if result.returncode:
            failures.append((role, label))
    try:
        for i, path in enumerate(configs):
            with path.open('x') as file:
                created.append(path)
                file.write(template.replace('{0,0,0,0,0,0}', '{2,0,0,0,0,' + str(i+1) + '}'))
        for role in ('tx', 'rx'):
            build(role, 'configured')
    finally:
        for path in created:
            path.unlink()
        # Leave only default, CONFIG_REQUIRED artifacts, not usable test credentials.
        for role in ('tx', 'rx'):
            build(role, 'default')
    return bool(failures)

if __name__ == '__main__':
    raise SystemExit(main())

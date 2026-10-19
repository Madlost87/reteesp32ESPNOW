# Configurazione locale

1. Identificare le due schede e assegnare esplicitamente i ruoli. Il numero di pin
   non determina chip, capacità flash o pin LED. Usare il MAC **STA**, non SoftAP.
2. Acquisire i MAC con identificazione esptool dopo autorizzazione all'accesso
   hardware, oppure dai log di un primo firmware diagnostico dopo flash autorizzato.
   La guida build/flash descrive questo percorso senza circolarità.
3. Copiare il modello completo (non il piccolo include di rimando):

```sh
cp -n shared/config.example.hpp espnow_tx/include/config.local.hpp
cp -n shared/config.example.hpp espnow_rx/include/config.local.hpp
```

4. Inserire il MAC RX nel file TX e viceversa. IDs predefiniti: TX=1, RX=2;
   `peerNode` deve essere l'ID dell'altro. Configurare lo stesso canale (1..11).
5. Inserire due chiavi casuali da 16 byte, PMK e LMK. Per questa coppia usare
   la stessa PMK e la stessa LMK in entrambi i file. Generare con un generatore
   crittografico locale, per esempio `secrets.token_bytes(16)` di Python in uno
   strumento locale riservato; non incollare le chiavi in chat o nei log.
   Lo script seguente scrive un frammento privato senza stamparle:

```sh
.venv/bin/python - <<'PY'
import os, secrets
path = 'build/keys.private.txt'
os.makedirs('build', exist_ok=True)
fd = os.open(path, os.O_WRONLY | os.O_CREAT | os.O_EXCL, 0o600)
with os.fdopen(fd, 'w') as f:
    for name in ('pmk', 'lmk'):
        f.write('constexpr uint8_t ' + name + '[16] = {' +
                ','.join('0x%02x' % b for b in secrets.token_bytes(16)) + '};\n')
PY
```

Trasferire le due dichiarazioni nei file locali sostituendo quelle a zero.
Non rigenerare separatamente LMK differenti per i due peer. Proteggere accesso
ai file e al PC. `config.local.hpp`, `build/`, `.pio/` e backup sono ignorati:
anche il binario contiene le chiavi e non va pubblicato. Gli esempi tutti a zero
sono rifiutati a runtime; non esiste un ripiego automatico in chiaro.

| Parametro | Default | Limiti/uso |
|---|---:|---|
| channel | 6 | 1..11, identico sui peer; scelta operativa da provare |
| intervalMs | 2000 | 100..86400000 ms, periodo prove senza sensore; telemetria sensore a 250 ms |
| timeoutMs | 250 | 50..60000 ms per attesa ACK |
| retries | 3 | 0..10 ritrasmissioni oltre al primo tentativo |
| summaryMs | 10000 | 1000..86400000 ms |
| periodicAtBoot | true | abilita prove periodiche senza sensore; non disabilita telemetria |
| node / peerNode | 1/2 o 2/1 | interi non nulli distinti |

I parametri sono costanti di build: modificarli richiede ricompilazione e un
nuovo flash autorizzato. `test on/off` modifica solo la generazione nella RAM.
Non si salvano impostazioni o contatori applicativi in flash. Una sola transazione
è attiva alla volta; un comando `alarm` durante BUSY è rifiutato esplicitamente,
non accodato. Con sensore attivo `test off` non ferma la telemetria: se `alarm` è rifiutato per
BUSY, ripeterlo dopo la transazione.


## Configurazione corrente e OTA

Entrambe le schede usano canale 6 e si collegano alla LAN per OTA. I file
`espnow_{tx,rx}/include/network.local.hpp` contengono SSID/password/BSSID;
`ota.local.hpp` contiene l'hash della password OTA specifica del ruolo.
I file locali già presenti non devono essere sovrascritti con esempi vuoti.

Le password OTA distinte sono in `secrets/ota.json` (0600), letto direttamente da
`tools/ota_upload.py`. Credenziali e binari non vanno pubblicati. DHCP rilevato:
TX 192.168.1.117, RX 192.168.1.176. [Procedura OTA e rotazione](aggiornamenti-ota.md).

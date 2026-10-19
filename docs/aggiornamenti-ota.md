# Aggiornamenti OTA — firmware 0.4.0

## Implementazione

`shared/remote_update.hpp` aggiunge ArduinoOTA autenticato e `/api/device` su
entrambe le schede. `shared/lan.hpp` gestisce la connessione comune sul BSSID e
canale configurati. RX continua a servire la dashboard; TX espone solo lo stato
tecnico `/api/device`. Soglia presenza sotto 20 cm, isteresi a 25 cm, ESP-NOW
cifrato e protocollo v2 restano invariati. Il TX ora deve essere raggiungibile dal
router per ricevere OTA; ESP-NOW non trasporta le immagini firmware.

Arduino 2.0.17 usa autenticazione challenge-response MD5 con password casuali
lunghe e differenti tra le schede. Non è TLS né verifica crittografica della firma
del firmware. Usare LAN fidata o VPN; non esporre UDP 3232/HTTP su Internet.
Il controllo ruolo/MAC dello script evita errori operativi, non autentica il server
HTTP e non sostituisce una firma del firmware. Un client OTA alternativo con la
password potrebbe inviare un'immagine del ruolo sbagliato.

## Credenziali e indirizzi

Indirizzi rilevati il 18 ottobre 2026: TX **192.168.1.117**, RX **192.168.1.176**.
Prenotare DHCP per MAC 7C:9E:BD:62:12:94 (TX) e D4:D4:DA:E4:3F:A4 (RX).

- `secrets/ota.json`: password TX/RX; file locale 0600, directory 0700, esclusi da Git.
- `espnow_{tx,rx}/include/ota.local.hpp`: hash password per la rispettiva build.
- `network.local.hpp` in entrambi i progetti: stessa LAN/BSSID/canale 6.
- `config.local.hpp`: MAC e chiavi ESP-NOW preservati.
- Nomi mDNS `cantiere-tx.local`, `cantiere-rx.local`; DHCP può cambiare gli IP.
  Per VPN usare gli IP: mDNS normalmente non attraversa reti instradate.
- `http://IP/api/device`: ruolo, MAC, versione, build, slot, identificativo avvio,
  disponibilità OTA, manutenzione e ultimo errore. Non contiene credenziali.

Conservare i file locali in un backup privato. Una compilazione senza hash valido
non apre un servizio OTA anonimo. Per cambiare password, aggiornare hash locale e
caricare usando la password ancora installata; solo dopo aggiornare la credenziale
nel JSON. Non cancellare le credenziali usate dal firmware precedente prima del flash.

## Procedura dal PC nella LAN o con VPN

Dalla radice del progetto:

```sh
export PLATFORMIO_CORE_DIR="$PWD/.pio-core"
./tools/test.sh
node tests/test_dashboard.js
.venv/bin/pio run -d espnow_tx -e tx
.venv/bin/pio run -d espnow_rx -e rx
python3 tools/ota_upload.py --role tx --ip cantiere-tx.local
python3 tools/ota_upload.py --role rx --ip cantiere-rx.local --host-port 45679
```

Lo script prende solo il `firmware.bin` compilato per quel ruolo, verifica marker,
dimensione e MAC noto, passa la password in memoria al tool Espressif (non negli
argomenti del processo), quindi verifica un nuovo boot e l'avvio nello slot alternato.
Non usa file bootloader, partitions o immagini complete da 4 MB per OTA.
Il successo certifica trasferimento e avvio; la funzionalità dell'allarme va controllata
anche nella dashboard. I marker del binario non sono firme digitali.

Aggiornare una scheda alla volta. Prima verificare il rifiuto di password errata:

```sh
python3 tools/ota_upload.py --role tx --ip cantiere-tx.local --check-auth
python3 tools/ota_upload.py --role rx --ip 192.168.1.176 --check-auth
```

`--report results/nome-univoco.json` conserva l'esito senza segreti. Il file deve
non esistere. `--credentials` permette un JSON privato alternativo.

Rete: il PC invia a UDP 3232 sulla scheda; la scheda apre una connessione TCP
verso il PC sulla porta 45678 (`--host-port` per cambiarla). Firewall/VPN devono
consentire questo ritorno. `--host-ip` seleziona l'indirizzo locale di ascolto;
non risolve da solo NAT o routing. Non è stata configurata una VPN in questa sessione.

## Primo caricamento e manutenzione

Il firmware 0.3.0 non ha un ricevitore OTA: il supporto iniziale va installato via
USB. Identificare MAC e porta prima di usare i comandi:

```sh
.venv/bin/pio run -d espnow_tx -e tx -t upload --upload-port /dev/ttyUSB0
.venv/bin/pio run -d espnow_rx -e rx -t upload --upload-port /dev/ttyUSB1
```

La tabella `default.csv` è esplicita e invariata: app0 a 0x10000 e app1 a 0x150000,
ciascuno 0x140000 byte; partizione otadata a 0xe000. L'OTA scrive lo slot inattivo.
Una scrittura incompleta normalmente lascia selezionato il firmware precedente.
Non interrompere l'alimentazione; non è stato eseguito un test distruttivo di blackout.

Durante il trasferimento `ArduinoOTA.handle()` occupa il loop: campionamento,
DATA/ACK e pubblicazione dashboard sono temporaneamente sospesi. Le code possono
perdere pacchetti; gli eventi di quel periodo non sono garantiti. `/api/device`
espone `ota_active`; la dashboard può mostrare dati vecchi o nodo senza contatto.
Gli aggiornamenti sono quindi manutenzione, non funzionamento ininterrotto dell'allarme.
Il riavvio RX cancella il registro RAM. Le chiavi e la rete restano nella nuova
immagine solo se la si compila con i file locali corretti.

## Errori e recupero

- Password errata: nessuna scrittura, correggere credenziale; errore OTA 0.
- Nessuna risposta: verificare Wi-Fi, IP, canale 6, VPN/firewall e disponibilità OTA.
- Trasferimento fallito: controllare `/api/device` e riprovare; conservare il log.
- Trasferimento riuscito ma nessun riavvio verificato: non dichiararlo concluso;
  controllare IP/DHCP, boot e funzionalità prima di aggiornare l'altra scheda.
- Firmware avviabile ma difettoso: se OTA è raggiungibile, ricompilare la versione
  funzionante con supporto OTA e le stesse credenziali, poi ricaricarla.
- Firmware che non avvia Wi-Fi/OTA o password persa: ripristino USB. Lo stack attuale
  non abilita rollback automatico dopo un boot difettoso. Due slot non lo garantiscono.
- Non modificare tabella partizioni, bootloader o dimensione flash tramite questa procedura.

Riferimenti: implementazioni locali ArduinoOTA/espota del pacchetto Arduino 2.0.17;
[partizioni Espressif](https://docs.espressif.com/projects/arduino-esp32/en/latest/tutorials/partition_table.html)
e [OTA IDF](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/ota.html).

## Esito hardware del 18 ottobre 2026

- Identità USB riconfermate; installazione iniziale 0.4.0 riuscita su entrambe.
- Password errata rifiutata da entrambe con `Authentication Failed`, senza cambio
  boot o partizione: `results/ota-v040/auth-tx.json` e `auth-rx.json`.
- Caricamento remoto autenticato TX (844656 byte) riuscito, avvio app0 → app1.
- Caricamento remoto autenticato RX (856352 byte) riuscito, avvio app0 → app1.
- Entrambi gli aggiornamenti hanno ricaricato il binario 0.4.0 appena installato:
  è una prova reale di trasferimento, scrittura e selezione del nuovo slot.
- Report con SHA-256, identità e boot prima/dopo: `results/ota-v040/ota-tx.json`
  e `ota-rx.json`. Credenziali escluse dai report.
- Test host passati: protocollo, sensore, CSV, destinazione OTA/ruolo/dimensione;
  test JavaScript dashboard passato. Build TX/RX senza avvisi di compilazione.

Verificato nella LAN. Accesso tramite VPN/Internet e interruzione alimentazione
non provati. Il rilevamento fisico del sensore non è dimostrato da questi test OTA.

Prova finale dopo reset hardware di entrambe: 253/253 messaggi ESP-NOW confermati
al primo tentativo, tre allarmi **simulati**, 182 richieste HTTP riuscite, risposta
media 67,53 ms e massima 486,40 ms. Nessun timeout/errore/overflow nei riepiloghi.
Entrambe le schede restano in app1 dopo il reset; dashboard confrontata col sorgente.
Log e analisi in `results/ota-v040/live/`, stato finale in `final-devices.json`.
Nessuna distanza valida nel campione: il problema di eco assente rimane distinto
dall'OTA e la soglia fisica non è stata validata con un ostacolo reale.

Se il PC segnala `Listen Failed` dopo un upload, la porta TCP può essere ancora
occupata: usare `--host-port 45679` (consentita nel firewall) oppure attendere che
la porta precedente sia libera. Il mancato ascolto non avvia una scrittura sulla scheda.

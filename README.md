# Allarme cantiere — fase 1 ESP-NOW

Prototipo tra **due ESP32 classici** con collegamento ESP-NOW. La centrale RX è ora
collegata alla LAN Wi-Fi per servire la pagina locale: **http://192.168.1.176/**.
Non richiede Internet o LoRa. [Guida centrale web](docs/centrale-web.md).
TX invia telemetria ogni 250 ms e transizioni del sensore; RX risponde con una conferma applicativa.
Il prototipo non è un impianto di allarme certificato. Portata, autonomia e affidabilità
restano da misurare nel cantiere. Il TX include il sensore ultrasuoni su TRIG G5 / ECHO G18; nessuna sirena viene pilotata.

## Messaggi testuali tra le schede — 0.6.0

- **TX:** http://192.168.1.117/
- **RX:** http://192.168.1.176/radio

Scrivi un testo nel campo e premi **Invia messaggio**: appare nel riquadro ricevuti
dell'altra ESP32, da cui puoi rispondere. Fino a 48 caratteri; ultimo testo inviato,
ultimo ricevuto e conferma applicativa visibili su ogni pagina. Protocollo v3.
[Uso e verifica](docs/prova-messaggi.md).
Diagnosi sensore chiusa su richiesta come **non risolta**; sensore disabilitato.

## Aggiornamento remoto

OTA autenticato disponibile su entrambe le schede: [procedura, credenziali e recupero](docs/aggiornamenti-ota.md).
Primo caricamento USB completato. Aggiornamenti successivi dalla LAN/VPN:
TX `192.168.1.117`, RX `192.168.1.176`.

## Avvio rapido sul PC

Dalla radice del progetto, Linux con Python 3 e g++:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r tools/requirements-lock.txt
export PLATFORMIO_CORE_DIR="$PWD/.pio-core"
./tools/test.sh
.venv/bin/pio run -d espnow_tx -e tx
.venv/bin/pio run -d espnow_rx -e rx
```

I primi download richiedono Internet **sul PC di sviluppo**, non nel cantiere.
I firmware compilano anche senza configurazione locale: in quel caso stampano
il MAC STA e `CONFIG_REQUIRED`, senza iniziare scambi ESP-NOW.

## Cronologia verifiche precedenti — 18 ottobre 2026

TX esegue **0.4.1** (diagnostica sensore), RX **0.4.0**, protocollo v2 a 48 byte. Primo caricamento
USB e successivo caricamento OTA autenticato completati su ciascuna; password
errata rifiutata e avvio nello slot alternativo verificato anche dopo reset.
Prova finale: **253/253 messaggi confermati**, **182 richieste HTTP riuscite**.

Il sensore usa **TRIG G5 / ECHO G18**: presenza dopo tre campioni consecutivi
sotto **20 cm**, rimozione dopo tre ad almeno **25 cm**. Durante le prove non è
arrivata alcuna distanza valida: **il rilevamento fisico non è ancora verificato**.
Diagnostica TX: 156 cicli TRIG, zero fronti ECHO rilevati, livello G18 basso nelle
letture. [Contatori e interpretazione](docs/sensore-ultrasuoni.md).
I tre eventi nel test sono **allarmi simulati da comando seriale**, non rilevamenti
fisici. Sirena e inserimento/disarmo non sono implementati.

La dashboard è sulla centrale: **http://192.168.1.176/** (e l'API su **http://192.168.1.176/api/status**). Gli IP sono DHCP.
Per aggiornare, dopo aver compilato il rispettivo firmware:

```sh
python3 tools/ota_upload.py --role tx --ip cantiere-tx.local
python3 tools/ota_upload.py --role rx --ip cantiere-rx.local --host-port 45679
```

Le credenziali vengono lette da `secrets/ota.json`, escluso da Git. Usare LAN fidata
o VPN; VPN non configurata né provata in questa sessione. Durante OTA il servizio
sulla scheda aggiornata è sospeso. [Procedura e recupero](docs/aggiornamenti-ota.md).
[Stato e cronologia delle prove](docs/stato.md).

## File e guide

- `espnow_tx/`, `espnow_rx/`: progetti indipendenti con dipendenza relativa da `shared/`.
- `shared/protocol.hpp`: wire format, transazioni, deduplicazione e metriche, senza Arduino.
- `shared/firmware.hpp`: adattatore ESP-NOW, code, seriale; selezione del ruolo a build.
- [Configurazione](docs/configurazione.md): MAC, chiavi locali e parametri.
- [Architettura/protocollo](docs/architettura.md) e [sicurezza/limiti](docs/sicurezza-limiti.md).
- [Seriale e CSV](docs/seriale-csv.md), [prove sul campo](docs/prove-campo.md),
  [scheda risultati](docs/scheda-risultati.md), [problemi](docs/troubleshooting.md).
- [Specifiche](specs/requisiti.md), [matrice requisiti/test](tests/matrice.md),
  [stato e verifiche](docs/stato.md), [fonti ufficiali](docs/fonti.md).

Dopo configurazione e flash autorizzato, sul TX usare `test off` per fermare
le prove periodiche (telemetria sensore sempre attiva), `alarm` per un allarme simulato, `stats` per il riepilogo.
Inviare i comandi con newline. RX accetta `stats` e `help`.

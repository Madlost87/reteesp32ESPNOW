# Compilazione, flash USB e aggiornamenti remoti

Firmware corrente 0.4.0 già installato su entrambe le schede. Per aggiornamenti
ordinari usare [OTA autenticato](aggiornamenti-ota.md); questa guida descrive anche
primo caricamento e recupero USB. Le porte USB possono cambiare: riconfermare i MAC.

## Ambiente riproducibile

Dalla radice, Python 3 (verificato 3.14.8), g++ e rete per i download iniziali:

```sh
python3 -m venv .venv
.venv/bin/python -m pip install -r tools/requirements-lock.txt
export PLATFORMIO_CORE_DIR="$PWD/.pio-core"
.venv/bin/pio --version
./tools/test.sh
.venv/bin/pio run -d espnow_tx -e tx
.venv/bin/pio run -d espnow_rx -e rx
```

Core 6.1.18, piattaforma 6.9.0 e pacchetti esatti nel file comune. Un eventuale
avviso su un Core globale più recente non richiede di modificarlo: usare `.venv/bin/pio`.
I progetti devono conservare la posizione relativa a `shared/` e al file INI comune.
La build di esempio esegue tutta la compilazione ma blocca a runtime gli scambi
finché MAC/chiavi non sono validi. Gli artefatti sono sotto `<progetto>/.pio/build/<env>/`.

## Identificazione prima del flash

Il solo profilo `esp32dev` è **provvisorio per compilare**. Include impostazioni
4 MB/DIO/40 MHz e partizioni standard, non una rilevazione dell'hardware. Non
trasformare il risultato della build in una conferma di compatibilità della scheda.

Per ciascuna scheda raccogliere modello/serigrafia, modulo, numero pin, chip reale,
revisione, USB-UART, porta stabile, dimensione/ID flash e alimentazione. Associare
fisicamente etichette A/B ai ruoli TX/RX. Se non è ESP32 classico, fermarsi e adattare
il progetto; non selezionare ESP32-S3 o ESP8266 per approssimazione.

Le operazioni esptool possono resettare il dispositivo, pur senza scrivere flash.
Gli esempi seguenti sono istruzioni per la futura sessione hardware, non operazioni
già eseguite. Collegare una scheda alla volta e verificare la porta:

```sh
.venv/bin/pio device list
# Sostituire la porta soltanto dopo averla identificata.
export PORT_TX=/dev/serial/by-id/IDENTIFICATIVO_REALE_TX
export PORT_RX=/dev/serial/by-id/IDENTIFICATIVO_REALE_RX
.venv/bin/python .pio-core/packages/tool-esptoolpy/esptool.py --port "$PORT_TX" chip_id
.venv/bin/python .pio-core/packages/tool-esptoolpy/esptool.py --port "$PORT_TX" flash_id
.venv/bin/python .pio-core/packages/tool-esptoolpy/esptool.py --port "$PORT_TX" read_mac
.venv/bin/python .pio-core/packages/tool-esptoolpy/esptool.py --port "$PORT_RX" chip_id
.venv/bin/python .pio-core/packages/tool-esptoolpy/esptool.py --port "$PORT_RX" flash_id
.venv/bin/python .pio-core/packages/tool-esptoolpy/esptool.py --port "$PORT_RX" read_mac
```

Controllare MAC STA nei log dopo l'eventuale caricamento. In alternativa, se non si
possono ottenere prima i MAC, il firmware di esempio può essere caricato solo dopo
richiesta esplicita e verifica del profilo: stamperà il MAC senza scambi. Poi si
configura il peer e si ricompila per un secondo caricamento autorizzato.

Verificare dimensione flash, modo/frequenza supportati e partizioni: se differiscono
dal profilo generico, modificare il singolo progetto in base ai dati confermati e
ricompilare. Nessun pin LED, pulsante, sensore o LoRa è assegnato dal nostro codice.

## Backup da valutare prima della sostituzione

Un flash sostituisce il firmware attualmente presente. Comunicare sempre quale
immagine verrà caricata e su quale porta; controllare anche secure boot/flash encryption
se presenti. Una lettura può essere disabilitata o produrre contenuti cifrati; non
promettere che sia ripristinabile senza verificarne le condizioni.

Esempio per un nuovo backup, dopo aver impostato la capacità effettivamente rilevata in byte:

```sh
mkdir -p backups
# FLASH_BYTES_TX e FLASH_BYTES_RX devono derivare da flash_id, non dal profilo di build.
.venv/bin/python .pio-core/packages/tool-esptoolpy/esptool.py --port "$PORT_TX" read_flash 0 "$FLASH_BYTES_TX" backups/tx-prima.bin
.venv/bin/python .pio-core/packages/tool-esptoolpy/esptool.py --port "$PORT_RX" read_flash 0 "$FLASH_BYTES_RX" backups/rx-prima.bin
sha256sum backups/tx-prima.bin backups/rx-prima.bin
```

Usare nomi nuovi per ogni backup e confrontare dimensione e hash di una seconda
lettura se possibile. Proteggere i backup: possono contenere segreti del vecchio firmware.

## Scheda A destinata a TX — solo dopo richiesta esplicita

Verificare configurazione locale, profilo e `PORT_TX`. Comunicare:
“Caricherò espnow_tx, ambiente tx, su [porta e scheda identificate]; il firmware
attuale verrà sostituito”. Quindi, solo con autorizzazione già ricevuta:

```sh
.venv/bin/pio run -d espnow_tx -e tx -t upload --upload-port "$PORT_TX"
.venv/bin/pio device monitor -p "$PORT_TX" -b 115200 --dtr 0 --rts 0
```

## Scheda B destinata a RX — solo dopo richiesta esplicita

Ripetere la verifica indipendente, soprattutto perché il modello della seconda
scheda è incerto. Comunicare immagine RX e porta effettiva, poi:

```sh
.venv/bin/pio run -d espnow_rx -e rx -t upload --upload-port "$PORT_RX"
.venv/bin/pio device monitor -p "$PORT_RX" -b 115200 --dtr 0 --rts 0
```

Non eseguire cancellazioni complete preventive. Non è implementato OTA.
L'autorizzazione alla compilazione non autorizza flash, reset o sostituzioni hardware.

## Verifica aggiuntiva della configurazione locale (solo PC)

Con entrambi i `config.local.hpp` assenti, `python3 tools/check_builds.py` compila
TX/RX con MAC sintetici e chiavi temporanee non stampate, elimina solo i file creati
dallo script e ricompila gli esempi di default. Se trova configurazioni locali si
ferma senza modificarle. Non apre seriali né usa target upload. I log sono in `build/`.
Un'interruzione forzata del processo può impedire il cleanup: in tal caso controllare
i file locali e ricompilare la propria configurazione prima di qualsiasi futuro deploy.

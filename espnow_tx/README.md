# Trasmettitore TX

Versione corrente **0.6.0**, sensore disabilitato. Pagina messaggi: http://192.168.1.117/.
[Testo libero bidirezionale e conferme ESP-NOW](../docs/prova-messaggi.md).
Le note sulle precedenti versioni sotto sono storiche.


Dalla radice: `PLATFORMIO_CORE_DIR="$PWD/.pio-core" .venv/bin/pio run -d espnow_tx -e tx`.
Dipende da `../shared/` e `../platformio-common.ini`; non spostare la sola cartella.
Configurazione: copiare `../shared/config.example.hpp` in `include/config.local.hpp`,
inserire MAC STA RX, chiavi condivise e parametri come nella [guida](../docs/configurazione.md).
Seriale 115200: `help`, `stats`, `test on`, `test off`, `alarm`, tutti con newline.
Firmware di esempio senza segreti: compila ma richiede configurazione per comunicare.

[Identificazione, backup, flash TX e monitor](../docs/build-flash.md).
Flash soltanto su richiesta esplicita; il profilo esp32dev non conferma il modello reale.

## Stato attuale e OTA

Firmware 0.6.0, protocollo v3. Diagnostica su `/api/sensor`. TX: MAC `7C:9E:BD:62:12:94`.
Indirizzo DHCP; `http://IP/api/device` espone ruolo, versione, slot e stato OTA.
Rete e credenziali OTA locali in `include/network.local.hpp` e `include/ota.local.hpp`;
non sovrascriverle con i modelli di esempio se la scheda è già configurata.

Dalla radice, dopo la compilazione:

```sh
python3 tools/ota_upload.py --role tx --ip cantiere-tx.local
```

[OTA: autenticazione, accesso LAN/VPN e recupero USB](../docs/aggiornamenti-ota.md).
Durante l'aggiornamento il servizio della scheda è temporaneamente sospeso.

TRIG G5, ECHO G18; allarme sotto 20 cm, rimozione ad almeno 25 cm.
`test off` non ferma sensore né telemetria. Eco assente nelle prove: rilevamento
fisico non verificato; `alarm` genera esclusivamente una simulazione.

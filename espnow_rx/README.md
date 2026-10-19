# Ricevitore RX

Versione corrente **0.6.0**, sensore disabilitato. Pagina messaggi: http://192.168.1.176/radio.
[Testo libero bidirezionale e conferme ESP-NOW](../docs/prova-messaggi.md).
Le note sulle precedenti versioni sotto sono storiche.


Dalla radice: `PLATFORMIO_CORE_DIR="$PWD/.pio-core" .venv/bin/pio run -d espnow_rx -e rx`.
Dipende da `../shared/` e `../platformio-common.ini`; non spostare la sola cartella.
Configurazione: copiare `../shared/config.example.hpp` in `include/config.local.hpp`,
inserire MAC STA TX, chiavi condivise e parametri come nella [guida](../docs/configurazione.md).
Seriale 115200: `help`, `stats`, entrambi con newline. RX risponde automaticamente ai DATA.
Firmware di esempio senza segreti: compila ma richiede configurazione per comunicare.

[Identificazione, backup, flash RX e monitor](../docs/build-flash.md).
Flash soltanto su richiesta esplicita; riconfermare MAC e porta prima di caricare.

## Stato attuale e OTA

Firmware 0.6.0, protocollo v3. RX: MAC `D4:D4:DA:E4:3F:A4`.
Indirizzo DHCP; `http://IP/api/device` espone ruolo, versione, slot e stato OTA.
Rete e credenziali OTA locali in `include/network.local.hpp` e `include/ota.local.hpp`;
non sovrascriverle con i modelli di esempio se la scheda è già configurata.

Dalla radice, dopo la compilazione:

```sh
python3 tools/ota_upload.py --role rx --ip cantiere-rx.local --host-port 45679
```

[OTA: autenticazione, accesso LAN/VPN e recupero USB](../docs/aggiornamenti-ota.md).
Durante l'aggiornamento il servizio della scheda è temporaneamente sospeso.

Dashboard: **http://192.168.1.176/**. Registro RAM di 16 eventi, cancellato dal
riavvio. Gli eventi simulati sono distinti da presenza e rimozione del sensore.

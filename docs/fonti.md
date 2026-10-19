# Versioni e fonti ufficiali

Consultate il 17 ottobre 2026. La scelta intenzionale è Arduino-ESP32 **2.0.17**
(basato su IDF **4.4.7**), platform-espressif32 **6.9.0**, Core PlatformIO **6.1.18**.
Non usare callback Arduino 3.x/IDF 5.x con questi sorgenti. Non si afferma che questa
sia la versione più recente o già idonea a un prodotto di sicurezza definitivo.

- [Release PlatformIO 6.9.0](https://github.com/platformio/platform-espressif32/releases/tag/v6.9.0)
  e [manifest bloccato](https://raw.githubusercontent.com/platformio/platform-espressif32/v6.9.0/platform.json):
  pacchetto Arduino 3.20017.0 e toolchain Xtensa 8.4.0+2021r2-patch5.
- [API ESP-NOW IDF 4.4.7](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/network/esp_now.html):
  inizializzazione, peer unicast, PMK/LMK di 16 byte, callback brevi e ACK applicativi.
  La conferma del livello MAC non garantisce elaborazione applicativa.
- [Header ufficiale esp_now.h](https://raw.githubusercontent.com/espressif/esp-idf/v4.4.7/components/esp_wifi/include/esp_now.h):
  `recv(const uint8_t*, const uint8_t*, int)` e `send(const uint8_t*, esp_now_send_status_t)`.
  Non è presente RSSI nel callback di ricezione di questa versione.
- [Header Wi-Fi nel core Arduino scelto](https://raw.githubusercontent.com/espressif/arduino-esp32/2.0.17/tools/sdk/esp32/include/esp_wifi/include/esp_wifi.h):
  selezione del canale, MAC STA e disabilitazione del power saving.
- [Profilo esp32dev](https://raw.githubusercontent.com/platformio/platform-espressif32/v6.9.0/boards/esp32dev.json):
  contiene assunzioni di compilazione tra cui flash 4 MB, CPU 240 MHz e flash DIO 40 MHz.
  Queste NON sono misure delle due schede. Verificarle prima di qualsiasi flash.

Nessuna libreria radio di terzi. Versioni dirette bloccate nel file comune; dipendenze
Python installate registrate in `tools/requirements-lock.txt` quando disponibili.
La riproducibilità dei sorgenti/configurazione non equivale a identità byte per byte
dei binari (timestamp, percorsi di build e ambiente possono differire).

- [RNG IDF 4.4.7](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/system/random.html):
  inizializzare la radio prima di generare la sessione per avere la sorgente di entropia RF.
- [Comandi esptool v4](https://docs.espressif.com/projects/esptool/en/release-v4/esp32/esptool/basic-commands.html):
  identificazione MAC/flash e lettura di backup; `elf2image` costruisce il binario sul PC
  e non esegue un flash sul dispositivo.

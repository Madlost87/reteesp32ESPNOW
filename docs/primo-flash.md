# Primo flash autorizzato — 17 ottobre 2026

Richiesta esplicita dell'utente: rilevare l'ESP32 ed eseguire il primo flash.
Un solo adattatore USB-UART rilevato; assegnato il ruolo TX per questo primo avvio.

| Dato | Rilevazione |
|---|---|
| Porta al momento della prova | `/dev/ttyUSB0` |
| USB-UART | Silicon Labs CP2102, VID:PID 10C4:EA60, seriale 0001 |
| Chip | ESP32-D0WDQ6, revisione v1.0, dual core, 240 MHz |
| Quarzo | 40 MHz |
| MAC STA | 7C:9E:BD:62:12:94 |
| Flash | 4 MB, manufacturer d8, device 4016 |
| Secure boot / flash encryption | Entrambi disattivati, lettura con driver esptool |
| Profilo caricato | esp32dev, Arduino 2.0.17, DIO/40 MHz |
| Firmware | espnow_tx, ambiente tx, versione 0.1.0, nodo 1, canale 6 |
| Configurazione | Esempio: CONFIG_REQUIRED, nessun peer/chiave operativa |

Il modello commerciale/serigrafia della DevKit non è determinabile dal solo chip;
resta da confermare, così come la seconda scheda. Non è stata presunta alcuna piedinatura.
Il comando get_security_info non è supportato su questo chip; letti invece i flag
tramite le API del driver ESP32 di esptool, senza scrivere eFuse.

## Backup e caricamento

Backup completo leggibile di 4194304 byte:
`backups/esp32-7c9ebd621294-first-flash/original.bin` (directory privata, esclusa da Git).
Confronto esptool `verify_flash`: **verify OK (digest matched)** prima del caricamento.
SHA-256: `aa9b84dbd6ffca69ccd7f41f87470d85cc5549f43ea06046f895f334ed0f2a2d`.
La verifica conferma corrispondenza dei dati letti; un ripristino non è stato provato.

Build e upload TX: **SUCCESS**; esptool ha verificato gli hash dei dati scritti.
SHA-256 firmware.bin: `c5b9a2074b7290ed655dea396660e6913ce3478d734456ebaa41c47438aacf7d`.
Nessuna cancellazione completa, OTA o scrittura eFuse. La normale scrittura ha
sostituito bootloader, tabella partizioni, boot_app0 e applicazione nei settori necessari.

## Verifica effettivamente eseguita

Riavvio e acquisizione seriale a 115200 per circa 12 secondi: boot DIO corretto,
firmware TX avviato, MAC/canale visibili, nessun FATAL nel campione.
`help` e `stats` hanno risposto; riepilogo periodico a 10 s presente.
Estrazione CSV dal log reale riuscita senza righe scartate.

Risultato atteso osservato: `CONFIG_REQUIRED`, radio ESP-NOW non avviata.
Non è ancora una prova di collegamento radio; nessun DATA o ACK inviato.

Log locali: `results/first-flash-7c9ebd621294/upload.log`, `serial.log`,
`tx-events.csv`, `tx-summary.csv` (esclusi da Git).

H00: identificazione elettronica/backup/flash TX completati, inventario visivo e RX pendenti.
H01: verificato solo avvio/seriale TX, prova 1000 messaggi non eseguita.
H02: verificati help/stats TX, allarmi/periodico radio non eseguiti.
Tutti gli altri casi radio e le fasi future restano nello stato precedente.

Prossimo passo: collegare e identificare il secondo ESP32, assegnarlo a RX,
configurare i MAC reciproci e chiavi locali, quindi caricare i firmware configurati
su nuova richiesta. L'autorizzazione al primo flash non implica caricamenti futuri.

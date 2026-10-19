# Matrice requisiti / verifiche

Test host eseguiti con `./tools/test.sh` sullo stesso `shared/protocol.hpp` del firmware.
Esiti correnti in [stato](../docs/stato.md). Una prova host superata non sostituisce
la corrispondente prova hardware. H*=**non eseguito, hardware da fare**; P*=**pianificato**.
R*=revisione statica; B*=compilazione. T*=automatico host.

| Requisito | Test/caso | Evidenza e criterio |
|---|---|---|
| F1-01 | B-TX, B-RX, R01 | Build due ambienti; dipendenza dalla stessa libreria e versioni INI |
| F1-02 | R01, H00 | Target guard e nessun GPIO; identificazione reale/flash compatibile per entrambe |
| F1-03 | R01, H01 | Solo STA e canale fisso; 1000 originali offline vicini |
| F1-04 | H00, H01 | Log boot completo, MAC STA confrontato e peer configurato |
| F1-05 | T01, H05 | Roundtrip/boundary/magic/versione/campi/peer errato |
| F1-06 | T03, T06, H01, H07 | ACK errati ignorati, MAC separato, risposta persa recuperata |
| F1-07 | T04, T06, H03, H05 | Scadenza esatta, zero e dieci retry, limite invii rispettato |
| F1-08 | T02, T06, H07 | Finestra 64, riordino, duplicato, stale, espulsione quattro sessioni |
| F1-09 | T02, T03, H04 | Nuove sessioni TX/RX, vecchi ACK, cache persa dichiarata |
| F1-10 | T05, H01, H09 | RTT/varianza/rollover; valori reali da acquisire |
| F1-11 | R01, H10 | Callback/code bounded, contatori perdita; errori/fatal osservabili |
| F1-12 | H02 | Comandi periodico/allarme e comportamento BUSY, nessun GPIO |
| F1-13 | T07, R01, H01 | Due test Python estrazione/rumore/troncamento, parametri compile-time |
| F1-14 | T04, T05, T06, R02, H07 | Denominatori e metriche; perdita ACK non equivale a perdita DATA |
| F1-15 | R01, R02, H05, H06 | Esempi zero bloccati, chiavi ignorate da VCS, peer cifrato, incompatibilità |
| F1-16 | T02, R02 | Limiti cache/replay/RSSI/mesh/portata espliciti |
| F1-17 | R02, H01–H10 | Procedure/scheda, dati e durata registrati senza risultati fittizi |
| F1-18 | R03, H00 | Nessun deploy eseguito; checklist futuro e comandi separati |
| F1-19 | T01–T07, B-TX, B-RX, R02 | Test codice condiviso, guide/matrice/stati coerenti |
| F2-01 | P201 | Tentare join sconosciuto, autorizzare, poi revocare; audit corretto |
| F2-02 | P202 | Comandi validi/invalidi/scaduti nei diversi stati, nessun disarmo improprio |
| F2-03 | P203 | Stimolare separatamente quattro classi evento, verificare centrale/sirena/registro |
| F2-04 | P204 | Spegnere nodo per L, verificare allerta e recupero heartbeat |
| F2-05 | P205 | Perdere ACK, duplicare evento, interrompere alimentazione durante commit |
| F2-06 | P206 | Scollegare casa/Internet/LoRa e provocare intrusione; misurare latenza locale |
| F2-07 | P207 | Misurare consumi e autonomia A, distacco rete/ritorno, soglia batteria |
| F2-08 | P208 | Replay prima/dopo reset e revoca/rotazione chiavi, nessuna accettazione indebita |
| F2-09 | P209 | Saturare C, fault injection scritture, controllo budget W e corruzione |
| F2-10 | P210 | Manutenzione autorizzata e negata, aggiornamento interrotto, recupero |
| F2-11 | P211 | Campagna radio finale su tutti i nodi, soglie approvate e ripetizioni |
| F3-01 | P301 | Revisione hardware/radio normativa/pin con dati reali prima di cablare |
| F3-02 | P302 | Perdite su ogni tratta, coda piena, duplicati; nessun falso ACK destinazione |
| F3-03 | P303 | Ponte spento/compromesso, replay e misura tratta reale; centrale autonoma |
| F4-01 | P401 | Integrazione scelta, riconnessione e disponibilità coerenti |
| F4-02 | P402 | Spegnere Internet/broker/HA, simulare evento, recuperare senza blocco locale |
| F4-03 | P403 | Identità non autorizzata, token revocato/scaduto, comando ripetuto; stato invariato |

## Identificativi di test automatici

T01 `validation`; T02 `dedup`; T03 `acknowledgements`; T04 `retries`;
T05 `metrics`; T06 `simulation` in `test_protocol.cpp`.
T07 `CsvTests` in `test_csv.py` (due casi). UBSan abilitato, assert attivi.

R01 revisione implementazione/configurazione; R02 revisione guide e tracciabilità;
R03 verifica delle operazioni eseguite nella sessione. Non sono certificazioni né
prove automatiche di sicurezza. La revisione della regola flash non sostituisce
l'identificazione obbligatoria al momento dell'effettivo deploy.

## Regressioni della correzione 0.1.1

F1-13/F1-14: `FirmwareOutputTests.test_real_formatter_and_pending_sensor` verifica
il serializzatore effettivamente incluso nel firmware (28 campi e valori), attraverso
`test_features.cpp`; non usa una ricostruzione parallela del formato.
Il medesimo eseguibile verifica la logica condivisa del sensore opzionale durante BUSY,
accettazione, cooldown e rollover. Questa verifica non certifica F2-03 né il driver ISR.
Build standard TX/RX e build separata sensore abilitato verificano entrambi i rami.

## Estensione centrale web richiesta successivamente

| Caso | Verifica | Esito |
|---|---|---|
| WEB01 connessione LAN e server sul dispositivo | MAC RX riconfermato, upload 0.2.1, WEB_READY e GET / dalla LAN | SUPERATO, results/web-v021 |
| WEB02 stato e registro RAM | test_features.cpp: finestra 16 eventi, ordine, limite contatto 10 s; API finale tre allarmi | SUPERATO |
| WEB03 convivenza HTTP/ESP-NOW | 33 originali più polling HTTP, correlazione log TX/RX | SUPERATO dopo correzione power saving; prima prova 0.2.0 FALLITA |
| WEB04 interfaccia e dati obsoleti | node tests/test_dashboard.js: stati normale/allarme/offline/errore HTTP; HTML sorgente confrontato con risposta | SUPERATO funzionale; verifica visiva browser NON ESEGUITA |
| WEB05 recupero dopo perdita router/cambio canale | Spegnere AP e verificare recupero e contatori | NON ESEGUITO |

## Aggiornamento 0.3.0

Protocollo v2, validazione distanza/età/stato; filtro tre campioni e isteresi;
conservazione transizioni durante BUSY, overflow, rollover; stato corrente resistente
al riordino; dashboard presenza/rimozione, eco assente e dati obsoleti: test host superati.
Build e flash TX/RX riusciti. Prova simultanea: 253/253 conferme e 199 HTTP riuscite.
Risposta fisica sensore NON VALIDATA (eco assente). Vedere docs/fluidita-v030.md.

## OTA 0.4.0

| Caso | Verifica | Esito |
|---|---|---|
| OTA01 autenticazione obbligatoria TX/RX | Challenge e password errata, boot/slot invariati | SUPERATO su entrambe |
| OTA02 trasferimento e boot TX | Upload autenticato, app0 → app1 e nuovo boot_id | SUPERATO |
| OTA03 trasferimento e boot RX | Upload autenticato, app0 → app1 e nuovo boot_id | SUPERATO |
| OTA04 ruolo/dimensione/identità destinazione | tests/test_ota.py | SUPERATO |
| OTA05 blackout e rollback automatico | Nessuna prova di blackout; rollback automatico non abilitato | NON ESEGUITO / NON IMPLEMENTATO |
| OTA06 accesso VPN fuori LAN | Nessuna VPN configurata | NON ESEGUITO |

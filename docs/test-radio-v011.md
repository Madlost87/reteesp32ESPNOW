# Flash e prova radio 0.1.1 — 17 ottobre 2026

Eseguiti su richiesta esplicita dell'utente «flash e test».
Identità riconfermate prima di scrivere:
- TX: /dev/ttyUSB0, MAC 7C:9E:BD:62:12:94, ESP32-D0WDQ6 rev. 1.0, 4 MB.
- RX: /dev/ttyUSB1, MAC D4:D4:DA:E4:3F:A4, ESP32-D0WD-V3 rev. 3.1, 4 MB.

Entrambi gli upload SUCCESS, hash delle regioni scritte verificati da esptool.
Backup originali già disponibili; nessuna cancellazione completa e nessuna scrittura eFuse.
Configurazione: canale 6, cifratura CCMP, intervallo 2000 ms, timeout 250 ms, 3 retry.
Sensore TX abilitato: TRIG G5 / ECHO G18. Nessuna chiave riportata in questo documento.

## Risultato: SUPERATO, prova breve

Acquisizione seriale simultanea di circa 77 secondi con riavvio dei due nodi.
30 prove periodiche più 3 allarmi da comando seriale: **33/33 confermati al primo tentativo**.
RX ha registrato gli stessi 33 eventi; correlazione sessione/sequenza/tipo verificata.

| Misura TX | Valore |
|---|---:|
| Originali / tentativi / conferme | 33 / 33 / 33 |
| Retry / timeout / senza conferma | 0 / 0 / 0 |
| Duplicati / invalidi / errori invio / overflow | 0 / 0 / 0 / 0 |
| RTT campioni | 33 |
| RTT medio / deviazione standard campionaria | 6.697 / 4.326 ms |
| RTT min / max | 3 / 13 ms |

RX: 33 originali, 33 ACK accodati, nessun errore MAC, overflow o duplicato.
Entrambi hanno stampato READY cifratura=CCMP, nessun FATAL nel campione.
CSV estratti senza righe scartate. Test host ripetuti con esito positivo.

I tre allarmi sono stati simulati via seriale: questa prova NON verifica misura della
distanza, cablaggio elettrico o risposta del sensore a un ostacolo reale.
Distanza/orientamento/ostacoli fra le schede non rilevati dall'agente.
Non completa H01 (1000 messaggi), H02 (20 allarmi) o le prove di guasto/portata/durata.

Al termine `test off` è rimasto attivo in RAM sul TX: generazione periodica fermata,
sensore ancora abilitato. Un riavvio riattiva il periodico previsto dalla configurazione.

## Evidenze locali

`results/flash-test-v011/`: upload-tx.log, upload-rx.log, host-tests.log,
tx-serial.log, rx-serial.log, CSV eventi/riepiloghi e analysis.json.

## HTTP

Non è implementato alcun server o pagina HTTP. RX è un risponditore ESP-NOW con
log seriale, non ancora una centrale web. Un'interfaccia offline richiederebbe
un'estensione esplicita, ad esempio AP locale della centrale e server HTTP;
non è stata aggiunta in questa sessione, che mantiene la comunicazione ESP-NOW senza AP.

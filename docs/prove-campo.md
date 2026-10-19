# Piano delle prove hardware e sul campo

Le prove brevi di banco, HTTP e OTA sono documentate in [stato](stato.md) e
[OTA](aggiornamenti-ota.md). Non equivalgono al completamento della matrice H00–H10
sotto. Portata, blackout, autonomia e rilevamento fisico restano da qualificare.

Le procedure storiche basate su periodico a 2 s e arresto con `test off` richiedono
una build di prova senza sensore (`ENABLE_ULTRASONIC=0`). Nel firmware corrente
il sensore invia telemetria ogni 250 ms anche dopo `test off`; usare acquisizioni
con finestra temporale e considerare le transazioni ancora in corso.

Prerequisiti: autorizzazione esplicita al flash, inventario H00, configurazione MAC/chiavi
corretta, due alimentazioni stabili, firmware compilati con stessi canale e protocollo.
Salvare log TX/RX, configurazione priva di segreti, build/versioni/hash e scheda risultati.
Per la prova ESP-NOW senza router verificare esplicitamente il comportamento
con AP assente; dashboard e OTA richiedono invece il router. Interrompere in caso di
alimentazione instabile o rischio fisico; questo documento non dimensiona la parte elettrica.

| Test | Procedura e criterio osservabile |
|---|---|
| H00 inventario | Scheda A/B, foto/serigrafie, chip/revisione/porta/MAC/flash, profilo compatibile, valutazione backup; tutto confermato prima del flash autorizzato |
| H01 vicinanza | 1–3 m, alimentazione stabile, 1000 originali a 2 s (~33 min 20 s minimo), nessun router/AP. Fermare `test off`, aspettare fino a 4 timeout, `stats`. Criterio iniziale banco: 1000/1000 confermati, contatori coerenti, nessun FATAL/overflow; ogni perdita va indagata, senza estrapolare affidabilità |
| H02 seriale | `test off`, aspettare fine, inviare 20 alarm singoli, verificare 20 eventi RX e 20 ACK; test on riprende. Provare comando durante BUSY e comando troppo lungo: rifiuto esplicito, nessun evento fantasma |
| H03 alimentazione | Spegnere RX per 30 s: TX continua e classifica senza conferma entro limite. Riaccendere: scambi recuperati senza reset TX. Togliere/ripristinare alimentazione TX e poi entrambi, annotare reset e nuovi contatori |
| H04 riavvii separati | Riavviare TX e RX separatamente almeno 10 volte ciascuno, anche durante scambio. Sessioni diverse, vecchio ACK non conferma nuovo messaggio, collegamento recupera. Nuovo evento RX dopo reset cache può ripetersi: limite previsto |
| H05 configurazioni errate | Una variazione alla volta: canale diverso, peer MAC unicast inesistente, ID peer errato. Con configurazione di test autorizzata, 20 originali per caso, nessun confirmed, numero invii limitato. Ripristinare e verificare recupero. Ogni nuovo flash richiede autorizzazione |
| H06 chiavi | LMK differenti: 20 originali senza conferme valide; MAC result non è criterio. Esempio a zero: CONFIG_REQUIRED e nessun invio. PMK configurata esplicitamente: una PMK diversa da sola non è un test affidabile di mismatch radio perché la chiave di traffico è LMK. Ripristinare coppia corretta |
| H07 perdita risposta | Cercare condizioni con DATA visibile RX ma ACK perso TX; raccogliere almeno 10 casi con retry e riferimenti coincidenti. RX non riesegue evento nella cache, TX conferma o conclude senza ACK. Se non si riproduce, dichiarare non eseguito/inconcludente; la simulazione host T06 copre la logica ma non la radio |
| H08 distanza/ostacoli/scatole | Tappe misurate (nessuna portata presunta), almeno 500 originali per tappa, vista libera e ostacoli reali. Ripetere con scatole chiuse/posizione definitiva, orientamento e macchinari in funzione annotati. Riportare first/D, confirmed/D, senza ACK, RTT e overflow. Soglia operativa da decidere prima di qualificare impianto |
| H09 prolungata | Almeno 24 ore e almeno 43200 originali con intervallo 2 s; dichiarare durata effettiva e numero reale (non inferirlo solo dal tempo). Nessun reset/FATAL/overflow non spiegato; tutti i contatori riconciliati. Estendere fino ad avere entrambi i minimi; qualifica di affidabilità resta futura |
| H10 carico/errori | Parametri minimi ammessi per almeno 1000 originali, monitor aperti; osservare drop/errori e reattività seriale. Provocare disconnessione peer, verificare limite retry. Errori init/callback assente non riproducibili senza strumenti: marcare sotto-casi non eseguiti, non fingere fault injection hardware |

Registrare `first`, `confirmed`, `unconfirmed` e `originals` a fine transazioni.
Confrontare ogni evento RX per tripla ID/sessione/seq con gli eventi TX. Un RX che
riceve senza ACK a TX è un caso atteso e non una contraddizione. Scartare dalle stime
radio prove con errore configurazione, FATAL, reset non segmentati o acquisizione incompleta;
conservare comunque i log come evidenza del problema.

RSSI non disponibile. Le misure non permettono da sole di distinguere disturbo,
perdita DATA e perdita ACK: servono i log di entrambi i nodi. Non dichiarare portata
garantita né usare questa prova come collaudo di un impianto certificato.

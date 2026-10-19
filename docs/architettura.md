# Architettura e protocollo v3

## Fase 1 implementata

```
PC seriale <--USB--> TX ESP32 <--ESP-NOW unicast cifrato--> RX ESP32 <--USB--> PC seriale
```

TX e RX si associano al router per gli aggiornamenti OTA autenticati; RX serve anche
la dashboard HTTP. Durante il caricamento OTA il loop è sospeso (manutenzione).
Entrambi usano lo stesso canale fisso e power save disabilitato.
TX e RX emettono DATA per le prove manuali e ACK per confermare; un unico peer configurato, MAC e ID controllati.
Le callback copiano dati in coda (16 elementi) o pubblicano un esito atomico;
parsing, stampe e stato applicativo sono nel loop. Code piene producono contatori,
non blocchi. Si attende la callback radio prima di inviare il pacchetto successivo.
L'assenza della callback per 2 s arresta la radio applicativa con FATAL: quella prova
non è valida e richiede diagnosi/riavvio. Gli errori immediati di invio consumano
un tentativo e seguono timeout/retry normali. Non sono conferme di ricezione.

## Messaggio: esattamente 244 byte

Tutti gli interi multibyte sono little endian; nessuna struct C++ è trasmessa grezza.

| Offset | Byte | Campo |
|---:|---:|---|
| 0 | 2 | magic 0x43 0x41 |
| 2 | 1 | versione applicativa 3 |
| 3 | 1 | kind: DATA=1, ACK=2 |
| 4 | 1 | event: prova=1, simulazione=2, telemetria=3, presenza=4, rimozione=5, messaggio testuale=6 |
| 5 | 1 | attempt: 0 primo invio, 1..10 retry |
| 6 | 2 | riservati, devono essere zero |
| 8 | 4 | ID mittente non nullo |
| 12 | 8 | sessione mittente non nulla |
| 20 | 4 | sequenza non nulla |
| 24 | 4 | ID originale confermato (zero nel DATA) |
| 28 | 8 | sessione originale confermata (zero nel DATA) |
| 36 | 4 | sequenza originale confermata (zero nel DATA) |

| 40 | 2 | distanza mm; 65535 = misura non valida |
| 42 | 1 | sensore: disabilitato=0, sconosciuto=1, libero=2, presente=3 |
| 43 | 1 | riservato zero |
| 44 | 4 | età campione ms; 4294967295 = nessun campione |

| 48 | 1 | lunghezza testo UTF-8 in byte, 0..192 |
| 49 | 3 | riservati zero |
| 52 | 192 | testo senza terminatore; padding zero |

Massimo 48 codepoint UTF-8, nessun controllo; testo consentito solo per evento 6.
Protocollo incompatibile con v1/v2: aggiornare entrambe le schede.
ACK copia anche il testo e i campi sensore, oltre a event, attempt e sequenza del DATA, usa ID/sessione propri e valorizza
la tripla di riferimento. La sequenza dell'ACK non è un contatore indipendente.
RX risponde anche ai duplicati e ai pacchetti vecchi validi per recuperare ACK persi.
Non risponde a formato, versione, ID o MAC non accettati. Le chiavi sono nel peer
ESP-NOW e non nel pacchetto. La callback di questa API non espone RSSI: non viene
stimato, né sostituito con `WiFi.RSSI()` che riguarderebbe un access point.

TX richiede corrispondenza di peer, tipo evento, ID/sessione/sequenza referenziati e
un attempt già inviato. Accetta una risposta di un tentativo precedente se la
transazione è ancora entro l'attesa corrente; a scadenza esatta l'ACK è tardivo.
Le callback MAC, anche positive, non completano la transazione. Gli ACK tardivi
non riclassificano messaggi già segnati senza conferma.

## Riavvii, sequenze e duplicati

La sessione casuale a 64 bit nasce dopo l'avvio Wi-Fi con `esp_random()`.
La sequenza parte da 1 e, prima di avvolgersi, viene cambiata sessione. La collisione
casuale rimane teoricamente possibile: non è una garanzia crittografica di unicità.
RX conserva quattro coppie ID/sessione, sostituite FIFO, con una finestra di 64
sequenze ciascuna. Accetta il riordino entro la finestra; sequenze già viste sono
duplicate; quelle precedenti alla finestra sono stale e non rieseguono l'evento.
Dopo espulsione di una sessione o riavvio RX, un vecchio DATA può risultare nuovo.
Questo limita la deduplicazione e impedisce qualsiasi promessa di exactly-once.
TX riconosce come duplicati ACK dell'ultima transazione confermata; ACK più vecchi
sono unmatched. Questi limiti sono intenzionali e testati.

Il riavvio RX non invalida la correlazione dell'ACK: RX usa la sua nuova sessione
ma continua a riferire quella TX. Il riavvio TX crea una nuova identità di scambio;
le risposte riferite alla vecchia sessione sono scartate. Contatori e cache sono RAM.

## Temporizzazione e metriche

Durate tramite sottrazione `uint32_t(now-then)`, valida oltre il rollover millis
con intervalli limitati; loop chiamato almeno una volta ogni 2^32 ms. Un accumulatore
64 bit estende l'uptime e l'età dell'ultima comunicazione anche per silenzi prolungati.
Nessuna attesa lunga nel loop; `delay(1)` cede CPU. I log seriali possono introdurre
latenza: usare parametri realistici, raccogliere entrambi i lati e contare overflow.
I tempi sono misurati al processamento nel loop, non all'antenna.

- **Originals**: transazioni iniziate TX, incluse quelle ancora attive.
- **Attempts**: chiamate invio DATA, inclusi errori immediati; retry = attempts-originals.
- **First**: transazioni confermate prima di qualsiasi ritrasmissione. È una misura
  conservativa della consegna confermata al primo tentativo, non della sola ricezione RX.
- **Confirmed**: originali con ACK valido entro l'ultima finestra ammessa.
- **Unconfirmed**: originali conclusi senza ACK; NON dimostra mancata ricezione DATA.
- **Timeouts**: singole attese scadute, incluse quelle seguite da retry e quella finale.
- Denominatore conclusi `D=confirmed+unconfirmed`; first/D e confirmed/D sono i tassi
  da riportare. Con D=0 non sono definiti. Con sensore disabilitato, fermare periodico e attendere
  la conclusione: `originals=D`. Con sensore attivo la telemetria continua: un
  riepilogo può includere una transazione ancora in corso. Non confrontare tassi con denominatori diversi.
- RTT: dal tentativo identificato nell'ACK alla sua elaborazione TX. L'echo di attempt
  evita l'ambiguità con risposte di invii precedenti; si include latenza di code e log.
  Media, min, max e deviazione standard campionaria (Welford); n<2 dà sd=0 convenzionale,
  n=0 significa assenza di campioni, non RTT zero. Nessun percentile dichiarato.
- Completion: tempo dal primo invio alla conferma; media separata, comprende retry.
- Last-valid-age: TX ultima conferma accettata o duplicato dell'ultimo ACK; RX ultimo
  DATA valido, anche duplicato/stale. -1 se mai visto; MAC success non aggiorna l'età.
- RX `rx_originals`: nuovi eventi secondo cache locale; `duplicates`, `stale` separati.
  Non equiparare questa stima al totale TX dopo riavvii o espulsioni dalla cache.

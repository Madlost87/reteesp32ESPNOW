# Seriale e raccolta CSV

115200 baud, 8N1. Aprire un monitor per scheda, usando le porte identificate nella guida
build/flash. DTR/RTS a zero riducono i reset indesiderati; alcuni adattatori possono comunque
resettare all'apertura. Eventuali reset iniziano una nuova sessione, non vanno nascosti.

```sh
.venv/bin/pio device monitor -p "$PORT_TX" -b 115200 --dtr 0 --rts 0 --filter log2file
.venv/bin/pio device monitor -p "$PORT_RX" -b 115200 --dtr 0 --rts 0 --filter log2file
```

Eseguire i due comandi in terminali separati; il filtro comunica il nome del file creato.
Spostare i log in `results/` con nomi univoci TX/RX e ID prova. Non aggiungere filtri
che antepongano timestamp alle righe CSV. Uscita monitor Ctrl+C. `help` mostra i comandi.

- `test off`: ferma le prove periodiche; sensore e telemetria ogni 250 ms restano attivi.
- `test on`: TX riprende periodico; la prima prova parte dopo `intervalMs`.
- `alarm`: un singolo evento simulato con lo stesso ACK/retry; se BUSY ripetere dopo
  conclusione. Non aziona dispositivi. Consigliato in modalità periodica disattivata.
- `stats`: riepilogo immediato, disponibile su entrambi i ruoli.

## Formati

Righe `#` sono diagnostiche leggibili; righe ROM non appartengono al protocollo di log.
`E` e `S` sono due tabelle CSV differenti, senza intestazione in seriale. Per estrarle:

```sh
mkdir -p results
python3 tools/extract_csv.py results/prova01-tx.log --prefix results/prova01-tx
python3 tools/extract_csv.py results/prova01-rx.log --prefix results/prova01-rx
```

Lo strumento crea `*-events.csv` e `*-summary.csv` con intestazioni; rifiuta di
sovrascriverle. Una riga E/S con numero campi errato viene contata e determina exit 1.
Conservare sempre il log grezzo; il tool non certifica l'integrità dell'acquisizione.

**E (12 colonne):**
`record,uptime_ms,role,type,node,local_session,sequence,event,attempt,packet_session,ref_session,value`.

Sessioni esadecimali 16 cifre, trattarle come testo nei fogli elettronici. `node` è
il nodo locale; `packet_session` è quella del mittente del pacchetto descritto;
`ref_session` zero nel DATA, sessione TX confermata nell'ACK. `sequence` è quella
dell'originale (anche negli ACK). `event` 1=prova, 2=simulazione, 3=telemetria, 4=presenza, 5=rimozione, 6=prova radio manuale; `attempt` parte da 0.
`uptime_ms` è locale, 64 bit: non sottrarre timestamp tra schede senza sincronizzazione.

| type | value / significato |
|---|---|
| data_send, ack_send | 0; API ha accettato l'invio, nessuna conferma ancora |
| send_error | codice esp_err_t; descrizione nella riga `#` successiva |
| mac_result | 0 successo MAC, 1 fallimento MAC; non è ACK applicativo |
| confirmed | RTT in ms del tentativo indicato dall'ACK |
| timeout_retry | 0; sta iniziando il retry `attempt` mostrato |
| unconfirmed | 0; limite esaurito senza ACK |
| probe, alarm_simulated, telemetry, sensor_alarm, sensor_clear, radio_test | 0; RX ha elaborato un nuovo originale secondo la cache |
| data_duplicate, data_stale | 0; RX non riesegue evento, risponde comunque |
| ack_duplicate, ack_unmatched | 0; ACK non completa un nuovo originale |

**S (28 colonne):**
`record,uptime_ms,role,local_session,originals,attempts,first,confirmed,timeouts,unconfirmed,rx_originals,duplicates,stale,unmatched,invalid,mac_ok,mac_fail,send_errors,queue_drops,ack_queued,ack_dropped,rtt_n,rtt_mean_ms,rtt_sd_ms,rtt_min_ms,rtt_max_ms,completion_mean_ms,last_valid_age_ms`.

Contatori cumulativi dall'avvio. Dalla versione 0.5.0 entrambi i ruoli possono trasmettere DATA e confermare
il peer: contatori di transazione e ricezione possono essere non nulli su entrambi. `duplicates` comprende
duplicati DATA sul RX oppure ACK dell'ultima transazione confermata sul TX.
`invalid` comprende lunghezza, formato e peer non validi. `queue_drops` è perdita
nella coda di ingresso; `ack_dropped` nella coda di uscita RX. `ack_queued` non significa
ACK ricevuto dal TX. `mac_ok/mac_fail` riguardano DATA sul TX, ACK sul RX.

Per definizioni esatte di tassi, scadenze, RTT e age vedere [architettura](architettura.md).
`last_valid_age_ms=-1` se mai ricevuta comunicazione applicativa valida; n=0 rende
non significativi i campi RTT. Contatori sospesi da FATAL non valgono come prova di
qualità radio. Segmentare sempre i risultati per avvio; annotare gli eventi simulati
insieme ai periodici oppure analizzarli separatamente usando E.

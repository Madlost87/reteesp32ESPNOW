# Scheda da copiare per ogni prova

**Stato iniziale: NON ESEGUITO — nessun valore radio misurato.**

- ID prova / test Hxx:
- Operatore, data/ora inizio/fine, durata effettiva:
- Luogo, meteo, macchinari/interferenze osservate:
- TX scheda/modulo/chip/revisione/porta/MAC/flash:
- RX scheda/modulo/chip/revisione/porta/MAC/flash:
- Alimentazione e tensioni osservate, scatole, antenne/orientamento/altezza:
- Firmware/versione/hash/configurazione senza chiavi:
- Canale, intervalMs, timeoutMs, retries, summaryMs:
- Distanza misurata, ostacoli/materiali, posizione/foto:
- Cifratura configurata: sì/no; identificatore chiavi interno (mai valori):
- Sessioni TX/RX e reset osservati:
- Log grezzi TX/RX e CSV associati:

| Misura | TX | RX |
|---|---|---|
| Originali / nuovi eventi secondo cache | | |
| Tentativi / ACK accodati | | |
| Confermati al primo tentativo | | n/a |
| Confermati finali | | n/a |
| Senza conferma | | n/a |
| Timeout singole attese | | n/a |
| Duplicati / stale | | |
| Errori API / MAC success-fail | | |
| Overflow code / ACK scartati | | |
| RTT n/media/sd/min/max ms | | n/a |
| Tempo completamento medio ms | | n/a |
| Età ultima comunicazione valida ms | | |
| Conclusi D / first÷D / confirmed÷D | | n/a |

- Casi DATA ricevuto ma ACK non osservato dal TX:
- Soglia dichiarata prima della prova e motivazione:
- Esito: SUPERATO / FALLITO / NON ESEGUITO / INCONCLUDENTE:
- Anomalie, limiti del campione e azioni successive:
- Eventuali sottocasi non eseguiti (es. fault injection):

Non sommare contatori cumulativi di righe S successive; usare ultimo riepilogo
per sessione/avvio o differenze esplicite. Dopo un reset separare i segmenti.

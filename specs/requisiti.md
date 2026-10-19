# Specifiche verificabili del sistema

Stato dei requisiti/test nella [matrice](../tests/matrice.md). “Implementato” indica
presenza del codice, non verifica sul campo. Soglie della fase 1 sono criteri di prova
del prototipo, non garanzie per l'impianto finale. Fasi 2–4 sono solo pianificate.

## Fase 1 — due ESP32, da implementare ora

| ID | Requisito e criterio di accettazione |
|---|---|
| F1-01 | Due progetti distinti TX/RX con Arduino e versioni bloccate; entrambi compilano con le istruzioni e condividono la logica testata |
| F1-02 | Target ESP32 classico; inventario individuale di chip, porta, flash e profilo prima di upload; nessun LED/pulsante non verificato usato |
| F1-03 | ESP-NOW offline, STA non associata, canale fisso uguale e configurabile; 1000 scambi vicini senza router né AP |
| F1-04 | Boot seriale con ruolo, firmware, protocollo, MAC STA, canale, sessione; MAC peer configurabile e documentato |
| F1-05 | Pacchetto compatto e validato: versione, nodo, sessione, sequenza, event, attempt; rigetto di lunghezze/campi non validi e peer inattesi |
| F1-06 | ACK applicativo riferito all'originale; MAC success non completa lo scambio; ACK riferito a un altro originale non modifica confirmed |
| F1-07 | Timeout configurabile e retry limitati; per ogni originale al massimo retries+1 invii, inclusi rifiuti API; classificazione finale senza conferma |
| F1-08 | Distinguere originali, tentativi, conferme, timeout, duplicati e stale; non ripetere evento RX entro cache e documentarne i limiti |
| F1-09 | Riavvio separato: nuova sessione TX, ACK vecchi non confermano nuovi originali; riavvio RX consente recupero del collegamento |
| F1-10 | RTT per attempt confermato, media/min/max/deviazione standard, tempo completamento ed età ultima comunicazione; casi rollover testati |
| F1-11 | Callback brevi e code limitate; errori init/send e overflow osservabili, nessun ciclo di retry infinito o attesa lunga applicativa |
| F1-12 | Periodico e alarm seriale con identico trasporto; test on/off, help, stats; BUSY dichiarato, nessun hardware GPIO necessario |
| F1-13 | Intervallo/timeout/retry/riepilogo configurabili; CSV documentato ed estraibile senza confondere record evento e riepilogo |
| F1-14 | Definizione di consegna al primo tentativo/finale e senza ACK; denominatori espliciti; assenza ACK non dichiarata come certa mancata ricezione |
| F1-15 | Cifratura unicast con PMK/LMK locali; niente segreti reali negli esempi/log; configurazione incompleta bloccata; chiavi errate non producono conferme valide |
| F1-16 | Limiti replay/deduplica espliciti; RSSI assente quando API non lo espone; nessuna promessa di mesh, portata, autonomia o certificazione |
| F1-17 | Prove campo ripetibili: vicinanza, distanza/ostacoli, alimentazione, riavvio, canale/MAC/chiavi errati, scatole, durata; report con quantità e stati reali |
| F1-18 | Nessun flash/erase/OTA prima di richiesta esplicita; prima del flash identificazione, destinazione, avviso sostituzione e valutazione backup |
| F1-19 | Test host sul codice condiviso e documentazione italiana completa con matrice; distinguere superato/fallito/non eseguito e hardware da fare |

## Fase 2 — sensori e centrale locale, pianificata

| ID | Requisito e criterio di accettazione futuro |
|---|---|
| F2-01 | Identificazione/associazione autorizzata per nodo: nodo sconosciuto non entra nel sistema; nuova associazione/revoca tracciata |
| F2-02 | Inserimento/disinserimento con stati espliciti e autorizzazione; comando non autorizzato, scaduto o ripetuto non disarma |
| F2-03 | Distinguere intrusione, manomissione, batteria bassa e perdita rete elettrica; stimolando ogni ingresso appare il corretto evento e risposta locale |
| F2-04 | Heartbeat e indisponibilità: con periodo H e timeout L approvati, nodo spento segnalato entro L più tolleranza misurata; recupero tracciato |
| F2-05 | ACK/retry/deduplicazione durevole e riavvio: stessa intrusione non aziona due volte la politica evento; guasti durante persistenza non perdono stato critico |
| F2-06 | Centrale/sirena autonome: interruzione ponte casa, Internet o gateway non impedisce azione locale entro latenza obiettivo approvata |
| F2-07 | Alimentazione e backup dimensionati su consumi misurati; prova distacco rete e autonomia richiesta A, avviso prima di esaurimento |
| F2-08 | Autenticità, freshness e chiavi per nodo con rotazione/revoca: replay pre/post riavvio e chiavi revocate rifiutati senza disarmo |
| F2-09 | Registro eventi resistente ai blackout, capacità C e budget scritture W; test fault injection e wear budget rispettati, overflow esplicito |
| F2-10 | Diagnostica/manutenzione autorizzate, stato batteria/radio/versione, aggiornamenti e recupero documentati; prova manutenzione e rollback sicuro |
| F2-11 | Copertura e latenza reali per tutti i nodi nelle scatole finali; soglie di affidabilità e durata concordate prima della qualificazione |

H, L, A, C, W e le soglie non sono inventati: devono essere decisi prima di implementare
questa fase; i relativi test non possono essere marcati superati in loro assenza.

## Fase 3 — eventuale ponte LoRa, pianificata

| ID | Requisito e criterio di accettazione futuro |
|---|---|
| F3-01 | Scheda, banda, pin, antenna e vincoli radio confermati prima di build/collegamento; scheda decisionale approvata e conformità valutata nel luogo d'uso |
| F3-02 | Ponte cantiere/casa con identità evento end-to-end, conferme distinte, retry/code limitati e priorità; perdita di ogni tratta non causa falsa conferma finale |
| F3-03 | Sicurezza end-to-end e indipendenza della centrale; ponte compromesso o spento non può disarmare e non arresta sirena locale; portata misurata |

## Fase 4 — eventuale Home Assistant/notifiche, pianificata

| ID | Requisito e criterio di accettazione futuro |
|---|---|
| F4-01 | Gateway locale espone eventi, disponibilità e timestamp ad Home Assistant; test di ricostruzione dopo disconnessione senza notifiche duplicate non dichiarate |
| F4-02 | Notifiche dichiarano indisponibilità del canale; Internet/broker/HA spenti non compromettono allarme locale; recupero e code limitati verificati |
| F4-03 | Comandi remoti solo autorizzati, autenticati, con scadenza e audit; token scaduto/revocato e replay non modificano lo stato inserito |

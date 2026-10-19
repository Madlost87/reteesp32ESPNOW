# Aggiornamento fluidità 0.3.0

La pagina è servita dalla centrale ESP32 su http://192.168.1.176/ (DHCP).
Entrambe le schede richiedono protocollo v2: 48 byte invece dei precedenti 40.

## Percorso del dato

1. TX genera un impulso su G5 ogni 100 ms e misura ECHO G18 tramite interrupt.
2. Tre campioni sotto 20 cm confermano presenza; tre ad almeno 25 cm confermano
   rimozione. La fascia intermedia mantiene lo stato. Nessun cooldown di 10 s.
3. Le transizioni hanno priorità; la telemetria invia distanza, stato ed età del
   campione ogni 250 ms quando la radio è libera. Un solo scambio DATA/ACK attivo.
4. RX riconosce duplicati, risponde con ACK e aggiorna stato corrente e registro.
   Le simulazioni sono distinguibili dagli eventi fisici; il registro conserva 16 eventi.
5. Il task HTTP serve una copia dello stato aggiornata ogni 100 ms. Il browser
   richiede aggiornamenti ogni 250 ms, senza sovrapporre richieste lente.

In condizioni normali il filtro richiede circa 200–300 ms dall'ingresso; snapshot,
polling e trasmissione aggiungono latenza. L'obiettivo è una risposta sotto il secondo,
non una garanzia misurata sull'intera catena fisica. La distanza arriva anche senza
cambiamenti di stato. Il banner mostra lo stato attuale, non un timer di 30 secondi.

Eco assente è una misura non valida, non una zona libera. Età campione >=1500 ms
indica dato obsoleto; nessun contatto radio per più di 10 s indica nodo offline.
L'età include l'attesa sul TX e i retry, ma non una misura sincronizzata del tempo
in volo. Lo stato fisico non regredisce per pacchetti con sequenza precedente nella
stessa sessione; sono ricordate quattro sessioni ritirate.

## Limiti residui

Timeout ACK 250 ms e tre retry possono occupare lo scambio per circa un secondo.
Otto transizioni sono accodabili: overflow segnalato in seriale. Nessuna persistenza
né ritrasmissione infinita degli eventi; lo stato viene rinfrescato dalla telemetria.
Seriale molto carica, Wi-Fi debole e riconnessioni possono ancora introdurre ritardi.
`test off` non spegne sensore o telemetria. Lo script storico `analyze_pair.py` è
per prove periodiche fermate, non per questa acquisizione con telemetria continua.

## Verifica ripetibile

- `./tools/test.sh`: protocollo, filtro sensore, code, ordinamento e CSV.
- `node tests/test_dashboard.js`: script reale della pagina con DOM simulato.
- Build `tx` e `rx` e caricamento su entrambe le schede.
- `tools/check_live_sensor.py`: reset esplicito delle due schede, acquisizione
  seriale, tre comandi di simulazione e polling HTTP simultaneo per circa un minuto.
  Richiede porte e URL espliciti e una directory nuova; il reset cancella lo storico.

I risultati della sessione sono in `results/fluid-v030/`.

## Esito sul dispositivo

Firmware 0.3.0 caricato con successo su TX e RX, hash verificati dal caricatore.
Nella prova di circa 63 secondi:

- 253/253 DATA correlati tra i log TX/RX e confermati al primo tentativo;
  tre sono allarmi simulati. Zero timeout, errori o overflow rilevati.
- RTT applicativo medio 5,85 ms, minimo 2 e massimo 85 ms.
- 199 richieste HTTP riuscite: media 40,08 ms, minimo 22,13 e massimo 122,23 ms.
- Età massima riportata del campione 348 ms, power save disabilitato e canale 6.
- HTML servito identico al sorgente (7692 byte). Test JavaScript superato;
  nessuna verifica visiva in browser reale.

Il riepilogo periodico TX a 60 s include una transazione attiva (243 iniziate,
242 confermate); i log successivi ne documentano la conferma. Il conteggio 253/253
usa l'intera acquisizione, non quel riepilogo intermedio.

**Nessuna misura di distanza valida:** tutti i campioni HTTP indicano stato
sconosciuto e distanza -1. Radio e pagina funzionano; non è stata verificata la
risposta fisica presenza/rimozione. L'assenza eco può dipendere da bersaglio,
collegamenti, alimentazione o sensore: i log non identificano quale causa.
Pin configurati invariati: TRIG G5, ECHO G18. La logica di presenza/rimozione è
verificata nei test host, non ancora con un ostacolo reale in questa sessione.

Log, CSV e misure: `results/fluid-v030/live/analysis.json`, `http.jsonl` e log seriali.
Questa prova breve non dimostra affidabilità a lungo termine né portata in cantiere.

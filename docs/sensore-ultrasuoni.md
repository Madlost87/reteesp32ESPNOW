# Sensore ultrasuoni — diagnosi chiusa, non risolta

Dalla versione 0.5.0 il sensore è **disabilitato** su richiesta dell’utente per
passare alle [prove messaggi](prova-messaggi.md). Le sezioni seguenti descrivono
il driver e le prove precedenti, non una misura attualmente attiva.

Il profilo `espnow_tx/platformio.ini` abilita ora il sensore con TRIG G5 ed ECHO G18,
come confermato dall’utente. RX non usa il sensore. Nel codice condiviso, in assenza
del flag esplicito, `ENABLE_ULTRASONIC` resta 0. Per tornare alla sola prova seriale,
impostare `-DENABLE_ULTRASONIC=0` nel profilo TX e ricompilare.
Le configurazioni locali con MAC/chiavi non sono state modificate da questa correzione.
Questa conferma dei pin non costituisce verifica dei livelli elettrici del sensore.

Per abilitare il sensore sul TX, dopo conferma del modello, dei collegamenti e dei
livelli elettrici, aggiungere alle build_flags dell'ambiente TX:
`-DENABLE_ULTRASONIC=1`, `-DSENSOR_TRIGGER_PIN=<GPIO confermato>` e
`-DSENSOR_ECHO_PIN=<GPIO confermato>`, conservando `${env.build_flags} -DROLE_TX=1`.
I segnaposto non sono comandi da eseguire. Trigger ed echo devono essere distinti,
trigger deve supportare uscita, entrambi devono essere disponibili sul modulo reale.
La sola validità del numero GPIO non verifica cablaggio o compatibilità elettrica.
Il codice non autorizza a collegare direttamente un segnale di tensione non verificata.

Il driver inizializza OUTPUT/INPUT e misura l'eco tramite interrupt CHANGE.
Campiona ogni 100 ms, abbandona l'attesa dopo 30 ms e non usa pulseIn; soltanto
l'impulso trigger di 10 microsecondi è sincrono. Il loop continua a elaborare ACK,
comandi e scadenze durante l'attesa dell'eco. Le misure dipendono dalla latenza ISR
ed esigono verifica sul dispositivo: non sono uno strumento di precisione.

Il sensore conferma presenza dopo tre campioni consecutivi sotto 20 cm e
rimozione dopo tre campioni consecutivi ad almeno 25 cm. La fascia intermedia
mantiene lo stato precedente. Nessun blocco di 10 secondi: una nuova entrata dopo
una rimozione può generare subito un nuovo evento. Eco assente o fuori durata
mostra stato sconosciuto, senza fingere che l'ostacolo sia stato rimosso.

Una coda di otto transizioni conserva presenza/rimozione mentre la radio è occupata;
gli eventi hanno priorità sulla telemetria ogni 250 ms. Un overflow è segnalato
in seriale da `sensor_event_drops`. Esauriti i retry, l'evento non è ritrasmesso
indefinitamente; la telemetria successiva aggiorna comunque lo stato corrente.
La coda e il registro non sono persistenti.

`test off` ferma solo le prove periodiche: telemetria e sensore continuano.
Il protocollo v2 distingue simulazione, presenza e rimozione. Non sono implementati
sirena o inserimento/disarmo. Campioni più vecchi di 1500 ms sono segnalati come
obsoleti dalla pagina; assenza eco e nodo offline sono condizioni differenti.

## Verifiche

- Build TX con sensore abilitato su G5/G18 e build RX senza sensore.
- Build aggiuntiva `sensor_compile` con GPIO 5/18 solo come parametri di compilazione:
  NON conferma quei pin sulle schede e NON è stata caricata.
- Test host di rilevamento valido/nullo/fuori soglia, conservazione durante BUSY,
  isteresi e transizioni e rollover del timer.
- Test sul serializzatore CSV reale condiviso: 28 campi, ordine e valori delle metriche,
  età ultima comunicazione e caso senza campioni. Compilazione host con controllo formato.

Prove hardware sensore: NON ESEGUITE. Da verificare eco assente, ostacoli a distanze
note, BUSY durante rilevamento, carico radio simultaneo e comportamento elettrico.

## Diagnostica TX 0.4.1

Il sensore va collegato al TX (MAC 7C:9E:BD:62:12:94, IP rilevato 192.168.1.117),
non alla centrale RX. `http://192.168.1.117/api/sensor` espone contatori dall'avvio:

- `triggers`: cicli di trigger richiesti dal driver; non è una misura elettrica del pin.
- `rising_edges` / `falling_edges`: fronti rilevati dall'interrupt su ECHO G18.
- `completed_pulses`: impulsi completati durante una finestra di misura.
- `timeouts_no_rise` / `timeouts_no_fall`: scadenze senza salita o senza discesa.
- `last_pulse_us`: ultima durata completata; zero all'ultimo timeout.
- `echo_level`: livello logico letto al momento della richiesta HTTP.

Contatori di trigger in aumento senza fronti ECHO restringono l'indagine a sensore,
alimentazione, collegamenti o acquisizione GPIO/interrupt; da soli non identificano
un componente guasto. Verificare modello, livelli elettrici e scheda a cui è collegato
il sensore. L'endpoint è di sola lettura e non cambia pin, soglia o funzionamento.

Esito del 18 ottobre: 156 trigger richiesti, zero fronti/impulsi ECHO rilevati,
156 timeout senza salita; G18 basso in tutte le tre letture HTTP. Telemetria verso
RX ripresa senza errori dopo OTA TX. Misura fisica ancora non validata.

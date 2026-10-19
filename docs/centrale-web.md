# Centrale web ESP32 — firmware 0.4.0

La pagina e l'API sono servite direttamente dalla centrale, porta HTTP 80:
**http://192.168.1.176/** e **http://192.168.1.176/api/status** (IP verificato in questa sessione). Non richiede Internet, CDN o server
sul PC. Il router assegna l'indirizzo via DHCP: per stabilizzarlo impostare una
prenotazione DHCP per il MAC della centrale D4:D4:DA:E4:3F:A4.

## Rete e ESP-NOW

La richiesta di collegamento alla LAN estende la fase iniziale senza router.
Entrambe le schede sono STA associate al Wi-Fi per OTA; ESP-NOW continua sul canale 6.
Vedere [aggiornamenti OTA](aggiornamenti-ota.md).
Configurazione locale in `espnow_rx/include/network.local.hpp` (ignorata da Git,
permessi 0600), modello in `network.example.hpp`. Password mai nei log.
SSID previsto: homeassistant, AP 2,4 GHz BSSID 6A:D7:9A:1B:33:12, canale 6.
L'ESP32 classico usa la rete 2,4 GHz; la connessione del PC a 5 GHz non viene modificata.

La radio è condivisa: router, centrale e TX devono usare lo stesso canale.
Il BSSID è fissato per evitare roaming automatico verso altri AP dello stesso SSID
su canali differenti. Non cambiare canale del router senza aggiornare il progetto.
Con AP assente, tentativi di connessione limitati a 10 secondi ogni 30; al timeout
si ripristina il canale ESP-NOW configurato. Scansione/associazione possono causare
perdite temporanee: la continuità in assenza router richiede una prova dedicata.
Un eventuale cambio canale dell'AP è segnalato come anomalia nella pagina, non
corretto silenziosamente sul nodo remoto. Il router resta necessario per accedere via HTTP.

## Interfaccia

- Stato centrale e stato contatto del nodo: scadenza 10 secondi.
- Originali ricevuti, numero allarmi, età ultimo contatto e canale effettivo.
- Ultimi 16 eventi di allarme/rimozione con nodo, sequenza, sessione nell'API ed età dell'evento.
- Versione, uptime, IP, RSSI della connessione al router, duplicati/errori/overflow.
- Distanza e stato fisico del sensore; poll ogni 250 ms, indicazione di dati obsoleti se la richiesta HTTP fallisce.

La presenza fisica usa lo stato del sensore, senza mantenere un falso allarme per
30 secondi. Le simulazioni sono solo nel registro, distinte da presenza e rimozione.
Inserimento/disarmo e sirena non sono implementati. Il registro è RAM e si azzera
al reset; conserva 16 eventi. Con sensore attivo, `test off` non ferma la telemetria.

Pagina e API sono di sola lettura, accessibili ai dispositivi della LAN, senza
login e senza TLS. Non sono aggiunti comandi remoti di disarmo o modifica chiavi.
Le credenziali Wi-Fi e ESP-NOW non sono incluse nelle risposte HTTP.

## Implementazione e verifica

`espnow_rx/web/index.html` contiene HTML/CSS/JS; eseguire `python3 tools/embed_web.py`
dopo averla modificata per rigenerare `shared/web_page.hpp`, quindi compilare RX.
`shared/central_web.hpp` usa il server HTTP IDF in un task dedicato e una copia
sincronizzata dello stato, aggiornata ogni 100 ms, per separare HTTP dal loop radio.
`shared/central_state.hpp` gestisce registro e scadenza del contatto; test host su
limite del registro, ordinamento, reset iniziale e soglia di indisponibilità.

Fonti ufficiali verificate per lo stack selezionato:
[server HTTP IDF 4.4.7](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/protocols/esp_http_server.html),
[ESP-NOW IDF 4.4.7](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/network/esp_now.html).
Firma di WiFi.begin verificata anche nell'header locale Arduino 2.0.17.


## Prove precedenti (0.2.1)

Per firmware corrente e prove 0.4.0 vedere [OTA](aggiornamenti-ota.md).
Il rapporto [fluidità 0.3.0](fluidita-v030.md) conserva la prova precedente.

Centrale RX 0.2.1 caricata con SUCCESS e hash verificati. TX conserva il firmware
0.1.1 già installato, compatibile con protocollo v1. Build sorgenti TX/RX e test host
riusciti; test JavaScript della pagina eseguito con `node tests/test_dashboard.js`
(stato normale, allarme, nodo senza contatto e errore HTTP). Non effettuata verifica
visiva in un browser reale.

La prima prova con RX 0.2.0 ha evidenziato 10 originali senza conferma su 33.
La correzione 0.2.1 usa anche `WiFi.setSleep(false)` prima dell'avvio, mantenendo
coerente l'impostazione del core Arduino che viene riapplicata all'evento STA_START.
L'API conferma `wifi_power_save=0` dopo la connessione. La prova ripetuta ha dato:

- 33/33 originali confermati al primo tentativo, inclusi tre allarmi seriali.
- Zero timeout, errori, duplicati e perdite in coda nel campione di circa 77 secondi.
- RTT medio 6.485 ms, deviazione standard 4.338 ms, minimo 3 e massimo 15 ms.
- Dieci richieste HTTP consecutive riuscite mentre ESP-NOW era attivo.
- HTML sul dispositivo identico al file sorgente; API con tutti e tre gli allarmi.
- Periodico TX riattivato a fine prova e nodo online verificato via API.

Log finali in `results/web-v021/`; prova intermedia fallita conservata in
`results/web-v020/`. Il test breve non dimostra portata o affidabilità a lungo termine;
la risposta fisica del sensore e il recupero dopo spegnimento router restano da provare.

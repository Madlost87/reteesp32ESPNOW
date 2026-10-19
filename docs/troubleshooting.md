# Risoluzione dei problemi

| Sintomo | Controllo/azione |
|---|---|
| CONFIG_REQUIRED | Copia del modello completo, MAC unicast dell'altro peer, PMK/LMK non zero; ricompilare, flash solo autorizzato |
| Non compila | Usare Core della virtualenv e versioni bloccate; conservare log; non cambiare framework a 3.x senza porting callback |
| Errore download DNS/HTTP | PC senza accesso ai registry: abilitare accesso autorizzato o predisporre cache; non riguarda il funzionamento offline dei nodi |
| Nessuna seriale | Porta/permessi/cavo dati/115200; chiudere altri monitor; non usare flash come tentativo diagnostico alla cieca |
| Reset frequenti | Alimentazione e USB, brownout, cavi; controllare log ROM e nuova sessione |
| mac_fail | Peer spento, MAC errato, canale diverso, ostacoli/interferenze; non dimostra da solo quale causa |
| mac_ok ma unconfirmed | RX può non aver elaborato DATA o ACK può essersi perso; controllare log RX, chiavi, ID e overflow |
| Chiavi incompatibili | Nessun ACK valido atteso; non dedurre sicurezza dal solo mac_result; confrontare configurazioni in ambiente riservato |
| send_error | Codice/descrizione nel log; peer/interfaccia/configurazione oppure risorse; retry limitato, niente loop infinito |
| FATAL inizializzazione | Correggere errore esp_err_to_name, poi riavvio; il firmware non dichiara READY |
| FATAL callback invio assente | Fermare la misura, conservare log, verificare stack/alimentazione; non continuare a inferire tassi |
| Duplicati RX | ACK perso o ritardato è normale causa; controllare originali e tentativi separati |
| RTT alto o code piene | Carico seriale, intervallo troppo basso, interferenze; aumentare intervallo/timeout e ripetere prova comparabile |
| RSSI assente | Limite della callback IDF 4.4.7 usata; non esiste un valore inventato o preso da un AP |
| alarm restituisce BUSY | Attendere fine transazione e ripetere; con sensore attivo test off non ferma la telemetria. Il comando rifiutato non è memorizzato |

Non pubblicare chiavi, binari con segreti o backup della flash nei report.


## Sensore, pagina e aggiornamenti

| Sintomo | Controllo/azione |
|---|---|
| Eco non disponibile, messaggi in aumento | Radio operativa ma misura non valida. Verificare modello sensore, alimentazione, massa comune, TRIG G5/ECHO G18 e livelli elettrici prima di modificare i collegamenti |
| Tre allarmi nel registro ma nessuna distanza | Le prove hanno generato tre allarmi simulati: non dimostrano il rilevamento fisico |
| Oggetto sotto 20 cm senza allarme | Serve eco valido e tre campioni consecutivi sotto soglia; assenza eco non significa zona libera |
| OTA non raggiungibile | Controllare IP DHCP, Wi-Fi sul canale 6 e /api/device; il TX usa 192.168.1.117, RX 192.168.1.176 nella sessione verificata |
| Authentication Failed | Password errata: controllare il ruolo nel JSON privato, senza riportare segreti nei log |
| OTA autenticato ma trasferimento fermo | Verificare ritorno TCP scheda → PC sulla porta 45678 e UDP PC → scheda sulla 3232, firewall e routing VPN |
| Dati fermi durante OTA | Il caricamento sospende temporaneamente il loop; attendere riavvio e verificare nuovo boot e slot |
| Dopo OTA la scheda non torna online | Verificare DHCP e alimentazione; se firmware/OTA non parte, recupero USB. Rollback automatico non abilitato |

[Procedura completa OTA e recupero](aggiornamenti-ota.md).

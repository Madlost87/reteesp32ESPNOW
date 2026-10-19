# Sicurezza, limiti e decisioni aperte

ESP-NOW unicast è configurato con CCMP e PMK/LMK locali: protegge il traffico della
coppia secondo le capacità del protocollo radio. MAC e ID vengono anche filtrati,
ma non sono da soli un'autenticazione. Non è implementato un ripiego in chiaro.
Le chiavi sono nei binari/flash: senza protezione fisica, secure boot e flash encryption
un dispositivo compromesso può esporle. Non si dichiara una valutazione di sicurezza
completa del vecchio stack selezionato; aggiornarlo e rivalutarlo prima di un prodotto.

**Deduplicazione non significa anti-replay.** La cache RAM limitata non persiste,
la sessione casuale non autentica un comando e la sequenza da sola non prova freschezza.
Non si garantisce resistenza al replay attraverso riavvii, rollback, espulsione cache
oppure a un peer legittimo compromesso. Per l'allarme definitivo progettare associazione
autorizzata, chiavi per nodo, revoca/rotazione, freshness autenticata (contatori durevoli
o challenge), finestra replay e gestione sicura del reset/rollback. Validare il comportamento
reale dello stack con test avversariali. Il prototipo non espone comandi radio di disarmo.

Entrambi i chip ESP32 e la flash da 4 MB sono stati identificati. Restano aperti:
verifica elettrica e risposta fisica del sensore, alimentazione definitiva, ingressi
manomissione, sirena, batteria, autonomia e qualificazione radio sul campo.
Le impostazioni attuali (telemetria 250 ms, offline dopo 10 s, registro RAM 16 eventi)
sono scelte del prototipo, non una qualifica dell'impianto.

Nessuna promessa di portata, autonomia o affidabilità: ostacoli, metallo, macchinari,
scatole, orientamento, interferenze e condizioni meteo richiedono prove reali.
ESP-NOW non è automaticamente mesh. Eventuali ripetitori richiedono progetto distinto,
limite hop, deduplica, gestione dei loop, latenza e sicurezza end-to-end.

## Fasi future

Fase 2: sensori -> centrale locale -> sirena locale. La centrale deve decidere e agire
senza casa, Internet o LoRa; inserimento/disinserimento autorizzato, stati espliciti,
heartbeat, riserva energetica, registro eventi con wear budgeting e diagnostica.

Fase 3 proposta: centrale cantiere -> ponte LoRa cantiere <-> ponte LoRa casa -> ricezione
locale. Trasferire eventi/heartbeat con identità end-to-end; separare ACK del ponte da
ACK della destinazione. Usare code limitate, priorità agli allarmi e gestione dei blackout.
Da decidere: modello radio, banda legalmente utilizzabile nel luogo d'installazione,
antenna, pin, potenza, duty cycle, protocollo raw LoRa oppure altra architettura, cifratura,
autenticazione, timeout e velocità. Non si presume alcuna frequenza né si assegnano pin.
La verifica normativa concreta appartiene alla selezione dell'hardware futuro.

Fase 4: gateway casa verso Home Assistant con integrazione da scegliere (ad esempio MQTT
locale autenticato); notifiche esterne solo quando il relativo collegamento è disponibile.
Guasti a gateway, broker o Internet non devono impedire l'allarme locale. Comandi remoti
richiedono autorizzazione, scadenza, anti-replay e audit; non inoltrare direttamente un
semplice messaggio seriale o MQTT come comando di disarmo.


## Superficie di rete nella versione 0.4.0

Entrambi i nodi si associano al router per OTA; RX serve anche la dashboard HTTP.
HTTP è di sola lettura, senza login/TLS. OTA richiede password distinta per scheda,
ma il protocollo ArduinoOTA usato non cifra il firmware e non ne verifica una firma.
Usare LAN fidata o VPN; non esporre direttamente i servizi su Internet.
Il controllo ruolo/MAC dello script protegge da errori operativi, non da un attaccante
sulla rete. Durante OTA sensore/radio sono sospesi; nessuna garanzia di eventi durante
la manutenzione. Due slot non implicano rollback automatico dopo firmware difettoso.
[Dettagli, prove e recupero](aggiornamenti-ota.md).

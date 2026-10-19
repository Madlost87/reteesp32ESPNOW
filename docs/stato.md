# Stato del progetto — 18 ottobre 2026

## Stato corrente: messaggi testuali 0.6.0

Campo testo su TX `/` e RX `/radio`, ultimo testo inviato/ricevuto e conferma.
Protocollo v3 a 244 byte, entrambe le schede da aggiornare insieme. Sensore disabilitato.
[Guida](prova-messaggi.md).

## Cronologia: prova messaggi bidirezionale 0.5.0

Sensore disabilitato; task di diagnosi chiuso come non risolto su richiesta.
Nuove pagine TX `/` e RX `/radio` per inviare messaggi di prova via ESP-NOW e
mostrare ricezione e ACK applicativo. Installazione OTA completata su entrambe;
20/20 messaggi ricevuti e confermati in dieci coppie simultanee. Nessun allarme
generato, zero errori/perdite in coda della centrale. [Procedura](prova-messaggi.md).

## Cronologia: TX 0.4.1 diagnostico / RX 0.4.0

Diagnostica TX installata via OTA, ritorno ad app0 verificato. Nei tre campioni
raccolti i trigger salgono da 114 a 156, ma i fronti ECHO sono zero e il livello
letto su G18 è basso. Nessuna distanza valida. Modello, alimentazione e collegamenti
del sensore ancora da confermare fisicamente. Radio e HTTP rispondono.
Risultati in `results/sensor-v041/diagnostic.json`; [diagnostica](sensore-ultrasuoni.md).

## Cronologia: OTA autenticato 0.4.0

Installazione iniziale USB e aggiornamento remoto completati su TX e RX. Entrambi
hanno rifiutato password errata e avviato app1 dopo OTA. TX 192.168.1.117,
RX 192.168.1.176. Soglia sotto 20 cm e protocollo v2 invariati.
[Procedura, risultati e recupero](aggiornamenti-ota.md). Accesso dalla LAN/VPN;
la manutenzione interrompe temporaneamente campionamento/radio sulla scheda aggiornata.
Prova finale: 253/253 messaggi confermati e 182 richieste HTTP riuscite. Sensore
ancora senza eco valido: i tre allarmi della prova erano simulati, non fisici.
Le sezioni successive sono rapporti storici e non descrivono lo stato attuale.


## Cronologia: firmware 0.3.0 su entrambe le schede

Protocollo v2, telemetria e pagina ogni 250 ms, filtro presenza/rimozione senza
cooldown di 10 s. Flash e prove completati: 253/253 messaggi confermati al primo
tentativo e 199 richieste HTTP riuscite. Distanza fisica non validata: ECHO assente
nel campione. Pagina http://192.168.1.176/. [Rapporto 0.3.0](fluidita-v030.md).
Le sezioni successive conservano la cronologia precedente.

## Cronologia: centrale web 0.2.1

RX aggiornato a 0.2.1 e collegato alla rete homeassistant sul canale 6.
Pagina **http://192.168.1.176/** verificata dalla LAN; API e tre allarmi visibili.
Dopo correzione del power saving: 33/33 conferme al primo tentativo durante prova
con HTTP attivo, nessun errore nel campione. TX installato resta 0.1.1, periodico
riattivato. [Dettagli e limiti](centrale-web.md). Le sezioni sotto sono cronologiche.


## Cronologia: flash TX/RX 0.1.1

Firmware 0.1.1 caricato su entrambe le schede su richiesta esplicita. Comunicazione
ESP-NOW cifrata verificata: 33/33 messaggi confermati al primo tentativo, inclusi tre
allarmi seriali; RTT medio 6.697 ms. Nessun errore nel campione di circa 77 secondi.
[Rapporto completo](test-radio-v011.md). Sensore abilitato ma risposta fisica non verificata.
Pagina HTTP assente. Le sezioni successive conservano la cronologia degli stati precedenti.


## Aggiornamento: primo flash autorizzato eseguito

TX caricato e avvio seriale verificato su ESP32-D0WDQ6 rev. 1.0, flash 4 MB,
MAC 7C:9E:BD:62:12:94. Backup completo verificato. Firmware in CONFIG_REQUIRED,
collegamento ESP-NOW ancora da configurare e provare. Vedere [rapporto primo flash](primo-flash.md).
Le sezioni seguenti conservano il resoconto della preparazione precedente al flash.

## Implementato

Fase 1: due firmware Arduino/PlatformIO per ESP32 classico, protocollo condiviso
40 byte, DATA/ACK, sessioni, timeout e retry, deduplica limitata, metriche e CSV,
comandi seriali, code callback, errori osservabili, peer unicast cifrato con chiavi
locali. Configurazione di esempio incompleta bloccata a runtime. Specifiche italiane,
36 requisiti tracciati, procedure di identificazione, configurazione, backup e flash.

## Verificato sul PC

Ambiente: Linux x86_64, Python 3.14.8, PlatformIO Core 6.1.18 in virtualenv;
platform-espressif32 6.9.0, Arduino 2.0.17/IDF 4.4.7,
Xtensa 8.4.0+2021r2-patch5, esptool 4.5.1, SCons 4.8.1.
Le impostazioni esp32dev stampate dal compilatore sono un profilo, non dati delle schede.

| Verifica | Esito finale | Evidenza locale |
|---|---|---|
| T01 validazione/encoding | SUPERATO | `build/tests.log` |
| T02 sequenze/duplicati/cache/sessioni | SUPERATO | `build/tests.log` |
| T03 ACK corretti e riavvii | SUPERATO | `build/tests.log` |
| T04 timeout/retry/scadenze | SUPERATO | `build/tests.log` |
| T05 RTT/varianza/rollover | SUPERATO | `build/tests.log` |
| T06 simulazione perdita ACK/allarme | SUPERATO | `build/tests.log` |
| T07 CSV (2 casi Python) | SUPERATO | `build/tests.log` |
| B-TX esempio default | SUPERATO | `build/build-tx-default.log` |
| B-RX esempio default | SUPERATO | `build/build-rx-default.log` |
| B-TX con config temporanea | SUPERATO | `build/build-tx-configured.log` |
| B-RX con config temporanea | SUPERATO | `build/build-rx-configured.log` |
| R01/R02 revisione sorgenti, guide, matrice | COMPLETATA | 36/36 requisiti tracciati; fonti ufficiali in `fonti.md` |
| R03 assenza deploy | VERIFICATA nella sessione | Nessun upload, erase, OTA o accesso seriale eseguito |

C++11 compilato con warning trattati come errori e UndefinedBehaviorSanitizer;
sei gruppi di casi C++, più due casi Python. Sono test della logica, non prove del
sistema radio, della cifratura reale, del timing FreeRTOS o delle schede.

Memoria stimata dal linker negli esempi finali: TX RAM 44044 byte, programma 738601 byte;
RX RAM 43988 byte, programma 737413 byte. Questi numeri non identificano la flash fisica
né costituiscono misura di RAM dinamica durante l'esercizio.

Problemi incontrati e risolti: download iniziali bloccati da DNS/rete della sandbox
(ripetuti con accesso autorizzato); dipendenza WiFi non individuata da LDF (include
esplicito nei main); collisione namespace con funzione C `alarm` (namespace `cantiere`).
Un'asserzione iniziale sull'accumulatore a 64 bit aveva un valore atteso errato ed è
stata corretta. Nessuno di questi fallimenti intermedi è presentato come test superato.
Esptool 4.5.1 ha emesso un SyntaxWarning Python sul proprio codice durante la prima
build; conversione immagini e build finali sono riuscite.

## Da verificare sul dispositivo

H00–H10: **NON ESEGUITI, hardware ancora da provare**. Modello preciso della seconda
scheda e caratteristiche di entrambe non confermati; nessuna porta aperta né chip
interrogato. MAC effettivi, flash, alimentazione, peer e chiavi operative da predisporre.
Nessuna misura di portata, perdita reale, RTT reale, durata/autonomia o RSSI disponibile.
Nessun firmware caricato. I binari finali sono esempi `CONFIG_REQUIRED`, non configurazioni
operative. I file locali temporanei delle verifiche sono stati rimossi dallo script;
nessuna chiave reale è stata richiesta o inserita nel progetto versionabile.

## Pianificato

Fase 2 sensori/centrale/sirena; fase 3 ponte LoRa; fase 4 Home Assistant/notifiche.
Tutti i test P201–P403 sono **PIANIFICATI**, non implementati e non eseguiti.
Non è un impianto certificato; il prototipo non dà garanzie anti-replay durevoli.

## Passi successivi

1. Confermare inventario delle due schede, assegnazione TX/RX e porte; verificare chip
   e flash nella futura sessione hardware, senza dedurli dalla DevKit a 30 pin.
2. Preparare MAC peer, chiavi locali e parametri, ricompilare entrambi.
3. Solo su richiesta esplicita: mostrare immagine/destinazione, avvisare della
   sostituzione, valutare backup leggibile e poi flash autorizzato.
4. Eseguire H01/H02 vicini, poi guasti/riavvii e campagne campo/prolungate;
   compilare la scheda risultati e decidere se procedere alla fase 2.

## Correzione successiva alla revisione — firmware 0.1.1

Corretto il riepilogo CSV (28 campi, serializzatore condiviso e test sull'output reale).
Separato il sensore TX opzionale dalla build RX; rimossi pulseIn e GPIO impliciti.
Driver a interrupt con timeout, inizializzazione pin esplicita, evento pendente durante
BUSY e cooldown solo dopo avvio transazione. Sensore disabilitato di default;
vedere [guida sensore](sensore-ultrasuoni.md). Configurazioni locali MAC/chiavi preservate.

Il precedente tentativo di upload dei due firmware operativi è stato rifiutato dal
controllo automatico per limite d'uso: nessuno dei due upload è stato eseguito da quel
comando. È disponibile il backup RX completo e verificato in
`backups/esp32-d4d4dae43fa4-first-flash/original-retry.bin`.
La scheda TX era stata caricata con il firmware diagnostico 0.1.0 nella sessione precedente.
Questa correzione riguarda sorgenti e build sul PC; nessun flash eseguito.

Esito correzione: build TX, RX e sensor_compile **SUPERATE**; sei gruppi protocollo,
assert del sensore e tre casi Python **SUPERATI**. Log in `build/fix-*.log`.
Nessuna prova radio o del sensore svolta durante la correzione.

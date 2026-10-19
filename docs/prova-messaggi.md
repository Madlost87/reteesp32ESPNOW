# Messaggi testuali tra ESP32 — firmware 0.6.0

## Uso

- **Nodo TX:** http://192.168.1.117/
- **Centrale RX:** http://192.168.1.176/radio (link anche nella dashboard `/`).

Scrivere nel campo **Messaggio** e premere **Invia messaggio**. Sull'altra pagina
il testo compare nel riquadro **Ultimo messaggio ricevuto**. Per rispondere, scrivere
nel campo della seconda scheda e inviare. Ogni pagina mostra anche l'ultimo testo
inviato e l'esito: in coda, attesa conferma, ricevuto dall'altra scheda o nessuna conferma.
La conferma indica ricezione sulla ESP32, non lettura da parte di una persona.

Fino a 48 caratteri su una riga, con accenti ed emoji. Il campo HTML usa maxlength=48
(unità UTF-16: alcune emoji occupano due posizioni). Il firmware valida UTF-8 fino a
48 codepoint / 192 byte. Testi vuoti, solo spazi, troppo lunghi o con controlli sono
rifiutati. Il contenuto è visualizzato come testo: tag HTML non vengono eseguiti.

Vengono conservati solo **l'ultimo testo inviato e l'ultimo ricevuto**, non una chat
persistente. Contatori e testi si azzerano al riavvio. I collegamenti all'altra scheda
usano mDNS; se non risolto, aprire direttamente gli IP sopra. Gli IP sono DHCP.

## Percorso del messaggio

Browser → HTTP della scheda locale → ESP-NOW unicast cifrato → altra ESP32 → ACK
applicativo ESP-NOW → esito HTTP del mittente. Il testo è incluso nel pacchetto radio,
non trasferito tra ESP32 mediante HTTP. Invii bidirezionali simultanei supportati;
ACK prioritari, retry e deduplicazione limitata come nel protocollo condiviso.
Un solo messaggio manuale pendente per scheda; un nuovo invio durante l'attesa riceve 409.

Firmware 0.6.0 usa **protocollo v3, 244 byte**, entro il limite ESP-NOW di 250 byte
dello stack locale. **Aggiornare entrambe le schede**: le versioni precedenti non sono
compatibili durante l'aggiornamento. OTA usa Wi-Fi separatamente e rimane disponibile.
Il sensore è disabilitato; diagnosi chiusa come non risolta su richiesta dell'utente.
Il TX continua Probe ogni 2 s, esclusi dal contatore dei messaggi testuali.

## API e sorgenti

- GET `/api/radio-test`: stato, sequenze, conteggio, `sent_text`, `received_text`.
- POST `/api/radio-test`: corpo testo UTF-8, header `X-Radio-Test: 1`, preferibilmente
  Content-Type `text/plain; charset=utf-8`. 202 = accodato, non ancora ricevuto.
- 400 per input errato; 409 per invio già in corso. Nessun CORS abilitato.
- HTTP senza login/TLS: i client nella LAN possono leggere e inviare testi. L'header
  custom blocca i form cross-origin, non autentica il mittente HTTP. Usare LAN fidata/VPN.

`shared/radio_test.hpp`: rotte e scambio tra task HTTP e loop radio.
`shared/message_text.hpp`: limiti e validazione UTF-8.
`shared/protocol.hpp`: serializzazione radio, testo incluso anche nell'ACK.
`shared/web/radio.html`: pagina; rigenerare con `python3 tools/embed_web.py`.

## Verifiche

```sh
./tools/test.sh
node tests/test_radio_page.js
node tests/test_dashboard.js
python3 tools/check_radio_web.py --report results/messaggi-nuova-prova.json
```

L'ultima prova richiede le schede accese. Invia dieci coppie simultanee e confronta
il testo esatto sul destinatario con quello del mittente, incluse virgolette, backslash,
accenti, tag HTML letterali ed emoji al limite. Verifica anche input non validi e
assenza di allarmi. I test JavaScript usano DOM simulato, non un browser reale.

Prova storica 0.5.0 (messaggio fisso senza testo): 20/20 messaggi confermati,
report `results/radio-v050/test.json`. Non è la prova del nuovo trasporto testuale.

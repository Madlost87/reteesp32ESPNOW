#pragma once
// Copiare in espnow_tx/include/config.local.hpp e espnow_rx/include/config.local.hpp.
// Gli zeri NON sono chiavi operative: l'applicazione rifiuta di avviare ESP-NOW.
namespace cfg {
constexpr uint8_t peerMac[6] = {0,0,0,0,0,0}; // MAC STA dell'ALTRA scheda
constexpr uint8_t pmk[16] = {0};
constexpr uint8_t lmk[16] = {0};
constexpr uint32_t node = ROLE_TX ? 1 : 2;
constexpr uint32_t peerNode = ROLE_TX ? 2 : 1;
constexpr uint8_t channel = 6; // stesso canale su entrambe, intervallo ammesso dal prototipo 1..11
constexpr uint32_t intervalMs = 2000;
constexpr uint32_t timeoutMs = 250;
constexpr uint8_t retries = 3; // totale massimo 4 chiamate di invio per originale
constexpr uint32_t summaryMs = 10000;
constexpr bool periodicAtBoot = true;
}

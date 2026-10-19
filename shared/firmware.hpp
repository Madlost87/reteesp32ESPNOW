#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <atomic>
#include <ArduinoOTA.h>
#include <esp_ota_ops.h>
#include <esp_http_server.h>
#include <ctype.h>
#include "radio_page.hpp"
#include "protocol.hpp"
#include "telemetry.hpp"
#include "central_state.hpp"
#if !ROLE_TX
#include <esp_http_server.h>
#include <freertos/semphr.h>
#include "web_page.hpp"
#endif
#if __has_include("network.local.hpp")
#include "network.local.hpp"
#else
#include "network.example.hpp"
#endif
#if __has_include("ota.local.hpp")
#include "ota.local.hpp"
#else
#include "ota.example.hpp"
#endif
#if __has_include("config.local.hpp")
#include "config.local.hpp"
#else
#include "config.example.hpp"
#endif
#if !CONFIG_IDF_TARGET_ESP32
#error Questo firmware richiede ESP32 classico
#endif
static_assert(cfg::channel>=1 && cfg::channel<=11, "Canale richiesto 1..11");
static_assert(cfg::node && cfg::peerNode && cfg::node!=cfg::peerNode, "Identificativi distinti non nulli");
static_assert(cfg::timeoutMs>=50 && cfg::timeoutMs<=60000 && cfg::retries<=cantiere::MaxRetries, "Timeout/retry fuori limite");
static_assert(cfg::intervalMs>=100 && cfg::intervalMs<=86400000 && cfg::summaryMs>=1000 && cfg::summaryMs<=86400000, "Intervalli fuori limite");
#ifndef ENABLE_ULTRASONIC
#define ENABLE_ULTRASONIC 0
#endif
#if ENABLE_ULTRASONIC && !ROLE_TX
#error Ultrasonic sensor is supported only on TX
#endif
#if ROLE_TX && ENABLE_ULTRASONIC
#include "ultrasonic.hpp"
#endif
namespace app {
using namespace cantiere;
constexpr const char* role=ROLE_TX ? "TX" : "RX";
constexpr const char* version="0.6.0";
struct Incoming { uint8_t mac[6]; uint8_t bytes[WireSize]; };
QueueHandle_t incoming=nullptr, outgoing=nullptr;
std::atomic<uint32_t> queueDrops{0}, badLengths{0};
// Exactly one ESP-NOW send in flight: callback result cannot be confused with the next send.
std::atomic<int> radioResult{-1};
bool ready=false, radioBusy=false, periodic=cfg::periodicAtBoot;
uint32_t radioStart=0, lastPeriodic=0, lastSummary=0, sequence=0;
uint64_t session=0, wideNow=0;
uint32_t lastTelemetry=0;
Clock clock;
Transaction tx;
Deduplicator received;
Message radioMessage, lastConfirmed;
bool haveConfirmed=false;
uint64_t rxOriginals=0, duplicates=0, stale=0, unmatched=0, invalid=0;
uint64_t macOk=0, macFail=0, sendErrors=0, ackQueued=0, ackDropped=0;
Metrics rxMetrics;
CentralState central;
#include "lan.hpp"
#include "radio_test.hpp"
#include "remote_update.hpp"
#if !ROLE_TX
#include "central_web.hpp"
#endif
void onReceive(const uint8_t* mac, const uint8_t* data, int len) {
  if (len!=int(WireSize)) { ++badLengths; return; }
  Incoming in; memcpy(in.mac,mac,6); memcpy(in.bytes,data,WireSize);
  if (xQueueSend(incoming,&in,0)!=pdTRUE) ++queueDrops;
}
void onSent(const uint8_t*, esp_now_send_status_t status) { radioResult.store(int(status)); }
void record(const char* type, const Message& m, long value=0) {
  Serial.printf("E,%llu,%s,%s,%lu,%016llx,%lu,%u,%u,%016llx,%016llx,%ld\n",
    (unsigned long long)wideNow,role,type,(unsigned long)cfg::node,(unsigned long long)session,
    (unsigned long)m.seq,unsigned(m.event),unsigned(m.attempt),
    (unsigned long long)m.session,(unsigned long long)m.refSession,value);
}
void summary() {
  const Metrics& s=tx.metrics;
  Serial.printf("# %s originali=%llu confermati=%llu senza_conferma=%llu duplicati=%llu\n",role,
    (unsigned long long)(ROLE_TX?s.originals:rxOriginals),(unsigned long long)s.confirmed,
    (unsigned long long)s.unconfirmed,(unsigned long long)duplicates);
  Diagnostics diagnostics{rxOriginals,duplicates,stale,unmatched,invalid+badLengths.load(),
    macOk,macFail,sendErrors,queueDrops.load(),ackQueued,ackDropped};
  writeSummary(Serial,wideNow,role,session,s,diagnostics);
}
bool check(esp_err_t err, const char* name) {
  if (err==ESP_OK) return true;
  Serial.printf("# FATAL %s: %s (%d); correggere e riavviare\n",name,esp_err_to_name(err),int(err));
  ready=false; return false;
}
bool nonzero(const uint8_t* p,size_t n) { for(size_t i=0;i<n;++i) if(p[i]) return true; return false; }
void send(const Message& m,uint32_t now) {
  uint8_t bytes[WireSize]; Message wire=m;
  if(ROLE_TX && wire.kind==Kind::Data && wire.sampleAgeMs!=UINT32_MAX)
    wire.sampleAgeMs+=elapsed(now,tx.sent[0]);
  encode(wire,bytes);
  radioMessage=m; radioResult.store(-1); radioBusy=true; radioStart=now;
  esp_err_t err=esp_now_send(cfg::peerMac,bytes,sizeof(bytes));
  if (err!=ESP_OK) {
    radioBusy=false; ++sendErrors; record("send_error",m,err);
    Serial.printf("# invio rifiutato: %s; non e una conferma applicativa\n",esp_err_to_name(err));
  } else record(m.kind==Kind::Ack?"ack_send":"data_send",m);
}
void handle(const Incoming& in,uint32_t now) {
  Message m;
  if (memcmp(in.mac,cfg::peerMac,6) || !decode(in.bytes,WireSize,m) || m.node!=cfg::peerNode) { ++invalid; return; }
  if (m.kind==Kind::Ack) {
    if (m.kind!=Kind::Ack || m.refNode!=cfg::node || m.refSession!=session) { ++invalid; return; }
    if (tx.acknowledge(m,cfg::peerNode,now,wideNow)) {
      if(m.event==Event::RadioTest)radioTest::state.store(3);
      lastConfirmed=m; haveConfirmed=true; record("confirmed",m,elapsed(now,tx.sent[m.attempt]));
    } else if (haveConfirmed && m.refSeq==lastConfirmed.refSeq && m.event==lastConfirmed.event) {
      ++duplicates; tx.metrics.touch(wideNow); record("ack_duplicate",m);
    } else { ++unmatched; record("ack_unmatched",m); }
  } else {
    if (m.kind!=Kind::Data || (ROLE_TX && m.event!=Event::RadioTest)) { ++invalid; return; }
    rxMetrics.touch(wideNow);
    Reception r=received.observe(m.node,m.session,m.seq);
    if (r==Reception::Fresh) { radioTest::receive(m); if(m.event!=Event::RadioTest)central.receive(m,wideNow); ++rxOriginals; record(m.event==Event::RadioTest?"radio_test":m.event==Event::Alarm?"alarm_simulated":m.event==Event::SensorAlarm?"sensor_alarm":m.event==Event::SensorClear?"sensor_clear":m.event==Event::Telemetry?"telemetry":"probe",m); }
    else if (r==Reception::Duplicate) { ++duplicates; record("data_duplicate",m); }
    else { ++stale; record("data_stale",m); }
    Message ack=reply(m,cfg::node,session);
    if (xQueueSend(outgoing,&ack,0)==pdTRUE) ++ackQueued; else ++ackDropped;
  }
}
bool beginMessage(Event event,uint32_t now,bool transition=false) {
  if (!ready || tx.active || radioBusy) { Serial.println("# BUSY oppure radio non pronta: comando non accodato"); return false; }
  if (sequence==UINT32_MAX) {
    // A sequence never wraps within a session.
    uint64_t old=session;
    do { session=(uint64_t(esp_random())<<32)|esp_random(); } while (!session || session==old);
    sequence=0; haveConfirmed=false; Serial.println("# nuova sessione per esaurimento sequenza");
  }
  Message m; m.node=cfg::node; m.session=session; m.seq=++sequence; m.event=event;
  if(event==Event::RadioTest)radioTest::outgoingText(m.text);
#if ROLE_TX && ENABLE_ULTRASONIC
  (transition?ultrasonic::alarm.front().reading:ultrasonic::alarm.reading()).apply(m,now);
#else
  (void)transition;
#endif
  if (!tx.start(m,now,cfg::timeoutMs,cfg::retries)) return false;
  send(tx.data,now);
  return true;
}
void command(const char* line,uint32_t now) {
  if (!strcmp(line,"stats")) summary();
  else if (!strcmp(line,"help")) Serial.println("# help | stats | alarm | test on | test off (comandi TX; terminare con invio)");
  else if (ROLE_TX && !strcmp(line,"alarm")) beginMessage(Event::Alarm,now);
  else if (ROLE_TX && !strcmp(line,"test on")) { periodic=true; lastPeriodic=now; Serial.println("# periodico attivo"); }
  else if (ROLE_TX && !strcmp(line,"test off")) { periodic=false; Serial.println("# prove periodiche ferme; telemetria sensore e allarmi restano attivi"); }
  else Serial.println("# comando sconosciuto o non previsto per questo ruolo");
}
void serialInput(uint32_t now) {
  static char line[48]; static size_t length=0; static bool overflow=false;
  for (int i=0;i<64 && Serial.available();++i) {
    char c=Serial.read(); if(c=='\r') continue;
    if(c=='\n') { if(!overflow) {line[length]=0; command(line,now);} else Serial.println("# comando troppo lungo"); length=0; overflow=false; }
    else if(length+1<sizeof(line)) line[length++]=c; else overflow=true;
  }
}
void setupApp() {
  Serial.begin(115200);
  Serial.printf("\n# ruolo=%s firmware=%s protocollo=%u chip=%s sdk=%s\n",role,version,Version,ESP.getChipModel(),ESP.getSdkVersion());
  WiFi.persistent(false); WiFi.setAutoReconnect(false);
  if (!WiFi.setSleep(false)) { Serial.println("# FATAL WiFi.setSleep"); return; }
  if (!WiFi.mode(WIFI_STA)) { Serial.println("# FATAL WiFi.mode"); return; }
  if (!check(esp_wifi_disconnect(),"disconnect") || !check(esp_wifi_set_ps(WIFI_PS_NONE),"power_save") ||
      !check(esp_wifi_set_channel(cfg::channel,WIFI_SECOND_CHAN_NONE),"set_channel")) return;
  uint8_t mac[6], channel=0; wifi_second_chan_t secondary;
  if (!check(esp_wifi_get_mac(WIFI_IF_STA,mac),"get_mac") || !check(esp_wifi_get_channel(&channel,&secondary),"get_channel")) return;
  Serial.printf("# MAC_STA=%02X:%02X:%02X:%02X:%02X:%02X canale=%u nodo=%lu intervallo=%lu timeout=%lu retry=%u riepilogo=%lu\n",
    mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],channel,(unsigned long)cfg::node,
    (unsigned long)cfg::intervalMs,(unsigned long)cfg::timeoutMs,cfg::retries,(unsigned long)cfg::summaryMs);
  do {session=(uint64_t(esp_random())<<32)|esp_random();} while(!session);
  Serial.printf("# sessione=%016llx RSSI=non_disponibile\n",(unsigned long long)session);
  if (!nonzero(cfg::peerMac,6) || (cfg::peerMac[0]&1) || !memcmp(mac,cfg::peerMac,6) ||
      !nonzero(cfg::pmk,16) || !nonzero(cfg::lmk,16)) {
    Serial.println("# CONFIG_REQUIRED: impostare MAC peer e chiavi locali non nulle; radio ESP-NOW non avviata"); return;
  }
  incoming=xQueueCreate(16,sizeof(Incoming)); outgoing=xQueueCreate(16,sizeof(Message));
  if(!incoming || !outgoing) { Serial.println("# FATAL creazione code"); return; }
  if (!check(esp_now_init(),"esp_now_init") || !check(esp_now_set_pmk(cfg::pmk),"set_pmk")) return;
  esp_now_peer_info_t peer{}; memcpy(peer.peer_addr,cfg::peerMac,6); memcpy(peer.lmk,cfg::lmk,16);
  peer.channel=cfg::channel; peer.ifidx=WIFI_IF_STA; peer.encrypt=true;
  if(!check(esp_now_add_peer(&peer),"add_peer") || !check(esp_now_register_recv_cb(onReceive),"recv_cb") ||
     !check(esp_now_register_send_cb(onSent),"send_cb")) return;
#if ROLE_TX && ENABLE_ULTRASONIC
  ultrasonic::setup();
  Serial.println("# sensore ultrasuoni abilitato; pin configurati esplicitamente");
#endif
  ready=true; Serial.println("# READY cifratura=CCMP; help per i comandi");
  remote::begin();lan::begin();
#if !ROLE_TX
  web::begin();
#endif
}
void loopApp() {
  uint32_t now=millis(); wideNow=clock.update(now); serialInput(now);
  if(ready) {
#if ROLE_TX && ENABLE_ULTRASONIC
    ultrasonic::poll(now);
#endif
    if(radioBusy) {
      int result=radioResult.exchange(-1);
      if(result!=-1) { radioBusy=false; if(result==ESP_NOW_SEND_SUCCESS) ++macOk; else ++macFail; record("mac_result",radioMessage,result); }
      else if(elapsed(now,radioStart)>=2000) { ready=false; Serial.println("# FATAL callback invio assente: prova invalida, riavviare"); }
    }
    Incoming in;
    for(int i=0;i<16 && xQueueReceive(incoming,&in,0)==pdTRUE;++i) handle(in,now);
    if(ready && !radioBusy) {
      // ACKs have priority, including when both boards initiated a test.
      Message ack;
      if(xQueueReceive(outgoing,&ack,0)==pdTRUE)send(ack,now);
      if(!radioBusy) {
        Tick action=tx.tick(now);
        if(action==Tick::Retry){record("timeout_retry",tx.data);send(tx.data,now);}
        if(action==Tick::Failed){record("unconfirmed",tx.data);if(tx.data.event==Event::RadioTest)radioTest::state.store(4);}
        if(!radioBusy && !tx.active && radioTest::state.load()==1) {
          if(beginMessage(Event::RadioTest,now)){radioTest::sentSeq.store(tx.data.seq);radioTest::state.store(2);}
        }
#if ROLE_TX && ENABLE_ULTRASONIC
        if(!radioBusy && !tx.active && ultrasonic::alarm.pending() && beginMessage(ultrasonic::alarm.front().event,now,true))ultrasonic::alarm.accepted();
        if(!radioBusy && !tx.active && elapsed(now,lastTelemetry)>=250 && beginMessage(Event::Telemetry,now))lastTelemetry=now;
#elif ROLE_TX
        if(!radioBusy && !tx.active && periodic && elapsed(now,lastPeriodic)>=cfg::intervalMs){lastPeriodic=now;beginMessage(Event::Probe,now);}
#endif
      }
    }
  }
  if(elapsed(now,lastSummary)>=cfg::summaryMs) {lastSummary=now; summary();}
  #if !ROLE_TX
  web::tick(now);
  #endif
#if ROLE_TX && ENABLE_ULTRASONIC
  static uint32_t reportedDrops=0;
  if(ultrasonic::alarm.drops()!=reportedDrops){reportedDrops=ultrasonic::alarm.drops();Serial.printf("# sensor_event_drops=%lu\n",(unsigned long)reportedDrops);}
#endif
  lan::tick(millis());
  remote::tick();
  delay(1);
}
} // namespace app
void setup() { app::setupApp(); }
void loop() { app::loopApp(); }

#pragma once
// Included inside namespace app, only for RX; HTTP serves snapshots from a separate task.
namespace web {
httpd_handle_t server=nullptr;
SemaphoreHandle_t snapshotMutex=nullptr;
constexpr size_t Capacity=6144;
char snapshot[Capacity]="{\"starting\":true}";
uint32_t lastSnapshot=0;
bool enabled=false;

void headers(httpd_req_t* req,const char* type) {
  httpd_resp_set_type(req,type);
  httpd_resp_set_hdr(req,"Cache-Control","no-store");
  httpd_resp_set_hdr(req,"X-Content-Type-Options","nosniff");
  httpd_resp_set_hdr(req,"Content-Security-Policy","default-src 'self'; script-src 'self' 'unsafe-inline'; style-src 'self' 'unsafe-inline'; frame-ancestors 'none'");
}
esp_err_t page(httpd_req_t* req) {
  headers(req,"text/html; charset=utf-8");
  return httpd_resp_send(req,centralPage,sizeof(centralPage)-1);
}
esp_err_t status(httpd_req_t* req) {
  char* copy=static_cast<char*>(malloc(Capacity));
  if(!copy) return httpd_resp_send_err(req,HTTPD_500_INTERNAL_SERVER_ERROR,"Memory unavailable");
  if(xSemaphoreTake(snapshotMutex,pdMS_TO_TICKS(50))!=pdTRUE) {
    free(copy);httpd_resp_set_status(req,"503 Service Unavailable");return httpd_resp_send(req,"Busy",HTTPD_RESP_USE_STRLEN);
  }
  memcpy(copy,snapshot,Capacity);
  xSemaphoreGive(snapshotMutex);
  headers(req,"application/json; charset=utf-8");
  esp_err_t result=httpd_resp_send(req,copy,HTTPD_RESP_USE_STRLEN);
  free(copy);return result;
}
void publish() {
  uint8_t channel=0;wifi_second_chan_t secondary;
  esp_wifi_get_channel(&channel,&secondary);
  wifi_ps_type_t ps=WIFI_PS_NONE;esp_wifi_get_ps(&ps);
  String json; if(!json.reserve(Capacity)) return;
  char prefix[1100];
  const bool online=CentralState::online(rxMetrics.seen,rxMetrics.lastValid,wideNow);
  snprintf(prefix,sizeof(prefix),
    "{\"wifi_power_save\":%d,\"version\":\"%s\",\"uptime_ms\":%llu,\"radio_ready\":%s,\"channel\":%u,\"channel_ok\":%s,"
    "\"node_online\":%s,\"last_contact_ms\":%lld,\"received\":%llu,\"alarm_count\":%llu,"
    "\"duplicates\":%llu,\"errors\":%llu,\"queue_drops\":%llu,\"wifi_rssi\":%ld,\"ip\":\"%s\",\"sensor_state\":%u,\"distance_mm\":%d,\"sample_age_ms\":%lld,\"events\":[",
    int(ps),version,(unsigned long long)wideNow,ready?"true":"false",channel,channel==cfg::channel?"true":"false",
    online?"true":"false",rxMetrics.seen?(long long)(wideNow-rxMetrics.lastValid):-1LL,
    (unsigned long long)rxOriginals,(unsigned long long)central.total,(unsigned long long)duplicates,
    (unsigned long long)(sendErrors+invalid+badLengths.load()),(unsigned long long)(queueDrops.load()+ackDropped),
    (long)WiFi.RSSI(),WiFi.localIP().toString().c_str(),unsigned(central.sensor),central.distanceMm==UINT16_MAX?-1:int(central.distanceMm),(long long)central.age(wideNow));
  json+=prefix;

  for(size_t i=0;i<central.count;++i) {
    const auto& e=central.latest(i);char event[220];
    snprintf(event,sizeof(event),"%s{\"node\":%lu,\"seq\":%lu,\"session\":\"%016llx\",\"age_ms\":%llu,\"kind\":%u}",
      i?",":"",(unsigned long)e.node,(unsigned long)e.seq,(unsigned long long)e.session,
      (unsigned long long)(wideNow-e.at),unsigned(e.event));
    json+=event;
  }
  json+="]}";
  if(json.length()>=Capacity) return;
  if(xSemaphoreTake(snapshotMutex,0)==pdTRUE) {
    memcpy(snapshot,json.c_str(),json.length()+1);xSemaphoreGive(snapshotMutex);
  }
}
void begin() {
  uint8_t any=0;for(auto b:network::bssid)any|=b;
  if(!network::ssid[0] || !any) {Serial.println("# WEB_CONFIG_REQUIRED: configurare network.local.hpp");return;}
  snapshotMutex=xSemaphoreCreateMutex();
  if(!snapshotMutex) {Serial.println("# WEB_ERROR: mutex");return;}
  httpd_config_t config=HTTPD_DEFAULT_CONFIG();
  config.max_open_sockets=3;config.lru_purge_enable=true;
  config.recv_wait_timeout=2;config.send_wait_timeout=2;
  if(httpd_start(&server,&config)!=ESP_OK) {Serial.println("# WEB_ERROR: avvio HTTP");return;}
  httpd_uri_t root{};root.uri="/";root.method=HTTP_GET;root.handler=page;
  httpd_uri_t api{};api.uri="/api/status";api.method=HTTP_GET;api.handler=status;
  if(httpd_register_uri_handler(server,&root)!=ESP_OK || httpd_register_uri_handler(server,&api)!=ESP_OK) {
    httpd_stop(server);server=nullptr;Serial.println("# WEB_ERROR: rotte HTTP");return;
  }
  remote::registerStatus(server);
  enabled=true;publish();
}
void tick(uint32_t now) {
  if(!enabled)return;
  if(elapsed(now,lastSnapshot)>=100) {lastSnapshot=now;publish();}
}
}

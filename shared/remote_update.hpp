#pragma once
// Included inside namespace app. OTA credentials are distinct for each board.
namespace remote {
std::atomic<bool> active{false},listening{false};
std::atomic<int> lastError{-1};
httpd_handle_t deviceServer=nullptr;
char identity[500];
esp_err_t device(httpd_req_t* req) {
  char json[700];
  snprintf(json,sizeof(json),"{%s,\"uptime_ms\":%lu,\"ota_ready\":%s,\"ota_active\":%s,\"ota_error\":%d}",identity,
    (unsigned long)millis(),listening.load()?"true":"false",active.load()?"true":"false",lastError.load());
  httpd_resp_set_type(req,"application/json");httpd_resp_set_hdr(req,"Cache-Control","no-store");
  return httpd_resp_send(req,json,HTTPD_RESP_USE_STRLEN);
}
#if ROLE_TX && ENABLE_ULTRASONIC
esp_err_t sensorDiagnostics(httpd_req_t* req) {
  const auto d=ultrasonic::snapshot();char json[550];
  snprintf(json,sizeof(json),"{\"role\":\"TX\",\"trigger_pin\":%d,\"echo_pin\":%d,\"echo_level\":%d,\"triggers\":%lu,\"rising_edges\":%lu,\"falling_edges\":%lu,\"completed_pulses\":%lu,\"timeouts_no_rise\":%lu,\"timeouts_no_fall\":%lu,\"last_pulse_us\":%lu}",
    SENSOR_TRIGGER_PIN,SENSOR_ECHO_PIN,digitalRead(SENSOR_ECHO_PIN),(unsigned long)d.triggers,
    (unsigned long)d.rises,(unsigned long)d.falls,(unsigned long)d.completed,
    (unsigned long)d.noRise,(unsigned long)d.noFall,(unsigned long)d.lastWidth);
  httpd_resp_set_type(req,"application/json");httpd_resp_set_hdr(req,"Cache-Control","no-store");
  return httpd_resp_send(req,json,HTTPD_RESP_USE_STRLEN);
}
#endif
void registerStatus(httpd_handle_t server) {
  radioTest::routes(server);
  httpd_uri_t uri{};uri.uri="/api/device";uri.method=HTTP_GET;uri.handler=device;
  if(httpd_register_uri_handler(server,&uri)!=ESP_OK)Serial.println("# OTA_ERROR rotta stato");
#if ROLE_TX && ENABLE_ULTRASONIC
  httpd_uri_t sensor{};sensor.uri="/api/sensor";sensor.method=HTTP_GET;sensor.handler=sensorDiagnostics;
  if(httpd_register_uri_handler(server,&sensor)!=ESP_OK)Serial.println("# SENSOR_ERROR rotta diagnostica");
#endif
}
void begin() {
  const esp_partition_t* slot=esp_ota_get_running_partition();
  snprintf(identity,sizeof(identity),"\"image_target\":\"%s\",\"role\":\"%s\",\"mac\":\"%s\",\"version\":\"%s\",\"build\":\"%s %s\",\"partition\":\"%s\",\"boot_id\":\"%016llx\"",
    ROLE_TX?"CANTIERE_OTA_ROLE_TX_V1":"CANTIERE_OTA_ROLE_RX_V1",role,WiFi.macAddress().c_str(),version,__DATE__,__TIME__,slot?slot->label:"unknown",(unsigned long long)session);
#if ROLE_TX
  httpd_config_t config=HTTPD_DEFAULT_CONFIG();config.max_open_sockets=2;config.lru_purge_enable=true;
  if(httpd_start(&deviceServer,&config)==ESP_OK)registerStatus(deviceServer);
#endif
  // Empty template must never expose an unauthenticated updater.
  if(strlen(ota_config::passwordHash)!=32){Serial.println("# OTA_CONFIG_REQUIRED");return;}
  for(const char* p=ota_config::passwordHash;*p;++p)if(!isxdigit(*p)){Serial.println("# OTA_CONFIG_INVALID");return;}
  ArduinoOTA.setHostname(ROLE_TX?"cantiere-tx":"cantiere-rx");
  ArduinoOTA.setPasswordHash(ota_config::passwordHash);ArduinoOTA.setPort(3232);
  ArduinoOTA.setTimeout(5000);
  ArduinoOTA.onStart([](){
    if(ArduinoOTA.getCommand()!=U_FLASH){Update.abort();return;}
    active.store(true);lastError.store(-1);Serial.println("# OTA_START manutenzione: radio/sensore sospesi durante scrittura");
  });
  ArduinoOTA.onEnd([](){Serial.println("# OTA_END riavvio");});
  ArduinoOTA.onError([](ota_error_t error){Update.abort();active.store(false);lastError.store(int(error));Serial.printf("# OTA_ERROR %u\n",unsigned(error));});
}
void tick() {
  if(strlen(ota_config::passwordHash)!=32)return;
  for(const char* p=ota_config::passwordHash;*p;++p)if(!isxdigit(*p))return;
  const bool connected=WiFi.status()==WL_CONNECTED && WiFi.channel()==cfg::channel;
  if(!connected){if(listening.exchange(false))ArduinoOTA.end();return;}
  if(!listening.load()){ArduinoOTA.begin();listening.store(true);Serial.println("# OTA_READY porta=3232 autenticazione obbligatoria");}
  // handle() blocks during transfer; do not suspend an outgoing transaction.
  if(!radioBusy && !tx.active)ArduinoOTA.handle();
}
}

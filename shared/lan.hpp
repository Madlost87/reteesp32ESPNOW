#pragma once
// Shared fixed-channel LAN connection; never wait for association in the radio loop.
namespace lan {
uint32_t attemptAt=0;bool connecting=false,wasConnected=false,enabled=false;
void connect(uint32_t now) {
  attemptAt=now;connecting=true;
  WiFi.begin(network::ssid,network::password,cfg::channel,network::bssid,true);
}
void begin() {
  uint8_t any=0;for(auto b:network::bssid)any|=b;
  enabled=network::ssid[0] && any;
  if(enabled)connect(millis());
  else Serial.println("# LAN_CONFIG_REQUIRED: network.local.hpp");
}
void tick(uint32_t now) {
  if(!enabled)return;
  bool connected=WiFi.status()==WL_CONNECTED;
  if(connected) {
    connecting=false;
    if(!wasConnected)Serial.printf("# LAN_READY %s http://%s/ canale=%d\n",role,WiFi.localIP().toString().c_str(),WiFi.channel());
  } else {
    if(wasConnected || (connecting && elapsed(now,attemptAt)>=10000)) {
      WiFi.disconnect(false,false);esp_wifi_set_channel(cfg::channel,WIFI_SECOND_CHAN_NONE);
      attemptAt=now;connecting=false;
    }
    if(!connecting && elapsed(now,attemptAt)>=30000)connect(now);
  }
  wasConnected=connected;
}
}

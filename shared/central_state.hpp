#pragma once
#include "protocol.hpp"
namespace cantiere {
struct AlarmEntry {uint64_t at=0,session=0;uint32_t node=0,seq=0;Event event=Event::Alarm;};
struct CentralState {
  AlarmEntry alarms[16];size_t count=0,next=0;uint64_t total=0;
  SensorState sensor=SensorState::Disabled;uint16_t distanceMm=UINT16_MAX;
  uint32_t sampleAge=UINT32_MAX,lastSeq=0;uint64_t sampleReceived=0,sensorSession=0;
  uint64_t retired[4]{};size_t retiredNext=0;
  void receive(const Message& m,uint64_t now) {
    bool current=true;
    if(sensorSession && m.session!=sensorSession) {
      for(auto old:retired)if(old==m.session)current=false;
      if(current){retired[retiredNext]=sensorSession;retiredNext=(retiredNext+1)%4;lastSeq=0;}
    }
    if(current && m.seq>lastSeq){sensorSession=m.session;lastSeq=m.seq;sensor=m.sensor;distanceMm=m.distanceMm;sampleAge=m.sampleAgeMs;sampleReceived=now;}
    if(m.event!=Event::Alarm && m.event!=Event::SensorAlarm && m.event!=Event::SensorClear)return;
    AlarmEntry e;e.at=now;e.session=m.session;e.node=m.node;e.seq=m.seq;e.event=m.event;
    alarms[next]=e;next=(next+1)%16;if(count<16)++count;
    if(m.event!=Event::SensorClear)++total;
  }
  const AlarmEntry& latest(size_t index)const{return alarms[(next+15-index)%16];}
  int64_t age(uint64_t now)const{return sampleAge==UINT32_MAX?-1:int64_t(sampleAge+now-sampleReceived);}
  static bool online(bool seen,uint64_t last,uint64_t now){return seen && now>=last && now-last<=10000;}
};
}

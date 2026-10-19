#pragma once
#include "protocol.hpp"
namespace cantiere {
struct SensorReading {
  uint16_t distanceMm=UINT16_MAX;
  SensorState state=SensorState::Unknown;
  uint32_t at=0;bool sampled=false;
  void apply(Message& m,uint32_t now) const {
    m.distanceMm=distanceMm;m.sensor=state;m.sampleAgeMs=sampled?elapsed(now,at):UINT32_MAX;
  }
};
struct SensorTransition {Event event=Event::Telemetry;SensorReading reading;};
class SensorAlarm {
  bool occupied=false,known=false;
  uint8_t nearCount=0,farCount=0;
  SensorTransition queue[8];size_t head=0,count=0;
  SensorReading reading_;
  uint32_t drops_=0;
  void enqueue(Event e) {
    if(count==8){++drops_;return;}
    queue[(head+count)%8].event=e;queue[(head+count)%8].reading=reading_;++count;
  }
 public:
  void observe(uint32_t pulseUs,uint32_t now) {
    reading_.sampled=true;reading_.at=now;
    if(!pulseUs || pulseUs>30000) {
      reading_.distanceMm=UINT16_MAX;reading_.state=SensorState::Unknown;
      nearCount=farCount=0;return;
    }
    reading_.distanceMm=uint16_t((pulseUs*17u+50u)/100u);
    if(reading_.distanceMm<200) {if(nearCount<3)++nearCount;farCount=0;}
    else if(reading_.distanceMm>=250) {if(farCount<3)++farCount;nearCount=0;}
    else {nearCount=farCount=0;}
    bool enter=false,leave=false;
    if(nearCount==3) {enter=!occupied;occupied=true;known=true;}
    if(farCount==3) {leave=occupied;occupied=false;known=true;}
    reading_.state=known?(occupied?SensorState::Present:SensorState::Clear):SensorState::Unknown;
    // While confirming initial state, a valid distance may still be shown.
    if(enter)enqueue(Event::SensorAlarm);
    if(leave)enqueue(Event::SensorClear);
  }
  const SensorReading& reading()const{return reading_;}
  bool pending()const{return count!=0;}
  const SensorTransition& front()const{return queue[head];}
  void accepted(){if(count){head=(head+1)%8;--count;}}
  uint32_t drops()const{return drops_;}
};
}

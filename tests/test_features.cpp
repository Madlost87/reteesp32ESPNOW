#include "sensor_logic.hpp"
#include "telemetry.hpp"
#include "central_state.hpp"
#include <cassert>
#include <cstdarg>
#include <cstdio>
struct Output {
  void printf(const char* format, ...) __attribute__((format(printf,2,3))) {
    va_list args; va_start(args,format); vprintf(format,args); va_end(args);
  }
};
int main() {
  using namespace cantiere;
  SensorAlarm alarm;
  alarm.observe(0,0);assert(!alarm.pending() && alarm.reading().distanceMm==UINT16_MAX);
  alarm.observe(500,100);alarm.observe(500,200);assert(!alarm.pending());
  alarm.observe(500,300);assert(alarm.pending() && alarm.front().event==Event::SensorAlarm);
  alarm.observe(500,400);assert(alarm.reading().state==SensorState::Present);
  // Hysteresis band must keep the established state.
  alarm.observe(1300,500);assert(alarm.reading().state==SensorState::Present);
  alarm.observe(2000,600);alarm.observe(2000,700);alarm.observe(2000,800);
  assert(alarm.front().event==Event::SensorAlarm);alarm.accepted();
  assert(alarm.front().event==Event::SensorClear);alarm.accepted();assert(!alarm.pending());
  // No 10-second blind interval: immediately accept a second confirmed entrance.
  alarm.observe(500,900);alarm.observe(500,1000);alarm.observe(500,1100);
  assert(alarm.pending());alarm.accepted();
  alarm.observe(0,1200);assert(alarm.reading().state==SensorState::Unknown);
  alarm.observe(500,1300);alarm.observe(500,1400);alarm.observe(500,1500);
  assert(!alarm.pending()); // missing echo did not silently clear occupancy
  SensorReading reading;reading.sampled=true;reading.at=UINT32_MAX-9;
  Message ageMessage;reading.apply(ageMessage,10);assert(ageMessage.sampleAgeMs==20);
  SensorAlarm noisy;
  for(unsigned i=0;i<20;++i){noisy.observe(500,i*200);noisy.observe(2000,i*200+100);}
  assert(!noisy.pending());
  for(unsigned i=0;i<10;++i){for(unsigned j=0;j<3;++j)noisy.observe(500,i*600+j*100);for(unsigned j=0;j<3;++j)noisy.observe(2000,i*600+300+j*100);}
  assert(noisy.drops()==12);
  CentralState state;
  Message message; message.node=1;message.session=123;message.seq=1;
  state.receive(message,1);assert(state.count==0);
  message.event=Event::Alarm;
  for(uint32_t seq=1;seq<=20;++seq) {message.seq=seq;state.receive(message,seq*100);}
  assert(state.total==20 && state.count==16 && state.latest(0).seq==20 && state.latest(15).seq==5);
  assert(state.latest(0).at==2000 && state.latest(0).session==123);
  assert(!CentralState::online(false,0,0));
  assert(CentralState::online(true,1000,11000));
  assert(!CentralState::online(true,1000,11001));
  assert(!CentralState::online(true,1000,999));
  Message old=message;old.seq=1;old.sensor=SensorState::Unknown;old.sampleAgeMs=1;
  state.receive(old,3000);assert(state.lastSeq==20);
  old.session=456;state.receive(old,3100);assert(state.sensorSession==456 && state.lastSeq==1);
  old.session=123;old.seq=21;state.receive(old,3200);assert(state.sensorSession==456);
  Metrics m; m.originals=1; m.attempts=2; m.first=3; m.confirmed=4;
  m.timeouts=5; m.unconfirmed=6;
  m.rtt.add(10); m.rtt.add(30); m.completion.add(40); m.touch(900);
  Diagnostics d{7,8,9,10,11,12,13,14,15,16,17};
  Output out;
  writeSummary(out,1000,"TX",0x123,m,d);
  Metrics empty;
  writeSummary(out,1000,"RX",0x456,empty,Diagnostics{});
}

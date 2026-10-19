#include "protocol.hpp"
#include <initializer_list>
#include <cassert>
#include <cstdio>
#include <cstring>
using namespace cantiere;
Message data(uint32_t seq=1,uint64_t session=123) { Message m; m.node=1; m.session=session; m.seq=seq; return m; }
void validation() {
  uint8_t b[WireSize]; Message m=data(), x; encode(m,b);
  assert(b[0]==0x43 && b[8]==1 && b[12]==123 && b[20]==1);
  assert(decode(b,sizeof(b),x) && x.session==123 && x.seq==1);
  assert(!decode(nullptr,WireSize,x)); assert(!decode(b,WireSize-1,x)); assert(!decode(b,WireSize+1,x));
  for(auto pos : {0,1,2,3,4,5,6,7,42,43}) { uint8_t old=b[pos]; b[pos]=255; assert(!decode(b,WireSize,x)); b[pos]=old; }
  for(auto pos : {8,12,20}) { uint8_t old=b[pos]; b[pos]=0; assert(!decode(b,WireSize,x)); b[pos]=old; }
  b[24]=1; assert(!decode(b,WireSize,x));
  m=reply(data(),2,999); encode(m,b); assert(decode(b,WireSize,x));
  for(auto pos : {24,28,36}) {uint8_t old=b[pos]; b[pos]=0; assert(!decode(b,WireSize,x)); b[pos]=old;}
  m=data(UINT32_MAX,UINT64_MAX); encode(m,b); assert(decode(b,WireSize,x) && x.session==UINT64_MAX && x.seq==UINT32_MAX);
  m=data();m.event=Event::Telemetry;m.sensor=SensorState::Present;m.distanceMm=85;m.sampleAgeMs=12;
  encode(m,b);assert(decode(b,WireSize,x) && x.distanceMm==85 && x.sensor==SensorState::Present && x.sampleAgeMs==12);
  b[2]=1;assert(!decode(b,WireSize,x)); // old protocol rejected
  m.distanceMm=6000;assert(!valid(m));m.distanceMm=UINT16_MAX;assert(!valid(m));
  std::puts("PASS T01 validazione e serializzazione");
}
void dedup() {
  Deduplicator d;
  assert(d.observe(1,10,1)==Reception::Fresh);
  assert(d.observe(1,10,1)==Reception::Duplicate);
  assert(d.observe(1,10,3)==Reception::Fresh);
  assert(d.observe(1,10,2)==Reception::Fresh);
  assert(d.observe(1,10,2)==Reception::Duplicate);
  assert(d.observe(1,10,100)==Reception::Fresh);
  assert(d.observe(1,10,1)==Reception::Stale);
  assert(d.observe(1,11,1)==Reception::Fresh);
  assert(d.observe(1,10,100)==Reception::Duplicate); // delayed previous boot
  assert(d.observe(2,10,100)==Reception::Fresh);
  assert(d.observe(1,12,1)==Reception::Fresh);
  assert(d.observe(1,13,1)==Reception::Fresh);
  assert(d.observe(1,10,100)==Reception::Fresh); // documented finite cache
  Deduplicator restarted; assert(restarted.observe(1,10,100)==Reception::Fresh);
  std::puts("PASS T02 duplicati finestra riordino sessioni e limite cache");
}
void acknowledgements() {
  Transaction t; assert(t.start(data(),0,100,2)); assert(!t.start(data(2),0,100,2));
  Message a=reply(data(),2,999);
  Message wrong=a; wrong.refSession++; assert(!t.acknowledge(wrong,2,10,10));
  wrong=a; wrong.refSeq++; assert(!t.acknowledge(wrong,2,10,10));
  wrong=a; wrong.refNode++; assert(!t.acknowledge(wrong,2,10,10));
  wrong=a; wrong.node++; assert(!t.acknowledge(wrong,2,10,10));
  wrong=a; wrong.event=Event::Alarm; assert(!t.acknowledge(wrong,2,10,10));
  wrong=a; wrong.attempt=1; assert(!t.acknowledge(wrong,2,10,10));
  assert(t.acknowledge(a,2,10,10)); assert(!t.acknowledge(a,2,11,11));
  assert(t.metrics.confirmed==1 && t.metrics.first==1);
  assert(t.start(data(2),20,100,2));
  a=reply(data(2),2,1000); assert(t.acknowledge(a,2,30,30)); // RX reboot
  assert(t.start(data(1,124),40,100,2));
  a=reply(data(),2,1000); assert(!t.acknowledge(a,2,50,50));
  a=reply(data(1,124),2,1000); assert(t.acknowledge(a,2,50,50));
  std::puts("PASS T03 correlazione ACK e riavvio TX/RX");
}
void retries() {
  Transaction t; assert(!t.start(data(),0,0,0)); assert(!t.start(data(),0,60001,0)); assert(!t.start(data(),0,100,11));
  assert(t.start(data(),100,50,2));
  assert(t.tick(149)==Tick::None); assert(t.tick(150)==Tick::Retry);
  assert(t.attempts==2 && t.data.attempt==1);
  assert(t.tick(200)==Tick::Retry); assert(t.tick(250)==Tick::Failed); assert(t.tick(999)==Tick::None);
  assert(t.metrics.originals==1 && t.metrics.attempts==3 && t.metrics.timeouts==3 && t.metrics.unconfirmed==1);
  assert(!t.acknowledge(reply(data(),2,5),2,251,251));
  assert(t.start(data(2),300,50,0)); assert(!t.acknowledge(reply(data(2),2,5),2,350,350));
  assert(t.tick(350)==Tick::Failed);
  assert(t.start(data(3),400,50,1)); assert(t.tick(450)==Tick::Retry);
  assert(t.acknowledge(reply(data(3),2,5),2,460,460)); // first ACK late, echoed attempt zero
  assert(t.metrics.first==0 && t.metrics.confirmed==1 && t.metrics.rtt.mean==60);
  std::puts("PASS T04 timeout esatti retry zero/max e ACK tardivi");
}
void metrics() {
  Transaction t; const uint32_t start=UINT32_MAX-20;
  assert(t.start(data(),start,50,1)); assert(t.tick(28)==Tick::None); assert(t.tick(29)==Tick::Retry);
  Message a=reply(t.data,2,8); assert(t.acknowledge(a,2,39,1000));
  assert(t.metrics.rtt.mean==10 && t.metrics.completion.mean==60);
  assert(t.metrics.lastValid==1000 && t.metrics.seen);
  Samples s; assert(s.sd()==0); s.add(10); assert(s.sd()==0); s.add(20); s.add(30);
  assert(s.mean==20 && std::abs(s.sd()-10)<0.0001 && s.minimum==10 && s.maximum==30);
  Clock c; assert(c.update(UINT32_MAX)==UINT32_MAX); assert(c.update(5)==uint64_t(UINT32_MAX)+6);
  assert(c.update(UINT32_MAX)==2*uint64_t(UINT32_MAX)+1); assert(c.update(5)==2*uint64_t(UINT32_MAX)+7);
  std::puts("PASS T05 metriche RTT varianza rollover e silenzio lungo");
}
void simulation() {
  Transaction t; Deduplicator d; auto m=data(); m.event=Event::Alarm;
  assert(t.start(m,0,100,10)); assert(d.observe(m.node,m.session,m.seq)==Reception::Fresh);
  // Lose every reply until the final attempt. Alarm must remain unique on RX.
  for(unsigned i=1;i<=10;++i) {
    assert(t.tick(i*100)==Tick::Retry);
    assert(d.observe(t.data.node,t.data.session,t.data.seq)==Reception::Duplicate);
  }
  auto a=reply(t.data,2,100); assert(t.acknowledge(a,2,1001,1001));
  assert(t.metrics.attempts==11 && t.metrics.confirmed==1 && t.metrics.unconfirmed==0 && t.metrics.first==0);
  std::puts("PASS T06 simulazione perdita ACK allarme e limite massimo");
}
void bidirectionalTest() {
  auto a=data();a.event=Event::RadioTest;
  auto b=a;b.node=2;b.session=999;
  uint8_t bytes[WireSize];Message parsed;
  encode(a,bytes);assert(decode(bytes,WireSize,parsed) && parsed.event==Event::RadioTest);
  Transaction left,right;assert(left.start(a,0,250,3));assert(right.start(b,0,250,3));
  assert(left.acknowledge(reply(a,b.node,b.session),2,10,10));
  assert(right.acknowledge(reply(b,a.node,a.session),a.node,11,11));
  assert(left.metrics.confirmed==1 && right.metrics.confirmed==1);
  a.event=Event(7);assert(!valid(a));
  std::puts("PASS T07 messaggio radio e transazioni bidirezionali simultanee");
}
void textMessages() {
  Message m=data(),parsed;m.event=Event::RadioTest;
  const char* text=u8"Ciao, caffè? \"Ricevuto\" \\ <b>ciao</b> 🙂";
  std::strcpy(m.text,text);uint8_t bytes[WireSize];encode(m,bytes);
  assert(WireSize<=250 && decode(bytes,WireSize,parsed) && !std::strcmp(text,parsed.text));
  assert(!std::strcmp(reply(m,2,22).text,text));
  bytes[2]=2;assert(!decode(bytes,WireSize,parsed));encode(m,bytes);
  bytes[49]=1;assert(!decode(bytes,WireSize,parsed));encode(m,bytes);
  bytes[48]=193;assert(!decode(bytes,WireSize,parsed));
  assert(!validText("\xc0\xaf",2));assert(!validText("\xed\xa0\x80",3));
  assert(!validText("\xf4\x90\x80\x80",4));assert(!validText("a\0b",3));assert(!validText("a\nb",3));
  memset(m.text,'a',49);m.text[49]=0;assert(!valid(m));m.text[48]=0;assert(valid(m));
  m.event=Event::Probe;assert(!valid(m));
  m.event=Event::RadioTest;for(size_t i=0;i<48;++i)memcpy(m.text+4*i,u8"🙂",4);m.text[192]=0;
  encode(m,bytes);assert(decode(bytes,WireSize,parsed) && !strcmp(m.text,parsed.text));
  std::puts("PASS T08 testo UTF-8, escape, limiti e protocollo precedente");
}
int main() { validation(); dedup(); acknowledgements(); retries(); metrics(); simulation(); bidirectionalTest(); textMessages(); }

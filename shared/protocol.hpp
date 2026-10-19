#pragma once
#include <stdint.h>
#include <stddef.h>
#include <cmath>
#include <cstring>
#include "message_text.hpp"

namespace cantiere {
constexpr size_t WireSize = 52+TextCapacity;
constexpr uint8_t Version = 3, MaxRetries = 10;
enum class Kind : uint8_t { Data = 1, Ack = 2 };
enum class Event : uint8_t { Probe = 1, Alarm = 2, Telemetry = 3, SensorAlarm = 4, SensorClear = 5, RadioTest = 6 };
enum class SensorState : uint8_t { Disabled=0, Unknown=1, Clear=2, Present=3 };
struct Message {
  Kind kind = Kind::Data;
  Event event = Event::Probe;
  uint8_t attempt = 0;
  uint32_t node = 0;
  uint64_t session = 0;
  uint32_t seq = 0, refNode = 0;
  uint64_t refSession = 0;
  uint32_t refSeq = 0;
  uint16_t distanceMm=UINT16_MAX;
  SensorState sensor=SensorState::Disabled;
  uint32_t sampleAgeMs=UINT32_MAX;
  char text[TextCapacity+1]{};
};
inline void put(uint8_t* p, uint64_t v, size_t n) {
  for (size_t i = 0; i < n; ++i) { p[i] = uint8_t(v); v >>= 8; }
}
inline uint64_t get(const uint8_t* p, size_t n) {
  uint64_t v = 0; for (size_t i = 0; i < n; ++i) v |= uint64_t(p[i]) << (8*i); return v;
}
inline bool valid(const Message& m) {
  size_t n=0;while(n<=TextCapacity && m.text[n])++n;
  if(!validText(m.text,n) || (m.event!=Event::RadioTest && n))return false;
  if (!m.node || !m.session || !m.seq || m.attempt > MaxRetries ||
      (uint8_t(m.event)<1 || uint8_t(m.event)>6)) return false;
  if(uint8_t(m.sensor)>3 || (m.distanceMm!=UINT16_MAX && m.distanceMm>5100)) return false;
  if(m.sensor==SensorState::Disabled && (m.distanceMm!=UINT16_MAX || m.sampleAgeMs!=UINT32_MAX)) return false;
  if(m.distanceMm!=UINT16_MAX && m.sampleAgeMs==UINT32_MAX) return false;
  if((m.sensor==SensorState::Clear || m.sensor==SensorState::Present) && (m.distanceMm==UINT16_MAX || m.sampleAgeMs==UINT32_MAX)) return false;
  if (m.kind == Kind::Data) return !m.refNode && !m.refSession && !m.refSeq;
  return m.kind == Kind::Ack && m.refNode && m.refSession && m.refSeq;
}
inline void encode(const Message& m, uint8_t* b) {
  b[0]=0x43; b[1]=0x41; b[2]=Version; b[3]=uint8_t(m.kind);
  b[4]=uint8_t(m.event); b[5]=m.attempt; b[6]=b[7]=0;
  put(b+8,m.node,4); put(b+12,m.session,8); put(b+20,m.seq,4);
  put(b+24,m.refNode,4); put(b+28,m.refSession,8); put(b+36,m.refSeq,4);
  put(b+40,m.distanceMm,2);b[42]=uint8_t(m.sensor);b[43]=0;put(b+44,m.sampleAgeMs,4);
  size_t n=0;while(n<TextCapacity && m.text[n])++n;
  b[48]=uint8_t(n);b[49]=b[50]=b[51]=0;
  for(size_t i=0;i<TextCapacity;++i)b[52+i]=i<n?uint8_t(m.text[i]):0;
}
inline bool decode(const uint8_t* b, size_t n, Message& m) {
  if (!b || n != WireSize || b[0]!=0x43 || b[1]!=0x41 || b[2]!=Version || b[6] || b[7] || b[43]) return false;
  if(b[48]>TextCapacity || b[49] || b[50] || b[51])return false;
  for(size_t i=b[48];i<TextCapacity;++i)if(b[52+i])return false;
  if(!validText(reinterpret_cast<const char*>(b+52),b[48]))return false;
  Message x;memcpy(x.text,b+52,b[48]); x.kind=Kind(b[3]); x.event=Event(b[4]); x.attempt=b[5];
  x.node=get(b+8,4); x.session=get(b+12,8); x.seq=get(b+20,4);
  x.refNode=get(b+24,4); x.refSession=get(b+28,8); x.refSeq=get(b+36,4);
  x.distanceMm=get(b+40,2);x.sensor=SensorState(b[42]);x.sampleAgeMs=get(b+44,4);
  if (!valid(x)) return false;
  m=x; return true;
}
inline Message reply(const Message& d, uint32_t node, uint64_t session) {
  Message a=d; a.kind=Kind::Ack; a.node=node; a.session=session;
  a.refNode=d.node; a.refSession=d.session; a.refSeq=d.seq; return a;
}
inline uint32_t elapsed(uint32_t now, uint32_t then) { return now-then; }
// Called at least once per 2^32 ms; extends millis() for long silence reporting.
struct Clock {
  uint32_t previous=0; uint64_t total=0;
  uint64_t update(uint32_t now) { total+=elapsed(now,previous); previous=now; return total; }
};
struct Samples {
  uint64_t n=0; double mean=0, m2=0; uint32_t minimum=0, maximum=0;
  void add(uint32_t v) {
    if (!n || v<minimum) minimum=v;
    if (!n || v>maximum) maximum=v;
    ++n; double d=v-mean; mean+=d/double(n); m2+=d*(v-mean);
  }
  double sd() const { return n>1 ? std::sqrt(m2/double(n-1)) : 0; }
};
struct Metrics {
  uint64_t originals=0, attempts=0, confirmed=0, first=0, timeouts=0, unconfirmed=0;
  Samples rtt, completion;
  uint64_t lastValid=0; bool seen=false;
  void touch(uint64_t now) { seen=true; lastValid=now; }
};
enum class Tick { None, Retry, Failed };
class Transaction {
 public:
  Message data;
  Metrics metrics;
  bool active=false;
  uint8_t attempts=0, retries=0;
  uint32_t timeout=0, sent[MaxRetries+1]{};
  bool start(Message m, uint32_t now, uint32_t wait, uint8_t retry) {
    if (active || !valid(m) || m.kind!=Kind::Data || m.attempt || !wait || wait>60000 || retry>MaxRetries) return false;
    data=m; timeout=wait; retries=retry; attempts=1; active=true; sent[0]=now;
    ++metrics.originals; ++metrics.attempts; return true;
  }
  Tick tick(uint32_t now) {
    if (!active || elapsed(now,sent[attempts-1])<timeout) return Tick::None;
    ++metrics.timeouts;
    if (attempts>retries) { active=false; ++metrics.unconfirmed; return Tick::Failed; }
    data.attempt=attempts; sent[attempts++]=now; ++metrics.attempts; return Tick::Retry;
  }
  bool acknowledge(const Message& a, uint32_t peer, uint32_t now, uint64_t wideNow) {
    if (!active || !valid(a) || a.kind!=Kind::Ack || a.node!=peer || a.event!=data.event ||
        a.refNode!=data.node || a.refSession!=data.session || a.refSeq!=data.seq ||
        a.attempt>=attempts || elapsed(now,sent[attempts-1])>=timeout) return false;
    active=false; ++metrics.confirmed; if (attempts==1) ++metrics.first;
    metrics.rtt.add(elapsed(now,sent[a.attempt])); metrics.completion.add(elapsed(now,sent[0]));
    metrics.touch(wideNow); return true;
  }
};
enum class Reception { Fresh, Duplicate, Stale };
// Four boot sessions, FIFO replacement, 64 sequence numbers per session. No security guarantee.
class Deduplicator {
  struct Slot { uint32_t node=0, top=0; uint64_t session=0, bits=0; } slots[4];
  size_t next=0;
 public:
  Reception observe(uint32_t node, uint64_t session, uint32_t seq) {
    Slot* s=nullptr;
    for (auto& x:slots) if (x.node==node && x.session==session) { s=&x; break; }
    if (!s) { s=&slots[next]; next=(next+1)%4; *s=Slot{}; s->node=node; s->session=session; }
    if (!s->bits) { s->top=seq; s->bits=1; return Reception::Fresh; }
    if (seq>s->top) {
      uint32_t d=seq-s->top; s->bits=d>=64 ? 1 : (s->bits<<d)|1; s->top=seq; return Reception::Fresh;
    }
    uint32_t d=s->top-seq;
    if (d>=64) return Reception::Stale;
    uint64_t mask=uint64_t(1)<<d;
    if (s->bits&mask) return Reception::Duplicate;
    s->bits|=mask; return Reception::Fresh;
  }
};
} // namespace cantiere

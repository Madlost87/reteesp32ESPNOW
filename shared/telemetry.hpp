#pragma once
#include "protocol.hpp"
namespace cantiere {
struct Diagnostics {
  uint64_t rxOriginals, duplicates, stale, unmatched, invalid;
  uint64_t macOk, macFail, sendErrors, queueDrops, ackQueued, ackDropped;
};
template<class Output>
void writeSummary(Output& out, uint64_t now, const char* role, uint64_t session,
                  const Metrics& s, const Diagnostics& d) {
  out.printf("S,%llu,%s,%016llx,"
    "%llu,%llu,%llu,%llu,%llu,%llu," // TX originals through unconfirmed
    "%llu,%llu,%llu,%llu,%llu," // RX originals through invalid
    "%llu,%llu,%llu,%llu,%llu,%llu," // MAC through ACK dropped
    "%llu,%.3f,%.3f,%lu,%lu,%.3f,%lld\n",
    (unsigned long long)now,role,(unsigned long long)session,
    (unsigned long long)s.originals,(unsigned long long)s.attempts,
    (unsigned long long)s.first,(unsigned long long)s.confirmed,
    (unsigned long long)s.timeouts,(unsigned long long)s.unconfirmed,
    (unsigned long long)d.rxOriginals,(unsigned long long)d.duplicates,
    (unsigned long long)d.stale,(unsigned long long)d.unmatched,(unsigned long long)d.invalid,
    (unsigned long long)d.macOk,(unsigned long long)d.macFail,(unsigned long long)d.sendErrors,
    (unsigned long long)d.queueDrops,(unsigned long long)d.ackQueued,(unsigned long long)d.ackDropped,
    (unsigned long long)s.rtt.n,s.rtt.mean,s.rtt.sd(),(unsigned long)s.rtt.minimum,
    (unsigned long)s.rtt.maximum,s.completion.mean,s.seen?(long long)(now-s.lastValid):-1LL);
}
}

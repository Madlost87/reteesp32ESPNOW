#!/usr/bin/env python3
"""Estrae E/S dai log seriali senza aprire porte o controllare dispositivi."""
import argparse
import csv
import sys

EVENT = 'record,uptime_ms,role,type,node,local_session,sequence,event,attempt,packet_session,ref_session,value'.split(',')
SUMMARY = ('record,uptime_ms,role,local_session,originals,attempts,first,confirmed,timeouts,unconfirmed,'
           'rx_originals,duplicates,stale,unmatched,invalid,mac_ok,mac_fail,send_errors,queue_drops,'
           'ack_queued,ack_dropped,rtt_n,rtt_mean_ms,rtt_sd_ms,rtt_min_ms,rtt_max_ms,completion_mean_ms,last_valid_age_ms').split(',')

def extract(source, events, summaries):
    writers = {'E': (csv.writer(events), EVENT), 'S': (csv.writer(summaries), SUMMARY)}
    for writer, header in writers.values():
        writer.writerow(header)
    rejected = 0
    for line in source:
        line = line.strip()
        if not line.startswith(('E,', 'S,')):
            continue
        row = next(csv.reader([line]))
        writer, header = writers[row[0]]
        if len(row) != len(header):
            rejected += 1
            continue
        writer.writerow(row)
    return rejected

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log')
    parser.add_argument('--prefix', required=True)
    args = parser.parse_args()
    # Exclusive creation avoids overwriting a previous field trial.
    with open(args.log, encoding='utf-8', errors='replace') as source, \
         open(args.prefix + '-events.csv', 'x', newline='') as events, \
         open(args.prefix + '-summary.csv', 'x', newline='') as summaries:
        rejected = extract(source, events, summaries)
    if rejected:
        print(f'Righe CSV troncate/non valide: {rejected}; conservare il log originale', file=sys.stderr)
        return 1
    return 0

if __name__ == '__main__':
    raise SystemExit(main())

import importlib.util
import io
import csv
import subprocess
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('extract_csv', Path(__file__).parents[1] / 'tools/extract_csv.py')
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class CsvTests(unittest.TestCase):
    def test_records_and_truncation(self):
        event = 'E,100,TX,confirmed,1,0000000000000123,1,1,0,0000000000000456,0000000000000123,10'
        summary = 'S,100,TX,0000000000000123,1,1,1,1,0,0,0,0,0,0,0,1,0,0,0,0,0,1,10.000,0.000,10,10,10.000,0'
        events, summaries = io.StringIO(), io.StringIO()
        rejected = module.extract(io.StringIO('# boot\n' + event + '\n' + summary + '\nE,broken\n'), events, summaries)
        self.assertEqual(rejected, 1)
        self.assertIn(event, events.getvalue())
        self.assertIn(summary, summaries.getvalue())
        self.assertEqual(len(module.EVENT), 12)
        self.assertEqual(len(module.SUMMARY), 28)

    def test_boot_noise_and_empty(self):
        events, summaries = io.StringIO(), io.StringIO()
        self.assertEqual(module.extract(io.StringIO('rom boot\n# READY\n'), events, summaries), 0)
        self.assertEqual(len(events.getvalue().splitlines()), 1)


class FirmwareOutputTests(unittest.TestCase):
    def test_real_formatter_and_pending_sensor(self):
        root = Path(__file__).parents[1]
        result = subprocess.run([str(root / 'build/test_features')], check=True, capture_output=True, text=True)
        rows = list(csv.reader(io.StringIO(result.stdout)))
        self.assertEqual(len(rows), 2)
        self.assertEqual(len(rows[0]), 28)
        actual = dict(zip(module.SUMMARY, rows[0]))
        self.assertEqual([int(actual[k]) for k in module.SUMMARY[4:21]], list(range(1,18)))
        self.assertEqual(actual['rtt_n'], '2')
        self.assertEqual(actual['rtt_mean_ms'], '20.000')
        self.assertEqual(actual['rtt_sd_ms'], '14.142')
        self.assertEqual(actual['rtt_min_ms'], '10')
        self.assertEqual(actual['rtt_max_ms'], '30')
        self.assertEqual(actual['completion_mean_ms'], '40.000')
        self.assertEqual(actual['last_valid_age_ms'], '100')
        self.assertEqual(rows[1][-1], '-1')
        events, summaries = io.StringIO(), io.StringIO()
        self.assertEqual(module.extract(io.StringIO(result.stdout), events, summaries), 0)

import io
import unittest
from unittest.mock import patch

from record import record


class CaptureTest(unittest.TestCase):
    def test_split_lines_and_quiet_period(self):
        class Port:
            in_waiting = 128

            def __init__(self):
                self.chunks = iter([b"\x1b[3", b"1mfirst\r\nsec", b"ond\npartial", b"", b"tail"])

            def read(self, size):
                assert size == self.in_waiting
                return next(self.chunks)

        log = io.StringIO()
        ticks = [0, 0, 0, 0, 1, 1, 2, 2, 7, 7, 8, 8, 10]
        with patch("record.time.monotonic", side_effect=ticks):
            record(Port(), log, duration=10)
        self.assertEqual(
            [line.partition(" ")[2] for line in log.getvalue().splitlines()],
            ["first", "second", "partial", "Host received no serial output for four seconds", "tail"],
        )

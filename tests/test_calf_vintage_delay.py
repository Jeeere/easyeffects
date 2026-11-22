"""Offline compatibility tests for Calf Vintage Delay; no audio server required.

Run with: python -m unittest discover -s tests -p 'test_calf_vintage_delay.py'
Requires lv2apply and Calf Vintage Delay to be installed.
"""
import array
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
import wave

URI = "http://calf.sourceforge.net/plugins/VintageDelay"


class VintageDelayImpulseTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if not shutil.which("lv2apply") or not shutil.which("lv2ls"):
            raise unittest.SkipTest("LV2 command-line utilities unavailable")
        plugins = subprocess.run(["lv2ls"], capture_output=True, text=True, check=True).stdout.splitlines()
        if URI not in plugins:
            raise unittest.SkipTest("Calf Vintage Delay unavailable")

    def render(self, rate=48000, **overrides):
        controls = dict(on=1, timing=1, ms=100, subdiv=1, time_l=1, time_r=1,
                        feedback=0.5, amount=1, dry=0, mix_mode=0, medium=1)
        controls.update(overrides)
        # The impulse is after startup, so it is not affected by host initialization.
        start = rate // 50
        samples = array.array("h", [0]) * (rate * 2)
        samples[start * 2] = 16384
        if sys.byteorder != "little":
            samples.byteswap()
        with tempfile.TemporaryDirectory(prefix="ee-vintage-delay-") as tmp:
            inp, out = os.path.join(tmp, "in.wav"), os.path.join(tmp, "out.wav")
            with wave.open(inp, "wb") as f:
                f.setparams((2, 2, rate, 0, "NONE", "not compressed"))
                f.writeframes(samples.tobytes())
            args = ["lv2apply", "-i", inp, "-o", out]
            for key, value in controls.items():
                args.extend(["-c", key, str(value)])
            subprocess.run(args + [URI], check=True, capture_output=True)
            with wave.open(out, "rb") as f:
                result = array.array("h")
                result.frombytes(f.readframes(f.getnframes()))
            if sys.byteorder != "little":
                result.byteswap()
        return start, result[::2], result[1::2]

    def test_plain_delay_and_feedback_at_two_rates(self):
        for rate in (44100, 48000):
            with self.subTest(rate=rate):
                start, left, right = self.render(rate, medium=0)
                delay = rate // 10
                self.assertEqual(left[start], 0)
                self.assertAlmostEqual(left[start + delay], 16384, delta=5)
                self.assertAlmostEqual(left[start + 2 * delay], 8192, delta=5)
                self.assertAlmostEqual(left[start + 3 * delay], 4096, delta=5)
                self.assertEqual(max(map(abs, right)), 0)

    def test_dry_and_wet_controls_are_independent(self):
        start, left, _ = self.render(dry=1, amount=0)
        self.assertEqual(left[start], 16384)
        self.assertEqual(max(map(abs, left[start + 100:])), 0)

    def test_tape_filter_is_in_feedback_path(self):
        start, plain, _ = self.render(medium=0)
        _, tape, _ = self.render(medium=1)
        delay = 4800
        # The filtered first echo differs from the unfiltered one; subsequent
        # echoes are filtered again, not just scaled copies of the first.
        self.assertNotEqual(tape[start + delay], plain[start + delay])
        first = tape[start + delay:start + delay + 100]
        second = tape[start + 2 * delay:start + 2 * delay + 100]
        self.assertGreater(sum(abs(x) for x in second[1:]), 0)
        self.assertNotEqual(second[1], round(first[1] * 0.5))

    def test_zero_feedback_has_only_one_echo(self):
        start, left, _ = self.render(feedback=0, medium=0)
        delay = 4800
        self.assertEqual(left[start + delay], 16384)
        self.assertEqual(max(map(abs, left[start + 2 * delay:])), 0)

    def test_bounded_feedback_decays(self):
        start, left, _ = self.render(feedback=0.95, medium=1)
        delay = 4800
        peaks = [max(map(abs, left[start + i * delay:start + (i + 1) * delay]))
                 for i in range(1, 8)]
        self.assertTrue(all(a > b for a, b in zip(peaks, peaks[1:])), peaks)


if __name__ == "__main__":
    unittest.main()

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class TapeEchoThreadSafetyTest(unittest.TestCase):
    def test_inherited_meters_use_atomic_cross_thread_state(self):
        header = (ROOT / 'src/plugin_base.hpp').read_text()
        self.assertIn('std::atomic<bool> updateLevelMeters', header)
        self.assertIn('std::atomic<float> input_peak_left', header)
        self.assertIn('std::atomic<float> output_peak_left', header)
        self.assertIn('READ getUpdateLevelMeters WRITE setUpdateLevelMeters', header)

    def test_lifecycle_and_controls_are_handed_off_without_ui_access(self):
        cpp = (ROOT / 'src/tape_echo.cpp').read_text()
        header = (ROOT / 'src/tape_echo.hpp').read_text()
        process = cpp.split('void TapeEcho::process(', 1)[1].split('void TapeEcho::process(', 1)[0]
        self.assertIn('instance_gate.test_and_set', process)
        self.assertIn('requested_config.load', process)
        self.assertIn('pending_controls', header)
        self.assertNotIn('BIND_LV2_PORT', cpp)
        self.assertNotIn('lv2_wrapper =', cpp)
        self.assertNotIn('create_instance', process)
        self.assertNotIn('destroy_instance', process)


if __name__ == '__main__':
    unittest.main()

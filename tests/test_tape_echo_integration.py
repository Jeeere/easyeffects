"""Checks the Tape Echo settings contract and registration wiring."""
import pathlib
import unittest
import xml.etree.ElementTree as ET

ROOT = pathlib.Path(__file__).resolve().parents[1]

class TapeEchoIntegrationTest(unittest.TestCase):
    def test_settings_are_bounded(self):
        path = ROOT / 'src/contents/kcfg/easyeffects_db_tape_echo.kcfg'
        self.assertTrue(path.exists(), 'Tape Echo settings are missing')
        entries = {e.get('name'): e for e in ET.parse(path).getroot().findall('.//{*}entry')}
        for name in ('delayTime', 'feedback', 'medium', 'dry', 'wet'):
            self.assertIn(name, entries)
        feedback = entries['feedback']
        self.assertLess(float(feedback.find('{*}max').text), 1.0)
        self.assertEqual(float(entries['delayTime'].find('{*}min').text), 10)
        self.assertEqual(float(entries['delayTime'].find('{*}max').text), 2000)

    def test_registration_and_controls_are_wired(self):
        checks = {
            'src/tags_plugin_name.cpp': 'BaseName::tape_echo',
            'src/effects_base.cpp': 'std::make_unique<TapeEcho>',
            'src/db_manager.cpp': 'new DbTapeEcho',
            'src/presets_manager.cpp': 'std::make_unique<TapeEchoPreset>',
            'src/contents/ui/PageStreamsEffects.qml': 'TapeEcho.qml',
            'src/tape_echo.cpp': 'plugins/VintageDelay',
            'src/tape_echo_preset.cpp': 'UPDATE_PROPERTY("feedback", Feedback)',
        }
        for filename, token in checks.items():
            with self.subTest(filename=filename):
                self.assertIn(token, (ROOT / filename).read_text())

if __name__ == '__main__':
    unittest.main()

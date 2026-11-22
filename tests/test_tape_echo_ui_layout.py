"""Guard the Tape Echo card's normal EeCard sizing and scrolling contract."""
from pathlib import Path
import unittest


class TapeEchoLayoutTest(unittest.TestCase):
    def test_card_keeps_eecard_content_layout(self):
        qml = (Path(__file__).resolve().parents[1] / "src/contents/ui/TapeEcho.qml").read_text()
        self.assertIn("Kirigami.ScrollablePage {", qml)
        self.assertNotIn("contentItem:", qml, "Replacing EeCard's contentItem breaks header and content sizing")
        for label in ("Medium", "Delay Time", "Feedback", "Dry", "Wet"):
            self.assertIn(f'i18n("{label}")', qml)


if __name__ == "__main__":
    unittest.main()

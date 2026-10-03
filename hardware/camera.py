from pathlib import Path
from datetime import datetime, timezone
import time

class CameraManager:
    """CSI camera; files remain on the Pi until an upload service is added."""
    def __init__(self, save_dir="images"):
        from picamera2 import Picamera2
        self.save_dir = Path(save_dir)
        self.save_dir.mkdir(parents=True, exist_ok=True)
        self.camera = Picamera2()
        self.camera.configure(self.camera.create_still_configuration())
        self.camera.start()
        time.sleep(2)

    def capture(self):
        name = datetime.now(timezone.utc).strftime("image_%Y%m%d_%H%M%S_%f.jpg")
        path = self.save_dir / name
        self.camera.capture_file(str(path))
        return str(path.resolve())

    def close(self):
        self.camera.stop()
        self.camera.close()

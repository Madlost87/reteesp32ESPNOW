"""Read only the ESP32 security flags using the pinned esptool driver."""
import sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / '.pio-core/packages/tool-esptoolpy'))
from esptool import ESP32ROM
esp = ESP32ROM(sys.argv[1], baud=115200)
try:
    esp.connect()
    print('secure_boot_enabled:', esp.get_secure_boot_enabled())
    print('flash_encryption_enabled:', esp.get_flash_encryption_enabled())
    print('encrypted_download_disabled:', esp.get_encrypted_download_disabled())
finally:
    esp.hard_reset()
    esp._port.close()

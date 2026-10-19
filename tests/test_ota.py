import importlib.util
from pathlib import Path
import unittest
spec=importlib.util.spec_from_file_location('ota_upload',Path(__file__).resolve().parents[1]/'tools/ota_upload.py')
ota=importlib.util.module_from_spec(spec)
spec.loader.exec_module(ota)

class OtaSafetyTests(unittest.TestCase):
    def test_role_and_size(self):
        data=b'\xe9'+b'\0'*300+b'CANTIERE_OTA_ROLE_TX_V1'
        ota.validate_image(data,'tx')
        for bad in (b'',data[1:],data+b'\0'*0x140000):
            with self.assertRaises(ValueError):ota.validate_image(bad,'tx')
        with self.assertRaises(ValueError):ota.validate_image(data,'rx')
    def test_destination_and_availability(self):
        info={'role':'TX','mac':ota.MACS['tx'],'ota_ready':True,'ota_active':False}
        ota.validate_device(info,'tx')
        for key,value in [('role','RX'),('mac',ota.MACS['rx']),('ota_ready',False),('ota_active',True)]:
            with self.assertRaises(ValueError):ota.validate_device(dict(info,**{key:value}),'tx')

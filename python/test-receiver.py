import time
from rpi_rf import RFDevice
rfdevice = RFDevice(27)
rfdevice.enable_rx()
timestamp = None
while True:
    if rfdevice.rx_code_timestamp != timestamp:
        timestamp = rfdevice.rx_code_timestamp
        id = rfdevice.rx_code >> 28
        var = (rfdevice.rx_code >> 24) & 0x0F
        val = (rfdevice.rx_code >> 8) & 0xFFFF
        crc = (rfdevice.rx_code) & 0xFF
        print(str(id) + " " + str(var) + " " + str(val) + " " + str(crc) +
                     " [pulselength " + str(rfdevice.rx_pulselength) +
                     ", protocol " + str(rfdevice.rx_proto) + "]")
    time.sleep(0.01)

rfdevice.cleanup()

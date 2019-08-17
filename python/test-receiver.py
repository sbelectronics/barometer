import time
from rpi_rf import RFDevice
rfdevice = RFDevice(27)
rfdevice.enable_rx()
timestamp = None
while True:
    if rfdevice.rx_code_timestamp != timestamp:
        timestamp = rfdevice.rx_code_timestamp
        if rfdevice.rx_bitlength == 49:
            id = rfdevice.rx_code >> 44
            seq = (rfdevice.rx_code >> 40) & 0x0F
            temp = (rfdevice.rx_code >> 30) & 0x3FF
            humid = (rfdevice.rx_code >> 20) & 0x3FF
            pres = (rfdevice.rx_code >> 8) & 0xFFF
            crc = rfdevice.rx_code & 0xFF
            print("id: %d, seq %d, temp: %d, humid: %d, pres: %d, crc: %d, pulselength: %d, protocol: %d" % (id, seq, temp, humid, pres, crc, rfdevice.rx_pulselength, rfdevice.rx_proto))
        else:
            id = rfdevice.rx_code >> 28
            var = (rfdevice.rx_code >> 24) & 0x0F
            val = (rfdevice.rx_code >> 8) & 0xFFFF
            crc = (rfdevice.rx_code) & 0xFF
            print(str(id) + " " + str(var) + " " + str(val) + " " + str(crc) +
                        " [pulselength " + str(rfdevice.rx_pulselength) +
                        ", protocol " + str(rfdevice.rx_proto) + ", bitlength ", rfdevice.rx_bitlength,  "]")
    time.sleep(0.01)

rfdevice.cleanup()

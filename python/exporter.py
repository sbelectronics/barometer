""" Export RTL-SDR barometer readings to prometheus
    http://www.smbaker.com/

    Dependencies:
    sudo pip install prometheus_client
"""

import sys
import json
import traceback
from prometheus_client import Gauge, start_http_server


class Exporter(object):
    def __init__(self):
        start_http_server(8000)

        self.gauge_humid = Gauge("SensorHumid", "BME280 Sensor Humidity", ["id"])
        self.gauge_temp = Gauge("SensorTemp", "BME280 Sensor Temperature", ["id"])
        self.gauge_barom = Gauge("SensorBarom", "BME280 Sensor Barometer", ["id"])
        self.gauge_seq = Gauge("SensorSeq", "BME280 Sensor Sequence Number", ["id"])
        self.gauge_time = Gauge("SensorTime", "BME280 Sensor Timestamp", ["id"])

    def process_lines(self):
        while True:
            try:
                self.process_line()
            except SystemExit, e:
                raise e
            except Exception:
                traceback.print_exc("Exception in main")

    def process_line(self):
        line = sys.stdin.readline().strip()
        if not line:
            # readline() will always return a /n terminated string, except
            # on the last line of input. If we see an empty string then
            # we must be at eof.
            print >> sys.stderr, "exporter reached EOF"
            sys.exit(0)

        data = json.loads(line)

        print json.dumps(data)

        if "id" not in data:
            return

        id = data["id"]

        if "temp" in data:
            self.gauge_temp.labels(id=id).set(data["temp"]/10.0)

        if "humid" in data:
            self.gauge_humid.labels(id=id).set(data["humid"]/10.0)

        if "barom" in data:
            self.gauge_barom.labels(id=id).set(data["barom"])

        if "seq" in data:
            self.gauge_seq.labels(id=id).set(data["seq"])

        self.gauge_time.labels(id=id).set_to_current_time()


def main():
    exp = Exporter()
    exp.process_lines()


if __name__ == "__main__":
    main()

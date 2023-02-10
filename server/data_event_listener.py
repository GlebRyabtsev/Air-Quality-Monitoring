import argparse
import csv
import itertools
import json
import time
from pathlib import Path
from queue import Queue
from threading import Thread
from typing import Optional

import requests
from sseclient import SSEClient

from packet import DataPointPacket
from util import append_entries

class MeasurementDataReceiver(Thread):
    def __init__(self, config: dict, dp_queue: Queue):
        super().__init__()
        self._access_token = config["access_token"]
        self._dp_queue = dp_queue
        self._exit = False
        self._session_duration = 3600  # default session duration in seconds
        if "session_duration" in config:
            self._session_duration = config["session_duration"]
        self._url = f'https://api.particle.io/v1/devices/events/{DataPointPacket.event_name}'

    def start(self) -> None:
        super().start()

    def join(self, timeout: Optional[float] = ...) -> None:
        __exit = True
        super().join(timeout)

    def run(self):
        while not self._exit:
            resp = requests.get(self._url,
                                headers={'Authorization': 'Bearer ' + self._access_token},
                                stream=True)
            client = SSEClient(resp)
            session_start_time = time.time()
            for event in client.events():
                if self._exit or (time.time() - session_start_time > self._session_duration):
                    client.close()
                    break
                event_data = json.loads(event.data)  # parsed json of the data field, {"data":"...",
                #                                                                      "published_at":"...",
                #                                                                      "coreid":"..."}
                device_id = event_data["coreid"]

                if event.event == DataPointPacket.event_name:
                    data_encoded = event_data["data"]  # data encoded in ascii85
                    self._dp_queue.put(DataPointPacket(data_encoded=data_encoded,
                                                       device_id=device_id))


class CSVWriter(Thread):
    # receives DPs from the queue and sorts them into .csv files; supports writing in files that
    # are not most recent sorts individual data points rather than whole packets

    def __init__(self, config: dict, dp_queue: Queue):
        super(CSVWriter, self).__init__()
        self._dp_queue = dp_queue
        self._config = config
        self._exit = False

    def start(self) -> None:
        super(CSVWriter, self).start()

    def run(self) -> None:
        while not self._exit:
            dp = dp_queue.get()
            dev = next(
                filter(lambda device: device["id"] == dp.device_id, self._config["devices"]),
                None)
            if dev is None:
                dev_name = dp.device_id
            else:
                dev_name = dev["name"]

            data_points = dp.get_data_points()
            append_entries(f'sensors/{dev_name}', data_points)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.parse_args()  # todo: read config file from args

    f = open('config.json')
    config_data = json.load(f)
    f.close()
    dp_queue = Queue(1000)
    measurement_data_receiver = MeasurementDataReceiver(config_data, dp_queue)
    csv_writer = CSVWriter(config_data, dp_queue)

    measurement_data_receiver.start()
    csv_writer.start()

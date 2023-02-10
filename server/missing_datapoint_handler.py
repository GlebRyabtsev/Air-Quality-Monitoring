import csv
import json
import multiprocessing
import time
from datetime import date
from typing import List, Tuple, Callable
from pathlib import Path
from warnings import warn
import logging

import requests
from sseclient import SSEClient

from packet import HandshakePacket, RequestedDataPointPacket
from util import insert_entries_into_directory, TimestampedCSVEntry, data_point_header_row

class PacketVerificationError(RuntimeError):
    pass


def find_gaps_in_file(filename: Path, max_gap: int) -> List[Tuple[int, int]]:
    """
    Given a .csv file with data points, searches for a gap in timestamps larger than max_gap.

    Assumes the file is sorted.
    :param filename: file to check
    :param max_gap: maximum allowed gap (seconds)
    :return: list of gaps (start and end exclusive!)
    """
    intervals = []
    with open(filename) as f:
        reader = csv.reader(f)
        next(reader)  # skip header
        last_timestamp = int(next(reader)[0])
        first = True
        for row in reader:
            try:
                timestamp = int(row[0])
            except ValueError:
                if not first:
                    warn(f"{filename} contains a corrupted line.", RuntimeWarning)
                continue
            if timestamp - last_timestamp > max_gap:
                intervals.append((last_timestamp, timestamp))
            last_timestamp = timestamp

            if first:
                first = False

    return intervals


def find_gaps_in_dir(directory: Path, max_gap: int, file_filter: Callable) -> List[Tuple[int, int]]:
    """
    Given a directory of .csv files with data points, finds gaps between timestamps of
    individual data points, which are larger than a max_gap
    :param directory: location of the csv files
    :param max_gap: maximum allowed gap (seconds)
    :param file_filter: function to filter the files, must take pathlib.Path and return boolean
    :return: list of gaps (start and end exclusive)
    """
    all_files = directory.glob('**/*.csv')
    filtered_files = [f for f in all_files if file_filter(f)]
    filtered_files.sort(key=lambda f: int(f.stem.replace('-', '')))

    def get_timestamps(f) -> Tuple[int, int]:
        """Returns first and last timestamps of a file"""
        with open(f, 'r') as f:
            rows = list(csv.reader(f))
            try:
                first = int(rows[0][0])
            except ValueError:
                first = int(rows[1][0])
            return first, int(rows[-1][0])

    # find gaps within and between the files
    _, last_timestamp = get_timestamps(filtered_files[0])
    intervals = find_gaps_in_file(filtered_files[0], max_gap)
    for f in filtered_files[1:]:
        # check between files
        first_timestamp, this_last_timestamp = get_timestamps(f)
        if first_timestamp - last_timestamp > max_gap:
            intervals.append((last_timestamp, first_timestamp))
        # check within the current file
        intervals.extend(find_gaps_in_file(f, max_gap))
        last_timestamp = this_last_timestamp

    return intervals


def send_handshake_and_listen(device_id: str, access_token: str, queue: multiprocessing.SimpleQueue,
                              intervals: List[Tuple[int, int]] | None = None):
    """
    Sends a handshake and listens for Requested Data Point Packets from the device.

    :param intervals: List of requested intervals. Can be None.
    :param device_id:
    :param access_token:
    :param queue: multiprocessing.SimpleQueue to put the result in. The result is a single queue
    item, either List[RequestedDataPointPacket] or RuntimeError.
    :return: nothing
    """
    handshake_url = f'https://api.particle.io/v1/devices/{device_id}/handshake'
    hp = HandshakePacket(device_id, intervals)
    data = {'arg': hp.encode()[0]}
    try:
        requests.post(handshake_url,
                      headers={'Authorization': 'Bearer ' + access_token},
                      data=data, timeout=1)
    except requests.exceptions.ReadTimeout:
        pass

    if not intervals:
        # put empty list into the return queue and return
        queue.put([])
        return

    response_url = f'https://api.particle.io/v1/devices/events/' \
                   f'{RequestedDataPointPacket.event_name}'
    resp = requests.get(response_url,
                        headers={'Authorization': 'Bearer ' + access_token},
                        stream=True)
    client = SSEClient(resp)
    client.events()
    packet_numbers_received = set()
    packets: List[RequestedDataPointPacket] = []
    for event in client.events():
        event_data = json.loads(event.data)
        event_device_id = event_data["coreid"]
        if event_device_id == device_id and event.event == RequestedDataPointPacket.event_name:
            packets.append(RequestedDataPointPacket(data_encoded=event_data["data"],
                                                    device_id=device_id))
            if not packets[-1].compare_handshake(hp):
                queue.put(PacketVerificationError)
                return
            total, number = packets[-1].get_number()
            packet_numbers_received.add(number)
            if len(packet_numbers_received) == total:
                queue.put(packets)
                return


def handshake(device_id: str,
              access_token: str,
              intervals: List[Tuple[int, int]] | None = None,
              timeout=600) -> List[TimestampedCSVEntry]:
    """
    Send a handshake to a device and wait for response or timeout.

    Wraps send_handshake_and_listen into a separate process in order to enable timeout. Can raise
    TimeoutError or RuntimeError.
    :param device_id:
    :param access_token:
    :param intervals: list of intervals to request
    :param timeout: timeout for the response packets (seconds)
    :return: list of data points
    """
    if intervals and len(intervals) > HandshakePacket.max_intervals:
        raise ValueError("Cannot pack that many intervals into one handshake packet.")

    # queue to return a value from  the request_and_listen() process
    queue = multiprocessing.SimpleQueue()

    process = multiprocessing.Process(target=send_handshake_and_listen,
                                kwargs={"intervals": intervals,
                                        "device_id": device_id,
                                        "access_token": access_token,
                                        "queue": queue})
    process.start()
    process.join(timeout)
    if process.is_alive():
        process.terminate()
        raise TimeoutError

    # get the return value from the shared queue
    res = queue.get()
    if isinstance(res, RuntimeError):
        raise res
    else:
        # extract data points
        data_points = []
        for packet in res:
            data_points.extend(packet.get_data_points())

        return data_points


def send_handshakes(device_id: str,
                    device_name: str,
                    access_token: str, *,
                    search_timespan_days: int,
                    max_gap: int,
                    n_attempts: int = 1) -> List[TimestampedCSVEntry]:
    """
    Searches for gaps in a given device's data and a handshakes (multiple handshakes if gaps do
    not fit into one handshake).

    Looks for csv files in sensors/<device name>/. Assumes that files are named conventionally
    and that rows in the files are sorted.
    :param device_id:
    :param device_name: Name of the device.
    :param access_token:
    :param search_timespan_days: Specifies the maximum age of a gap in data points for it to be
    requested in the handshake.
    :param max_gap: Specifies maximum allowed gap between adjacent data points.
    :param n_attempts: Total number of attempts to send a handshake if a TimeoutError or
    PacketVerificationError occurs. If all attempts fail, the underlying error is raised.
    :return: Received data points.
    """

    # define filter function for find_gaps_in_dir()
    def is_within_timespan(file: Path) -> bool:
        file_date = date(*map(int, file.stem.split('-')))
        return (date.today() - file_date).days < search_timespan_days

    csv_dir = Path(f'sensors/{device_name}')
    if not csv_dir.exists():
        warn(f"Directory does not exist: {csv_dir}", RuntimeWarning)
    logging.info("Searching for gaps in the data...")
    requested_intervals = find_gaps_in_dir(csv_dir, max_gap, is_within_timespan)
    logging.info(f"Found {len(requested_intervals)} gaps.")
    data_points = []

    # iterate over requested intervals and send out handshakes, while respecting maximum
    # handshake packet capacity
    for i in range(0, len(requested_intervals), HandshakePacket.max_intervals):
        logging.info("Sending handshake...")
        last_error = None
        # attempt to send a handshake multiple times if the device does not respond
        # (TimeoutError) or sends a wrong or corrupt packet (PacketVerificationError)
        for j in range(n_attempts):
            try:
                result = handshake(
                    device_id,
                    access_token,
                    requested_intervals[
                        i:min(i + HandshakePacket.max_intervals, len(requested_intervals))
                    ])
            except (TimeoutError, PacketVerificationError) as e:
                # timed out waiting for response or received wrong/corrupt packet
                logging.warning(f"Error when performing handshake: {e}")
                last_error = e
                pass
            else:
                # response received or not required
                break
        else:
            # error on all attempts, raise the most recent one
            logging.error("Error on all attempts.")
            raise last_error
        # we have received the response, so add data points to the result
        logging.info(f"Received response, number of datapoints: {len(result)}")
        data_points.extend(result)

    return data_points


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    dev_id = "e00fce68cdb95fcd741bbb51"
    at = "8a54582a292c40694f0177036e6e1af6df9d3ba4"
    dev_name = "Test-Argon-1"
    dps = send_handshakes(dev_id, dev_name, at, search_timespan_days=30, max_gap=17)  #
    # todo:
    # test search timespan
    logging.info("Inserting...")
    insert_entries_into_directory(f"./sensors/{dev_name}", dps, header=data_point_header_row)
    logging.info("Done...")

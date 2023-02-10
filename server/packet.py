import base64
import time
from typing import List, Tuple

from util import uint_to_bytes, bytes_to_uint, TimestampedCSVEntry


class OutgoingPacket:
    """Represents a general packet sent from server to a device.

    As opposed to an IncomingPacket, outgoing packets are carried in a Particle Device Function's
    payload, not in a Particle Cloud Event. Therefore, event name is not necessary.
    """
    device_id = None

    def __init__(self):
        self.data = bytearray()

    def encode(self):
        return base64.a85encode(self.data), self.device_id


class HandshakePacket(OutgoingPacket):
    """
    A handshake packet sent from server to a device.
    """
    max_intervals = 99

    def __init__(self, device_id: str, time_intervals: List[Tuple[int, int]] = None):
        """
        :param device_id
        :param time_intervals: A list of time intervals for the device to look for data points
        in, or None.
        """
        super().__init__()
        if time_intervals is None:
            time_intervals = []
        self.device_id = device_id
        self.TIMESTAMP = int(time.time())
        self.data.extend(uint_to_bytes(self.TIMESTAMP, 4))
        for interval in time_intervals:
            self.data.extend(uint_to_bytes(interval[0], 4))
            self.data.extend(uint_to_bytes(interval[1], 4))


class IncomingPacket:
    """Base class for a packet sent from a device to the server.
    """
    event_name = None  # Particle Cloud event name associated with a packet type

    def __init__(self, *, data_encoded: str | None = None, data_decoded: bytes | None = None,
                 device_id: str | None = None):
        if data_encoded:
            self.data = base64.a85decode(data_encoded)
            if data_decoded:
                raise RuntimeError("Wrong parameters.")
        else:
            self.data = data_decoded
        self.device_id = device_id

    def get_timestamp(self):
        return bytes_to_uint(self.data[0:4])


class GeneralDataPointPacket(IncomingPacket):
    """Base class for incoming packets containing data points.

    Allows for different types of packets through _HEADER_LENGTH class constant.
    """
    _MULTIPLIER = 32.0 * 100.0
    _DATAPOINT_SIZE = 34
    _HEADER_LENGTH = None

    def get_data_points(self) -> List[TimestampedCSVEntry]:
        data_point_index = 1
        data_points = []
        while data_point_index * self._DATAPOINT_SIZE + self._HEADER_LENGTH <= len(self.data):
            datapoint_start_byte = self._DATAPOINT_SIZE * (data_point_index - 1) \
                                   + self._HEADER_LENGTH
            timestamp = time.localtime(bytes_to_uint(self.data[
                                                     datapoint_start_byte:datapoint_start_byte + 4]))
            datapoint_values = []
            for i in range(10):
                value_start_byte = datapoint_start_byte + 4 + i * 3
                datapoint_values.append(bytes_to_uint(
                    self.data[value_start_byte:value_start_byte + 3]) / self._MULTIPLIER)
            data_points.append(TimestampedCSVEntry(timestamp, datapoint_values))
            data_point_index += 1
        return data_points


class DataPointPacket(GeneralDataPointPacket):
    """Packet of data points containing the regular measurements."""
    _HEADER_LENGTH = 0
    event_name = "dp"


class RequestedDataPointPacket(GeneralDataPointPacket):
    """Represents a packet of data points, which were previously requested by the server.

    See the corresponding class in the firmware for packet format."""

    event_name = 'rdp'

    def __init__(self, data_encoded: str, device_id: str):
        super().__init__(data_encoded=data_encoded, device_id=device_id)
        self._HEADER_LENGTH = 10

    def compare_handshake(self, hp: HandshakePacket) -> bool:
        """Compare a handshake with the copy of the handshake stored in the RDP packet.

        Only timestamps are compared"""
        return hp.TIMESTAMP == bytes_to_uint(self.data[6:10])

    def get_number(self) -> Tuple[int, int]:
        return bytes_to_uint(self.data[4:5]), bytes_to_uint(self.data[5:6])


class TextPacket(IncomingPacket):
    """A base class for packets containing text data.
    """
    def get_text(self) -> str:
        return str(self.data[4:])


class ErrorPacket(TextPacket):
    """Packet containing an error message.
    """
    event_name = "error"

import csv
import itertools
import time
from pathlib import Path
from typing import List, Tuple, Dict
from dataclasses import dataclass
from os import name as os_name

# Standard header for the csv files
data_point_header_row = ['# Unix time', 'Time', '1-NP0.5', '1-NP1.0', '1-NP2.5', '1-NP4.0', '1-NP10',
              '2-NP0.5', '2-NP1.0', '2-NP2.5', '2-NP4.0', '2-NP10']


@dataclass
class TimestampedCSVEntry:
    """Represents a row in a CSV file which has a timestamp. The first item is the unix timestamp
    and the second one is the human-readable representation.
    """
    timestamp: time.struct_time
    data: List

    def get_row(self) -> List:
        """
        Get a list which can be passed to csvwriter.writerow()
        :return:
        """

        return [int(time.mktime(self.timestamp)), time.asctime(self.timestamp)] + self.data


def get_file_name(entry: TimestampedCSVEntry):
    """Returns standard filename for a datapoint (yyyy-mm-dd.csv)."""
    t = entry.timestamp
    return f"{t.tm_year}-{t.tm_mon}-{t.tm_mday}.csv"


def get_path(path_str):
    """Returns pathlib.Path for a given path.

    The returned path has expanded home directory ('~') and is absolute."""

    dir_path = Path(path_str)
    if os_name == 'posix':
        dir_path = dir_path.expanduser()
    else:
        raise NotImplementedError
    return dir_path.absolute()


def append_entries(directory: str, entries: List[TimestampedCSVEntry]) \
        -> None:
    """
    Sorts entries and writes them to a file. Overwrites the file, if it already exists,
    without checking for duplication or correct order.

    :param directory: directory of the output files
    :param entries: list of csv entries
    """
    last_filename = None
    file = None
    csv_writer = None
    dir_path = get_path(directory)
    dir_path.mkdir(parents=True, exist_ok=True)
    for e in entries:
        filename = get_file_name(e)
        if filename != last_filename:
            if last_filename is not None:
                file.close()
            file_path = dir_path.joinpath(filename)
            new = not file_path.exists()
            file = file_path.open('a')
            csv_writer = csv.writer(file)
            if new:
                csv_writer.writerow(data_point_header_row)
            last_filename = filename
        csv_writer.writerow(e.get_row())
    file.close()


def insert_entries_into_directory(directory: str, entries: List[TimestampedCSVEntry], header=None,
                                  replace=False, replace_header=False) -> int:
    """
    Inserts csv entries (e.g. data points) into corresponding csv files in a directory, creating new
    files if necessary and either skipping or replacing entries with matching timestamps.

    :param directory: directory of the output files
    :param entries: list of entries
    :param header: header used to create new files
    :param replace: specifies, whether to overwrite entries with matching timestamps
    :param replace_header: specifies, whether to replace existing headers with header argument
    :return: number of entries written (excluding those skipped when overwriting is disabled)
    """
    dir_path = get_path(directory)
    dir_path.mkdir(parents=True, exist_ok=True)

    entries.sort(key=lambda entry: entry.timestamp)

    @dataclass
    class FileEntryList:
        filename: str
        entries: List[TimestampedCSVEntry]

    file_entry_lists: List[FileEntryList] = [FileEntryList(get_file_name(entries[0]), [])]
    prev_timestamp = entries[0].timestamp
    for entry in entries:
        if entry.timestamp == prev_timestamp:
            file_entry_lists[-1].entries.append(entry)
        else:
            file_entry_lists.append(FileEntryList(get_file_name(entry), [entry]))

    new_entries_counter = 0
    for fel in file_entry_lists:
        new_entries_counter += insert_entries_into_file(dir_path.joinpath(fel.filename),
                                                        fel.entries, header=header,
                                                        replace=replace,
                                                        replace_header=replace_header)

    return new_entries_counter


def insert_entries_into_file(file_path: Path, entries: List[TimestampedCSVEntry], header=None,
                             replace=False, replace_header=False) -> int:
    """Insert csv entries into a given file.

    Replace or skip entries with matching timestamps in the process.

    Undefined behaviour if csv file already contains duplicate timestamps.

    :param file_path: file
    :param entries: list of entries
    :param header: header to use
    :param replace: specifies, whether to replace entries with matching timestamps
    :param replace_header: specifies, whether to replace existing headers with the one provided
    as an argument
    :return: number of rows written (including replaced ones, and excluding headers)
    """
    used_header = header
    if file_path.exists():
        with open(file_path, 'r') as file:
            row_buffer = list(csv.reader(file))
            if row_buffer:
                try:
                    int(row_buffer[0][0])
                except ValueError:
                    if replace_header:
                        used_header = row_buffer[0]
                    del row_buffer[0]
    else:
        row_buffer = []

    for r in row_buffer:  # make timestamps integer
        r[0] = int(r[0])

    initial_length = len(row_buffer)

    row_buffer.extend(map(lambda entry: entry.get_row(), entries))
    row_buffer.sort(key=lambda r: r[0])  # python sort is guaranteed to be stable

    rows_to_write = [row_buffer[0]]
    for r in row_buffer[1:]:
        if r[0] != rows_to_write[-1][0]:
            rows_to_write.append(r)
        elif replace:
            rows_to_write[-1] = r

    with open(file_path, 'w') as file:
        writer = csv.writer(file)
        if used_header:
            writer.writerows([used_header])
        writer.writerows(rows_to_write)
    return len(entries) if replace else len(rows_to_write) - initial_length


def uint_to_bytes(x: int, n_bytes: int) -> bytes:
    """
    Convert an unsigned int to a given number of bytes in little-endian order.
    :param x: number to convert
    :param n_bytes: number of bytes
    :return: bytes
    """
    return int.to_bytes(x, n_bytes, "little", signed=False)


def bytes_to_uint(b) -> int:
    """
    Convert bytes in little-endian order to an unsigned int.
    :param b: bytes
    :return: number
    """
    return int.from_bytes(b, "little", signed=False)
import argparse
from pathlib import Path

from packet import DataPointPacket, ErrorPacket
from util import insert_entries_into_directory, insert_entries_into_file, get_path, \
    data_point_header_row, TimestampedCSVEntry
from time import localtime

# calling syntax: packet_binary_decoder.py file [end of range] output_dir

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=
    "This small utility can be used to convert from binary packet files to CSV files.\n\n"
    
    "The program has two modes of operation:\n"
    "- Single file mode: packet_binary_decoder.py <packet file> <output dir> [options]\n"
    "- Range mode: packet_binary_decoder.py <first packet> <last packet> <output dir> [options]\n\n"
    
    "When operating in range mode, the utility attempts to convert all packets files in the "
    "directory, whose timestamps are between the first and the last packet (inclusive). It assumes" 
    "that the packets are named using the standard convention: <integer unix "
    "timestamp>.<type>.pkt\n\n"
    
    "If a csv file(s) with a matching name is already present in the output directory, the utility "
    "will insert the new entries into it. In case entries with identical timestamps "
    "already exists, they will be kept intact. This behavior can be changed with the \"-replace\" "
    "option.", formatter_class=argparse.RawTextHelpFormatter)

    parser.add_argument("binary_packet", type=str, help="Filename of the binary packet, "
                                                        "or the start of the interval for "
                                                        "multiple packets")
    parser.add_argument("end_binary_packet", type=str, help="(Optional) End of interval of "
                                                            "multiple packets. Assumes following "
                                                            "naming scheme: <unix "
                                                            "timestamp>.*.pkt. Assumes packets "
                                                            "are in the same directory.", nargs="?")
    parser.add_argument("destination", type=str, help="Directory in which the csv files are "
                                                      "written.")
    parser.add_argument("-replace", action='store_true', help="Replace data points with matching "
                                                              "timestamps.")

    args = parser.parse_args()

    if args.end_binary_packet is None:
        # Single file mode
        packet_files = [args.binary_packet]
    else:
        # Range mode
        first_packet = get_path(args.binary_packet)
        last_packet = get_path(args.end_binary_packet)
        assert (first_packet.is_file() and last_packet.is_file())
        if first_packet.parent != last_packet.parent:
            print("Error: files must be in the same directory.")
            exit(1)
        # get integer timestamps from filenames
        first_timestamp = int(first_packet.name.split('.')[0])
        last_timestamp = int(last_packet.name.split('.')[0])

        g = first_packet.parent.glob("**/*.*.pkt")
        # filter those files whose timestamps lie between the first and last file
        packet_files = [p for p in g if p.is_file()
                        and first_timestamp <= int(p.name.split('.')[0]) <= last_timestamp]

    data_packets = []  # stores data point packet objects created from the binary files
    text_packets = []  # stores text packet objects created from the binary files

    for filename in packet_files:
        with open(filename, "rb") as file:
            data = file.read()
            event_name = filename.suffixes[0][1:]
            if event_name == DataPointPacket.event_name:
                data_packets.append(DataPointPacket(data_decoded=data))
            elif event_name == ErrorPacket.event_name:
                text_packets.append(ErrorPacket(data_decoded=data))
            else:
                print(f"Warning: unknown event_name {event_name}")

    data_entries = []
    for p in data_packets:
        # insert the data points into the csv files in the target directory
        data_entries.extend(p.get_data_points())
    data_points_written = insert_entries_into_directory(args.destination, data_entries,
                                                        header=data_point_header_row,
                                                        replace=args.replace)

    # Save text packets to a log file
    text_entries = []
    for p in text_packets:
        text_entries.append(TimestampedCSVEntry(localtime(p.get_timestamp()), [p.get_text()]))

    LOG_FILENAME = "text_packets.txt"
    log_file = get_path(args.destination).joinpath(LOG_FILENAME)
    text_entries_written = insert_entries_into_file(log_file, text_entries, header=None,
                                                    replace=args.replace)

    print(f"Written {data_points_written} new data points and {text_entries_written} new text "
          f"entries.")

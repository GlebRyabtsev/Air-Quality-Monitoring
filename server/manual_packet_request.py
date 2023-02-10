import argparse
import json

import requests

from packet import HandshakePacket

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Request data points from a sensor in a certain "
                                                 "time interval.")
    parser.add_argument("device_id", type=str, help='ID of the device from '
                                                    'which to request data.')
    credentials_arg_group = parser.add_mutually_exclusive_group(required=True)
    credentials_arg_group.add_argument("--config", type=str, help='Location of the configuration '
                                                                  'file')
    credentials_arg_group.add_argument("--token", type=str, help='Access token for Particle API')

    parser.add_argument("start_time", type=int, help="Beginning of the requested time interval as "
                                                     "UNIX timestamp")
    parser.add_argument("end_time", type=int, help="End of the requested time interval as UNIX "
                                                   "timestamp")

    args = parser.parse_args()
    url = f'https://api.particle.io/v1/devices/{args.device_id}/handshake'
    if args.config is not None:
        with open(args.config) as f:
            config_data = json.load(f)
        access_token = config_data['access_token']
    else:
        access_token = args.access_token

    hp = HandshakePacket(args.device_id, [(int(args.start_time), int(args.end_time))])
    data = {'arg': hp.encode()[0]}
    requests.post(url, headers={'Authorization': 'Bearer ' + access_token},
                  data=data)

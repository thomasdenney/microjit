#!/usr/bin/env python3

# This script has only been tested on macOS 10.13 (High Sierra)
# You will additionally need pySerial https://pythonhosted.org/pyserial/

import argparse
import codecs
import os
import re
import serial
import struct
from time import sleep

parser = argparse.ArgumentParser(description="Send Stack programs via serial")
parser.add_argument("-d", "--device", default="",
                    help="Name of serial port (will guess otherwise)")
parser.add_argument("-a", "--ascii", action="store_true",
                    help="Input file is ASCII hexadecimal rather than binary")
parser.add_argument("-s", "--separator", default="",
                    help="Separator for ASCII encoded files (default is whitespace)")
parser.add_argument("-v", "--verbose", action="store_true")
parser.add_argument("-t", "--time", type=float, default=0.1,
                    help="Time to wait transmission")
parser.add_argument("input", help="Name of the input file")

args = parser.parse_args()

if args.device == "":
    devices = os.listdir("/dev/")
    pattern = re.compile(r"^cu\.usbmodem")
    matches = [x for x in devices if pattern.match(x)]
    if len(matches) > 0:
        args.device = os.path.join("/dev/", matches[0])
    else:
        print("No device found, please specify a device")
        exit()


def byte_string(bs):
    return ":".join("{:02x}".format(x) for x in bs)


def send(ser, data, name):
    n = ser.write(data)
    ser.flush()
    if args.verbose:
        print("[SEND {}]\t{} ({}/{} bytes)".format(name,
                                                   byte_string(data), n, len(data)))


def send_length(ser, length):
    # < is little endian
    # H is unsigned short
    to_send = struct.pack("<H", length)
    send(ser, to_send, "LENGTH")


if args.ascii:
    with open(args.input, "r") as f:
        contents = f.read().strip()
        if args.separator != "":
            contents = contents.replace(args.separator, '')
        else:
            pattern = re.compile(r"\s+")
            contents = re.sub(pattern, '', contents)
        bs = bytearray(codecs.decode(contents, 'hex'))
else:
    with open(args.input, "rb") as f:
        bs = f.read()

with serial.Serial(args.device, baudrate=115200) as ser:
    if args.verbose:
        print("[CONNECTED]\t{}".format(args.device))
    send_length(ser, len(bs))
    send(ser, bs, "PROGRAM")
    # I've had issues with micro:bits rebooting without compiling the program if
    # these two lines are not present
    ser.flush()
    sleep(args.time)

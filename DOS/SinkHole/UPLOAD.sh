#!/bin/bash
COM1="/dev/ttyUSB0"
COM2="/dev/ttyUSB1"

make sink.upload MOTES=$COM1
make node.upload MOTES=$COM2 && make login MOTES=$COM2

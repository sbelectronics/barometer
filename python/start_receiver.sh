#! /bin/bash
cd /home/pi
nohup ./run_receiver.sh > /tmp/receiver.out 2> /tmp/receiver.err &

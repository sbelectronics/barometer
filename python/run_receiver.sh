#! /bin/bash
/usr/local/bin/rtl_433 -R 135 -f 315000000 -F json | python -u ./exporter.py

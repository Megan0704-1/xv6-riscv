#!/bin/bash
sudo fio --filename=/dev/mapper/cache --name=test --rw=read --direct=1 --size=20MB --numjobs=1
sudo dmsetup status cache
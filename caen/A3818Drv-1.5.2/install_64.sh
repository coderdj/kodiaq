#!/bin/bash

cp -R . /usr/src/a3818-1.5.1
dkms add -m a3818 -v 1.5.1
dkms build -m a3818 -v 1.5.1
dkms install -m a3818 -v 1.5.1
insmod /lib/modules/`uname -r`/updates/dkms/a3818.ko

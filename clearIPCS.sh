#!/bin/bash

killall tapper observe reconstruct tapplot
for id in $(ipcs -m | awk '/0x/ {print $2}'); do ipcrm -m $id > /dev/null; done

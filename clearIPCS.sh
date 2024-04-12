#!/bin/bash

for ((i=0; i<100; i++)); do
	ipcrm -m $i &> /dev/null
done

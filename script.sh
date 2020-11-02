#!/bin/bash

rm -r build 
mkdir build && cd build
cmake -DBOARD=galileo ..
make 
cd ..
cp build/zephyr/zephyr.strip /media/khyati/0D3D-C450/kernel
umount /media/khyati/0D3D-C450



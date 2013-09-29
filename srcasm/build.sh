#!/bin/sh
cc asm6.c -o asm6
g++ text2data.cpp -o text2data
./text2data music.txt -asm6
./asm6 header.asm famitraxx.nes
nes famitraxx.nes

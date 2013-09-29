cc65 -Oi src/famitraxx.c
ca65 src/crt0.s
ca65 src/famitraxx.s
ca65 src/neslib.s
ld65 -o famitraxx.nes -C src/nes.cfg src/crt0.o src/neslib.o src/famitraxx.o nes.lib 
nes famitraxx.nes

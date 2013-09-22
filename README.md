famitraxx
=========

Nintendo source for famitracker track rom player. Used for http://lowtoy.com/argenchip/ next album.

#Building NES ROMs with cc65

```
$ git clone https://github.com/oliverschmidt/cc65
$ make
```

Make sure that after compiling cc65, binaries folder are available to user.

#compiling helloworld
```
../cc65/bin/cl65 -t nes src/hello.c -o hello.nes
```

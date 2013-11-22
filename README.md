pipeduino
=========

simplex/half-duplex serial pipe through an arduino

to set up receiving side serial device:
```
$> stty -F /dev/DEVICE 115200 cs8 cread clocal
$> cat /dev/DEVICE > /path/to/destfile
```

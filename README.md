=ti59 and TMC0501 microcode simulator=

This project allow to run microcode for TMC0501 generation calculator.
See http://www.datamath.org/Chips/TMS0500-CS.htm for more information

This is based of previous work :
http://hsl.wz.cz/ti\_59.htm
https://www.hrastprogrammer.com/emulators.htm

The goal is to able to run microcode of several calculator and understand
how the differents chips work together.

For that each chip is implemented in a separate file and communicate with
bus like shared data.


== How to use it ==

download firmware
```
./bin/get_rom.sh
```

run the selected model

```
./bin/SR50.sh
```

```
./bin/ti59.sh
```


you can pass option "-p" to enable printer

Key mapping is print on startup

You can check doc dir for more technical information.




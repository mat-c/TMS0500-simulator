#! /bin/sh
PRINT="-r rom/rom-print/TMC0569.txt"
./main -r rom/rom-SR56/TMC0537A7645.txt -s rom/rom-SR56/TMC0537A7645-CONST.txt -r rom/rom-SR56/TMC0538A7644.txt -s rom/rom-SR56/TMC0538A7644-CONST-K.txt -k sr56 -R $PRINT $@

#! /bin/sh
PRINT="-r rom/rom-print/TMC0561.txt"
./main -r rom/rom-SR52/TMC0524B.txt -s rom/rom-SR52/TMC0524B-CONST.txt -r rom/rom-SR52/TMC0562C.txt -r rom/rom-SR52/TMC0563B.txt -k sr52 -R -R $PRINT $@

#! /bin/sh
PRINT="-r rom/rom-print/TMC0561.txt"
./main -r rom/rom-SR51A/TMC0532A.txt -s rom/rom-SR51A/TMC0532A-CONST.txt -r rom/rom-SR51A/TMC0533A.txt -s rom/rom-SR51A/TMC0533A-CONST.txt -k sr51 $PRINT $@

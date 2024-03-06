#! /bin/sh
PRINT="-r rom/rom-print/TMC0561.txt -p"
./main -r rom/rom-SR51/TMC0522C7503.txt -s rom/rom-SR51/TMC0522C7503-CONST.txt -r rom/rom-SR51/TMC0523A7509.txt -s rom/rom-SR51/TMC0523A7509-CONST.txt -k sr51 $PRINT $@

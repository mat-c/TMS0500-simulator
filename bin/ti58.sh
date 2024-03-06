#! /bin/sh
#rom-ti59/TMC0571B.txt  rom-ti59/TMC0582-CONST-K.txt  rom-ti59/TMC0582.txt  rom-ti59/TMC0583-CONST-K.txt  rom-ti59/TMC0583.txt
./main -r rom-ti59/TMC0582.txt -s rom-ti59/TMC0582-CONST-K.txt -r rom-ti59/TMC0583.txt -s rom-ti59/TMC0583-CONST-K.txt -r rom-ti59/TMC0571B.txt -k ti58 -R -R $@

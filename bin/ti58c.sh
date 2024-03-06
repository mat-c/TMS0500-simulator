#! /bin/sh
#rom-ti59/TMC0571B.txt  rom-ti59/TMC0582-CONST-K.txt  rom-ti59/TMC0582.txt  rom-ti59/TMC0583-CONST-K.txt  rom-ti59/TMC0583.txt
#rom-ti58c/CD2400-CONST-K.txt  rom-ti58c/CD2400.txt  rom-ti58c/CD2401-CONST-K.txt  rom-ti58c/CD2401.tx
./main -r rom-ti58c/CD2400.txt -s rom-ti58c/CD2400-CONST-K.txt -r rom-ti58c/CD2401.txt -s rom-ti58c/CD2401-CONST-K.txt -r rom-ti58c/TMC0573.txt -k ti58 -m $@

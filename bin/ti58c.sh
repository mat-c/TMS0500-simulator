#! /bin/sh
#rom/rom-ti59/TMC0571B.txt  rom/rom-ti59/TMC0582-CONST-K.txt  rom/rom-ti59/TMC0582.txt  rom/rom-ti59/TMC0583-CONST-K.txt  rom/rom-ti59/TMC0583.txt
#rom/rom-ti58c/CD2400-CONST-K.txt  rom/rom-ti58c/CD2400.txt  rom/rom-ti58c/CD2401-CONST-K.txt  rom/rom-ti58c/CD2401.tx
./main -r rom/rom-ti58c/CD2400.txt -s rom/rom-ti58c/CD2400-CONST-K.txt -r rom/rom-ti58c/CD2401.txt -s rom/rom-ti58c/CD2401-CONST-K.txt -r rom/rom-ti58c/TMC0573.txt -k ti58c -m $@

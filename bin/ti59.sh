#! /bin/sh
#rom/rom-ti59/TMC0571B.txt  rom/rom-ti59/TMC0582-CONST-K.txt  rom/rom-ti59/TMC0582.txt  rom/rom-ti59/TMC0583-CONST-K.txt  rom/rom-ti59/TMC0583.txt
./main -r rom/rom-ti59/TMC0582.txt -s rom/rom-ti59/TMC0582-CONST-K.txt -r rom/rom-ti59/TMC0583.txt -s rom/rom-ti59/TMC0583-CONST-K.txt -r rom/rom-ti59/TMC0571B.txt -k ti59 -R -R -R -R $@

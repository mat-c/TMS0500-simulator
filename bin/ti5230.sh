#! /bin/sh
#TMC0572A.txt  TMC0587D-CONST.txt  TMC0587D.txt  TMC0588E-CONST.txt  TMC0588E.txt
./main -r rom/rom-ti5230/TMC0587D.txt -s rom/rom-ti5230/TMC0587D-CONST.txt -r rom/rom-ti5230/TMC0588E.txt -s rom/rom-ti5230/TMC0588E-CONST.txt -r rom/rom-ti5230/TMC0572A.txt -k ti5230 -R -R -R -R -R -R -R $@

#! /bin/sh
#TMC0526A.txt:BANK: 0
#TMC0564D.txt:BANK: 1
#TMC0565C.txt:BANK: 2
#TMC0566A.txt:BANK: 3
#TMC0567B.txt:BANK: 5
#TMC0570A.txt:BANK: 6

#OPT1="-R -R -R -R -R -R -R -R"
#OPT2="$OPT1 $OPT1 $OPT1 -R -R -R"
./main -r rom/rom-SR60/TMC0526A.txt -s rom/rom-SR52/TMC0524B-CONST.txt -r rom/rom-SR60/TMC0564D.txt -r rom/rom-SR60/TMC0565C.txt -r rom/rom-SR60/TMC0566A.txt -r rom/rom-SR60/TMC0567B.txt -r rom/rom-SR60/TMC0570A.txt -k sr60 -P -R -R -R -R -R $OPT1 $OPT2 $@

#! /bin/bash
set -x
set -e

rm -f log.txt
PROG="./bin/SR50.sh"


echo 111+56=167
#printf " 111+56\n\x18" | $PROG
echo "test mem"
printf " .0123+456-789*\nxSisctashsdlELX&&<<p>RdRdY9r9\n/p\n<<<<.8888888888e88n.n\x7F!/0\n 69!3>78-1\n<12345678>+=s" | $PROG

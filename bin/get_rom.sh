#! /bin/sh


URL=http://www.datamath.org/Chips/ROM/

set -e

mkdir rom

cd rom

mkdir rom-SR50
cd rom-SR50
wget $URL/TMC0521B-CONST.txt
wget $URL/TMC0521B.txt
cd -

mkdir rom-SR50r1
cd rom-SR50r1
wget $URL/TMC0521E-CONST.txt
wget $URL/TMC0521E.txt
cd -

mkdir rom-SR50A/
cd rom-SR50A/
wget $URL/TMC0531A-CONST.txt
wget $URL/TMC0531A.txt
cd -

mkdir rom-print
cd rom-print
wget $URL/TMC0561.txt
wget $URL/TMC0569.txt
cd -

mkdir rom-SR51
cd rom-SR51
wget $URL/TMC0522C7503-CONST.txt
wget $URL/TMC0522C7503.txt
wget $URL/TMC0523A7509-CONST.txt
wget $URL/TMC0523A7509.txt
cd -

mkdir rom-SR51A
cd rom-SR51A
wget $URL/TMC0532A-CONST.txt
wget $URL/TMC0532A.txt
wget $URL/TMC0533A-CONST.txt
wget $URL/TMC0533A.txt
cd -

mkdir rom-SR52
cd rom-SR52
wget $URL/TMC0524B-CONST.txt
wget $URL/TMC0524B.txt
wget $URL/TMC0562C.txt
wget $URL/TMC0563B.txt

#A variant
wget $URL/TMC0534B-CONST.txt
wget $URL/TMC0534B.txt
cd -

mkdir rom-SR56
cd rom-SR56
wget $URL/TMC0537A7645-CONST.txt
wget $URL/TMC0537A7645.txt
wget $URL/TMC0538A7644-CONST-K.txt
wget $URL/TMC0538A7644.txt
cd -

mkdir rom-ti59
cd rom-ti59
wget $URL/TMC0571B.txt
wget $URL/TMC0582-CONST-K.txt
wget $URL/TMC0582.txt
wget $URL/TMC0583-CONST-K.txt
wget $URL/TMC0583.txt
cd -

mkdir rom-ti58c
cd rom-ti58c
wget $URL/CD2400-CONST-K.txt
wget $URL/CD2400.txt
wget $URL/CD2401-CONST-K.txt
wget $URL/CD2401.txt
wget $URL/TMC0573.txt
cd -

mkdir rom-SR51-II
cd rom-SR51-II
wget $URL/TMC0581-CONST.txt
wget $URL/TMC0581.txt
cd -

mkdir module-lib
cd module-lib
wget $URL/TMC0541.txt
cd -

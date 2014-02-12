#!/bin/bash

saptamani=$1
# Se va modifica tipul de schedule aici dupa nevoie
export OMP_SCHEDULE=$2
export OMP_NUM_THREADS=1

echo "Pentru" $saptamani "si" $OMP_SCHEDULE >> analiza.txt

echo "Nrthread"=$OMP_NUM_THREADS >> analiza.txt
{ time ./paralel_optimizat $saptamani ./teste_tema1/in100_20 out; } 2>> analiza.txt
diff out ./teste_tema1/out100_20_100

export OMP_NUM_THREADS=2
echo "----------------" >> analiza.txt
echo "Nrthread"=$OMP_NUM_THREADS >> analiza.txt
{ time ./paralel_optimizat $saptamani ./teste_tema1/in100_20 out; } 2>> analiza.txt
diff out ./teste_tema1/out100_20_100

export OMP_NUM_THREADS=4
echo "----------------" >> analiza.txt
echo "Nrthread"=$OMP_NUM_THREADS >> analiza.txt
{ time ./paralel_optimizat $saptamani ./teste_tema1/in100_20 out; } 2>> analiza.txt
diff out ./teste_tema1/out100_20_100
echo "----------------" >> analiza.txt
echo ""

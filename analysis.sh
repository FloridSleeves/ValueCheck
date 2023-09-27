#!/bin/bash
#########################################################################
# File Name: linux.sh
# Author: lily
# mail: floridsleeves@gmail.com
# Created Time: 2023-06-10 23:28:34
#########################################################################

rm -f tmp/$1-liveness
touch tmp/$1-liveness
source SVF/setup.sh
for f in `ls $1-bitcode/$1_*.bc`
do
   date >> tmp/$1-liveness
   echo $f >> tmp/$1-liveness
   opt -load  ./valuecheck/build/libTransientAnalysisPass.so  -enable-new-pm=0 -t all -TransientLiveness $f > /dev/null 2>> tmp/$1-liveness
   opt -load  ./valuecheck/build/libLLVMLocalLivenessAnalysisPass.so  -enable-new-pm=0 -t all -LocalLiveness $f > /dev/null 2>> tmp/$1-liveness
   opt -load  ./valuecheck/build/libFieldAnalysisPass.so -enable-new-pm=0 -t all -LocalLiveness $f > /dev/null 2>> tmp/$1-liveness
done

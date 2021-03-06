#!/bin/bash

# Build documentation source stage using the kernel and services directories
root_dir=`pwd`
cd ./docs
doxygen ./Doxyfile
cd ${root_dir}
cd ./docs/output/latex/
pdflatex refman
makeindex refman
pdflatex refman
makeindex refman
pdflatex refman
cd ${root_dir}
cp ./docs/output/latex/refman.pdf ./docs

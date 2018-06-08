#!/bin/bash

#echo off

# First parameter is the filename
WORKING_DIR=.
INPUT_FILE_NAME=$1
OUTPUT_DIR="../_build/test_"$INPUT_FILE_NAME
OUTPUT_FILE_NAME="a.out"


#echo $OUTPUT_DIR


# Remove the old OUTPUT_DIR if it exists
if [ -d "$OUTPUT_DIR" ]; then rm -Rf $OUTPUT_DIR; fi

mkdir $OUTPUT_DIR

# Copy the source file to the OutputDir
cp $WORKING_DIR/$INPUT_FILE_NAME $OUTPUT_DIR/$INPUT_FILE_NAME

# Change to the output directory
cd $OUTPUT_DIR

# Compile with flags for gcov
g++-4.6 -std=c++0x -pthread -o $OUTPUT_FILE_NAME $INPUT_FILE_NAME
#CC=g++-4.6
#CFLAGS=`pth-config --cflags`
#LDFLAGS=`pth-config --ldflags`
#LIBS=`pth-config --libs`

#$CC -std=c++0x $CFLAGS -c $INPUT_FILE_NAME

#$CC -std=c++0x $LDFLAGS -o $OUTPUT_FILE_NAME test.o $LIBS


#gcc -fprofile-arcs -ftest-coverage -o $OUTPUT_FILE_NAME $INPUT_FILE_NAME

# Run the compiled File (we are now in the build directory)
./$OUTPUT_FILE_NAME






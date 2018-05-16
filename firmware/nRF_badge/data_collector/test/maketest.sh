#!/bin/bash

#echo off

# First parameter is the filename
WORKING_DIR=.
INPUT_FILE_NAME=$1
OUTPUT_DIR="../_build/test_"$INPUT_FILE_NAME
OUTPUT_FILE_NAME="a.out"
OUTPUT_HTML_DIR="HTML"

echo $OUTPUT_DIR
#INPUT_FILE_NAME_PREFIX=""


# SPLIT the input by the "." --> Watch out, file is not allowed to have spaces " "!!
#IFS='.';
#arrIN=($INPUT_FILE_NAME)
#unset IFS;

#N=$(echo ${arrIN[@]} | wc -w)
#echo $N
#N=`expr $N - 2`
#i=0
#while [ "$i" -lt $N ]
#do
#	INPUT_FILE_NAME_PREFIX="$INPUT_FILE_NAME_PREFIX${arrIN[i]}"
#	echo ${arrIN[i]}
#	INPUT_FILE_NAME_PREFIX="$INPUT_FILE_NAME_PREFIX."
#	i=`expr $i + 1`	
#done
#INPUT_FILE_NAME_PREFIX="$INPUT_FILE_NAME_PREFIX${arrIN[N]}"



#echo $INPUT_FILE_NAME_PREFIX


# Remove the old OUTPUT_DIR if it exists
if [ -d "$OUTPUT_DIR" ]; then rm -Rf $OUTPUT_DIR; fi

mkdir $OUTPUT_DIR

# Copy the source file to the OutputDir
cp $WORKING_DIR/$INPUT_FILE_NAME $OUTPUT_DIR/$INPUT_FILE_NAME

# Change to the output directory
cd $OUTPUT_DIR

# Compile with flags for gcov
gcc -fprofile-arcs -ftest-coverage -o $OUTPUT_FILE_NAME $INPUT_FILE_NAME

# Run the compiled File (we are now in the build directory)
./$OUTPUT_FILE_NAME

# Remove the compiled file
#rm $WORKING_DIR/$OUTPUT_FILE_NAME


# Move all the important Files to the OUTPUT_DIR!
#mv $WORKING_DIR/$INPUT_FILE_NAME_PREFIX.gcda $OUTPUT_DIR/$INPUT_FILE_NAME_PREFIX.gcda
#mv $WORKING_DIR/$INPUT_FILE_NAME_PREFIX.gcno $OUTPUT_DIR/$INPUT_FILE_NAME_PREFIX.gcno
#cp $WORKING_DIR/$INPUT_FILE_NAME $OUTPUT_DIR/$INPUT_FILE_NAME


# Run GCOV
gcov $OUTPUT_DIR/$INPUT_FILE_NAME


#RUN LCOV
lcov -c -d . -o coverage.info

#Generate the HTML
mkdir $OUTPUT_HTML_DIR
genhtml coverage.info -o $OUTPUT_HTML_DIR




#!/bin/bash

# define this to disable overwrites 
PROTECT=0

# takes one arg: the name of the .root file to be converted. 
# arg=""


# check number of args supplied 
if [[ $# -ne 2 ]]
then
    echo $#
    echo "ERROR: there should be 2 args"
    exit -1
fi




# check that a .root was supplied 
if [[ "${1##*.}" = "root" ]]
then
    prefix=$(basename $1)
    prefix=${prefix%.*}
    echo 'INFO: processing ' $prefix
else
    echo ${1##*.}
    echo "ERROR: file supplied doesn't have .root suffix."
    exit -1
fi







FILENAME=$1

if [[ $2 = '-d' ]]
then
    OUTPUT_DIR=$(dirname $1)/../extracted_root_tree_data/$prefix/
else
    OUTPUT_DIR=$2/$prefix/
fi


    
CODE_DIR=$(dirname $0 )/

# echo $CODE_DIR
echo 'Output directory = '${OUTPUT_DIR} 

# give warning if this dir exists
if [[ -d $OUTPUT_DIR ]]
then
    if [[ $PROTECT == 1 ]]
    then
	echo "ERROR: directory already exists and PROTECT is enabled, exiting." 
	exit -1
    else
	echo "INFO: directory already exists and PROTECT is disabled. contents will be overwritten."
    fi
fi


mkdir -p $OUTPUT_DIR

for d in $(seq 1 1 4)
do
    for i in $(seq 0 1 31)
    do
	for j in $(seq 0 1 31)
	do

	    mkdir -p $OUTPUT_DIR/$d/$i/$j

	done	  
    done
done


# run it
root -q "${CODE_DIR}ttree_to_bin.cpp(\"$1\", \"${OUTPUT_DIR}\")"


# for d in $ 

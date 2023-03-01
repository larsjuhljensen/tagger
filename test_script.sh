#!/bin/bash

# the first para is the name of output
printf "Define the outputs name: [prefix name]\n"
printf "default: (without name) >>> "
read -r try_num
#try_num=$1

# the second para is whether to use Wc or not (yes or no)
printf "Decide whether to use the Wc or not: [yes|no]\n"
printf "default:no >>> "
read -r use_Wc
#use_Wc=$2

input="$(pwd)/data"
output="$(pwd)/out"

if [ -d $output ]; then
	echo "dict out exsists"
else
	mkdir out
	echo "dict out is created"
fi

if [ $use_Wc == 'y' ] || [ $use_Wc == 'Y' ] || [ $use_Wc == 'yes' ] || [ $use_Wc == 'Yes' ]; then
	### with corpus weights
	$(pwd)/tagcorpus --types=$input/test_types.tsv --entities=$input/test_entities.tsv --names=$input/test_names.tsv --documents=$input/test_documents.tsv --out-matches=$output/${try_num}_test_matches.tsv --out-pairs=$output/${try_num}_test_pairs.tsv --out-segments=$output/${try_num}_test_segments.tsv --corpus-weights=$input/test_Wc.tsv
else
	### without corpus weights
	$(pwd)/tagcorpus --types=$input/test_types.tsv --entities=$input/test_entities.tsv --names=$input/test_names.tsv --documents=$input/test_documents.tsv --out-matches=$output/${try_num}_test_matches.tsv --out-pairs=$output/${try_num}_test_pairs.tsv --out-segments=$output/${try_num}_test_segments.tsv
fi


#!/bin/bash

try_num=$1
input="$(pwd)/data"
output="$(pwd)/out"

if [ -d $output ]; then
	echo "dict out exsists"
else
	mkdir out
	echo "dict out is created"
fi

$(pwd)/tagcorpus --types=$input/test_types.tsv --entities=$input/test_entities.tsv --names=$input/test_names.tsv --documents=$input/test_documents.tsv --out-matches=$output/${try_num}_test_matches.tsv --out-pairs=$output/${try_num}_test_pairs.tsv --out-segments=$output/${try_num}_test_segments.tsv


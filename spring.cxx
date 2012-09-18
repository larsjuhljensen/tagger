#include "document.h"
#include "match_handlers.h"
#include "meta_handlers.h"
#include "print_handlers.h"
#include "score_handlers.h"
#include "threaded_batch_tagger.h"

#include <fstream>
#include <iostream>

using namespace std;

int main (int argc, char *argv[])
{
	assert(argc >= 4);
	
	TsvDocumentReader document_reader = TsvDocumentReader(stdin);
	EntityTypeMap* entity_type_map = NULL;
	GetMatchesParams params;
	MetaBatchHandler batch_handler;
	ThreadedBatchTagger batch_tagger;
	
	int threads = atoi(argv[1]);
	
	batch_tagger.load_names(-2, "species_names.tsv");
	batch_tagger.load_global("species_global.tsv");
	entity_type_map = new EntityTypeMap(argv[2]);
	
	batch_handler.push_back(new PrintBatchHandler(argv[2]));
	batch_handler.push_back(new ScoreBatchHandler(argv[3], entity_type_map, 1.0, 2.0, 0.0, 0.6));
	
	params.auto_detect = false;
	params.entity_types.push_back(-2);
	params.find_acronyms = false;
	
	batch_tagger.process(threads, &document_reader, params, &batch_handler);
	
}

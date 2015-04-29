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
	
	cerr << "# Loading data ...";
	batch_tagger.load_names(argv[2], argv[3]);
	if (argc >= 5) {
		batch_tagger.load_global(argv[4]);
	}
	if (argc >= 6) {
		batch_tagger.load_local(argv[5]);
	}
	if (argc >= 7) {
		entity_type_map = new EntityTypeMap(argv[2]);
		batch_tagger.load_groups(entity_type_map, argv[6]);
	}
	cerr << " done." << endl;
	
	if (argc < 8) {
		batch_handler.push_back(new PrintBatchHandler(stdout));
	}
	else if (argc == 11) {
		batch_handler.push_back(new ScoreBatchHandler(stdout, entity_type_map, atof(argv[7]), atof(argv[8]), atof(argv[9]), atof(argv[10])));
	}
	else if (argc == 12) {
		batch_handler.push_back(new SelectiveScoreBatchHandler(stdout, entity_type_map, atof(argv[7]), atof(argv[8]), atof(argv[9]), atof(argv[10]), argv[11]));
	}
	else if (argc == 13) {
		batch_handler.push_back(new PrintBatchHandler(argv[11]));
		batch_handler.push_back(new ScoreBatchHandler(argv[12], entity_type_map, atof(argv[7]), atof(argv[8]), atof(argv[9]), atof(argv[10])));
	}
	else if (argc == 14) {
		batch_handler.push_back(new PrintBatchHandler(argv[12]));
		batch_handler.push_back(new SelectiveScoreBatchHandler(argv[13], entity_type_map, atof(argv[7]), atof(argv[8]), atof(argv[9]), atof(argv[10]), argv[11]));
	}
	else {
		cerr << " wrong number of arguments." << endl;
	}
	
	params.auto_detect = true;
	params.entity_types.push_back(9606); // STRING human proteins. 
	params.entity_types.push_back(-1);   // STITCH chemicals.
	params.entity_types.push_back(-2);   // NCBI taxonomy.
	params.entity_types.push_back(-11);  // Wikipedia.
	params.entity_types.push_back(-21);  // GO biological process.
	params.entity_types.push_back(-22);  // GO cellular component.
	params.entity_types.push_back(-23);  // GO molecular function.
	params.entity_types.push_back(-24);  // GO other (unused).
	params.entity_types.push_back(-25);  // BTO tissues.
	params.entity_types.push_back(-26);  // DOID diseases.
	params.entity_types.push_back(-27);  // ENVO environments.
	params.entity_types.push_back(-28);  // APO phenotypes.
	params.entity_types.push_back(-29);  // FYPO phenotypes.
	params.entity_types.push_back(-30);  // MPheno phenotypes.
	params.entity_types.push_back(-31);  // NBO behaviors.
	params.entity_types.push_back(-32);
	params.entity_types.push_back(-33);
	
	#ifdef REFLECT_PROFILE
	profile_print();
	#endif
	
	batch_tagger.process(threads, &document_reader, params, &batch_handler);
	cerr << endl << "# Batch done." << endl;
	
	#ifdef REFLECT_PROFILE
	profile_print();
	#endif
	
}

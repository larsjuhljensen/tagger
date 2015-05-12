#include "document.h"
#include "match_handlers.h"
#include "meta_handlers.h"
#include "print_handlers.h"
#include "score_handlers.h"
#include "threaded_batch_tagger.h"

#include <fstream>
#include <iostream>

extern "C"
{
    #include <getopt.h>
}

#define MAXFILENAMELEN 256

using namespace std;

int validate_req (char* var, const char name[]) {
    if (strcmp(var, "") == 0) {
        printf("Must specify %s file with --%s=filename\n", name, name);
        exit(1);
    }
    return 1;
}

int validate_opt(char* var) {
    return strcmp(var, "");
}

int main (int argc, char *argv[])
{
	GetMatchesParams params;
	MetaBatchHandler batch_handler;
	ThreadedBatchTagger batch_tagger;

	// some default values for the command line arguments
	int threads = 1;
	char documents[MAXFILENAMELEN] = "";
	char entities[MAXFILENAMELEN] = "";
	char names[MAXFILENAMELEN] = "";
	char groups[MAXFILENAMELEN] = "";
	char stopwords[MAXFILENAMELEN] = "";
	int organism = 9606;
	char type_pairs[MAXFILENAMELEN] = "";
	char out_matches[MAXFILENAMELEN] = "";
	char out_pairs[MAXFILENAMELEN] = "";
	float document_weight = 1;
	float paragraph_weight = 2;
	float sentence_weight = 0.2;
	float normalization_factor = 0.6;
	
	int c; // parse command line arguments
	while (1) {
		static struct option long_options[] =
		{
			{"threads", optional_argument, 0, 't'},
			{"entities", required_argument, 0, 'e'},
			{"names", required_argument, 0, 'n'},
			{"groups", optional_argument, 0, 'g'},
			{"stopwords", optional_argument, 0, 's'},
			{"organism", optional_argument, 0, 'o'},
			{"document-weight", optional_argument, 0, 'd'},
			{"paragraph-weight", optional_argument, 0, 'r'},
			{"sentence-weight", optional_argument, 0, 'c'},
			{"normalization-factor", optional_argument, 0, 'f'},
			{"type-pairs", optional_argument, 0, 'p'},
			{"out-matches", optional_argument, 0, 'm'},
			{"out-pairs", optional_argument, 0, 'a'},
			{"documents", optional_argument, 0, 'i'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		
		c = getopt_long (argc, argv, "t:e:n:g:s:o:d:r:c:f:o:m:a:l:h:", long_options, &option_index);
		
		/* Detect the end of the options. */
		if (c == -1)
			break;
		
		switch (c) {
			
			case 'h':
				printf("Usage: %s [OPTIONS]\n", argv[0]);
				printf("Required Arguments\n");
				printf("\t--entities=filename\n");
				printf("\t--names=filename\n");
				printf("Optional Arguments\n");
				printf("\t--documents=filename\tRead input from file instead of from STDIN\n");
				printf("\t--threads=%d\n", threads);
				printf("\t--stopwords=filename\n");
				printf("\t--organism=%d\n", organism);
				printf("\t--document-weight=%1.2f\n", document_weight);
				printf("\t--paragraph-weight=%1.2f\n", paragraph_weight);
				printf("\t--sentence-weight=%1.2f\n", sentence_weight);
				printf("\t--normalization-factor=%1.2f\n", normalization_factor);
				printf("\t--type-pairs=filename\tTypes of pairs that are allowed\n");
				printf("\t--groups=filename\n");
				printf("\t--out-matches=filename\n");
				printf("\t--out-pairs=filename\n");
				exit(0);
				break;
			
			case 't':
				if (optarg) {
					threads = atoi(optarg);
				}
				break;
			
			case 'e':
				if (optarg) {
					strncpy(entities, optarg, min(MAXFILENAMELEN, int(sizeof(entities))));
				}
				break;
			
			case 'n':
				if (optarg) {
					strncpy(names, optarg, min(MAXFILENAMELEN, int(sizeof(names))));
				}
				break;
			
			case 'g':
				if (optarg) {
					strncpy(groups, optarg, min(MAXFILENAMELEN, int(sizeof(groups))));
				}
				break;
			
			case 's':
				if (optarg) {
					strncpy(stopwords, optarg, min(MAXFILENAMELEN, int(sizeof(stopwords))));
				}
				break;
			
			case 'o':
				if (optarg) {
					organism = atoi(optarg);
				}
				break;
			
			case 'd':
				if (optarg) {
					document_weight = atof(optarg);
				}
				break;
			
			case 'r':
				if (optarg) {
					paragraph_weight = atof(optarg);
				}
				break;
			
			case 'c':
				if (optarg) {
					sentence_weight = atof(optarg);
				}
				break;
			
			case 'f':
				if (optarg) {
					normalization_factor = atof(optarg);
				}
				break;
			
			case 'm':
				if (optarg) {
					strncpy(out_matches, optarg, min(MAXFILENAMELEN, int(sizeof(out_matches))));
				}
				break;
			
			case 'a':
				if (optarg) {
					strncpy(out_pairs, optarg, min(MAXFILENAMELEN, int(sizeof(out_pairs))));
				}
				break;
			
			case 'p':
				if (optarg) {
					strncpy(type_pairs, optarg, min(MAXFILENAMELEN, int(sizeof(type_pairs))));
				}
				break;
			
			case 'i':
				if (optarg) {
					strncpy(documents, optarg, min(MAXFILENAMELEN, int(sizeof(documents))));
				}
				break;
			
			case '?':
				/* getopt_long already printed an error message. */
				break;
			
			default:
				abort ();
		}
	
	}
	
	validate_req(entities, "entities");
	validate_req(names, "names");
	
	cerr << "# Loading data ...";
	batch_tagger.load_names(entities, names);
	if (validate_opt(groups)) {
		batch_tagger.load_groups(groups);
	}
	if (validate_opt(stopwords)) {
		batch_tagger.load_global(stopwords);
	}
	cerr << " done." << endl;
	
	TsvDocumentReader *document_reader;
	if (validate_opt(documents)) {
		document_reader = new TsvDocumentReader(documents);
	}
	else {
		document_reader = new TsvDocumentReader(stdin);
	}
	
	if (validate_opt(out_matches)) {
		batch_handler.push_back(new PrintBatchHandler(out_matches));
	}
	else {
		batch_handler.push_back(new PrintBatchHandler(stdout));	
	}
	
	if (validate_opt(out_pairs)) {
		if (validate_opt(type_pairs)) {
			batch_handler.push_back(new SelectiveScoreBatchHandler(out_pairs, batch_tagger.entity_type_map, document_weight, paragraph_weight, sentence_weight, normalization_factor, type_pairs));
		}
		else {
			batch_handler.push_back(new ScoreBatchHandler(out_pairs, batch_tagger.entity_type_map, document_weight, paragraph_weight, sentence_weight, normalization_factor));
		}
	}
	
	params.auto_detect = true;
	params.entity_types.push_back(organism);	// STRING human proteins. 
	params.entity_types.push_back(-1);		// STITCH chemicals.
	params.entity_types.push_back(-2);		// NCBI taxonomy.
	params.entity_types.push_back(-11);		// Wikipedia.
	params.entity_types.push_back(-21);		// GO biological process.
	params.entity_types.push_back(-22);		// GO cellular component.
	params.entity_types.push_back(-23);		// GO molecular function.
	params.entity_types.push_back(-24);		// GO other (unused).
	params.entity_types.push_back(-25);		// BTO tissues.
	params.entity_types.push_back(-26);		// DOID diseases.
	params.entity_types.push_back(-27);		// ENVO environments.
	params.entity_types.push_back(-28);		// APO phenotypes.
	params.entity_types.push_back(-29);		// FYPO phenotypes.
	params.entity_types.push_back(-30);		// MPheno phenotypes.
	params.entity_types.push_back(-31);		// NBO behaviors.
	params.entity_types.push_back(-32);
	params.entity_types.push_back(-33);
	
	batch_tagger.process(threads, document_reader, params, &batch_handler);
	cerr << endl << "# Batch done." << endl;
	
	exit(0);
}
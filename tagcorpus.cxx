#include "document.h"
#include "match_handlers.h"
#include "meta_handlers.h"
#include "print_handlers.h"
#include "score_handlers.h"
#include "segment_handlers.h"
#include "threaded_batch_tagger.h"

#include <fstream>
#include <iostream>

extern "C"
{
	#include <getopt.h>
}

#define MAXFILENAMELEN 256
#define VERSION "1.1"

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
	MetaBatchHandler batch_handler;
	ThreadedBatchTagger batch_tagger;

	// some default values for the command line arguments
	char types[MAXFILENAMELEN] = "";
	char entities[MAXFILENAMELEN] = "";
	char names[MAXFILENAMELEN] = "";
	char documents[MAXFILENAMELEN] = "";
	char groups[MAXFILENAMELEN] = "";
	char type_pairs[MAXFILENAMELEN] = "";
	char stopwords[MAXFILENAMELEN] = "";
	bool autodetect = false;
	bool tokenize_characters = false;
	float document_weight = 1;
	float paragraph_weight = 2;
	float sentence_weight = 0.2;
	float normalization_factor = 0.6;
	int threads = 1;
	char out_matches[MAXFILENAMELEN] = "";
	char out_pairs[MAXFILENAMELEN] = "";
	char out_segments[MAXFILENAMELEN] = "";
	
	int c; // parse command line arguments
	while (1) {
		static struct option long_options[] =
		{
			{"types", required_argument, 0, 'y'},
			{"entities", required_argument, 0, 'e'},
			{"names", required_argument, 0, 'n'},
			{"documents", optional_argument, 0, 'i'},
			{"groups", optional_argument, 0, 'g'},
			{"type-pairs", optional_argument, 0, 'p'},
			{"stopwords", optional_argument, 0, 's'},
			{"autodetect", no_argument, 0, 'u'},
			{"tokenize-characters", no_argument, 0, 'z'},
			{"document-weight", optional_argument, 0, 'd'},
			{"paragraph-weight", optional_argument, 0, 'r'},
			{"sentence-weight", optional_argument, 0, 'c'},
			{"normalization-factor", optional_argument, 0, 'f'},
			{"threads", optional_argument, 0, 't'},
			{"out-matches", optional_argument, 0, 'm'},
			{"out-pairs", optional_argument, 0, 'a'},
			{"out-segments", optional_argument, 0, 'b'},
			{"version", no_argument, 0, 'v'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};
		
		int option_index = 0;
		
		c = getopt_long (argc, argv, "y:e:n:i:g:p:s:u:d:r:c:f:t:m:a:h:", long_options, &option_index);
		
		/* Detect the end of the options. */
		if (c == -1)
			break;
		
		switch (c) {
			
			case 'h':
				printf("Usage: %s [OPTIONS]\n", argv[0]);
				printf("Required Arguments\n");
				printf("\t--types=filename\n");
				printf("\t--entities=filename\n");
				printf("\t--names=filename\n");
				printf("Optional Arguments\n");
				printf("\t--documents=filename\tRead input from file instead of from STDIN\n");
				printf("\t--groups=filename\n");
				printf("\t--type-pairs=filename\tTypes of pairs that are allowed\n");
				printf("\t--stopwords=filename\n");
				printf("\t--autodetect Turn autodetect on\n");
				printf("\t--tokenize-characters Turn single-character tokenization on\n");
				printf("\t--document-weight=%1.2f\n", document_weight);
				printf("\t--paragraph-weight=%1.2f\n", paragraph_weight);
				printf("\t--sentence-weight=%1.2f\n", sentence_weight);
				printf("\t--normalization-factor=%1.2f\n", normalization_factor);
				printf("\t--threads=%d\n", threads);
				printf("\t--out-matches=filename\n");
				printf("\t--out-pairs=filename\n");
				printf("\t--out-segments=filename\n");
				exit(0);
			
			case 'v':
				printf("JensenLab Tagger version %s\n", VERSION);
				exit(0);
			
			case 'y':
				if (optarg) {
					strncpy(types, optarg, min(MAXFILENAMELEN, int(sizeof(types))));
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
			
			case 'i':
				if (optarg) {
					strncpy(documents, optarg, min(MAXFILENAMELEN, int(sizeof(documents))));
				}
				break;
			
			case 'g':
				if (optarg) {
					strncpy(groups, optarg, min(MAXFILENAMELEN, int(sizeof(groups))));
				}
				break;
			
			case 'p':
				if (optarg) {
					strncpy(type_pairs, optarg, min(MAXFILENAMELEN, int(sizeof(type_pairs))));
				}
				break;
			
			case 's':
				if (optarg) {
					strncpy(stopwords, optarg, min(MAXFILENAMELEN, int(sizeof(stopwords))));
				}
				break;
			
			case 'u':
				autodetect = true;
				break;
			
			case 'z':
				tokenize_characters = true;
			
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
			
			case 't':
				if (optarg) {
					threads = atoi(optarg);
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
			
			case 'b':
				if (optarg) {
					strncpy(out_segments, optarg, min(MAXFILENAMELEN, int(sizeof(out_segments))));
				}
				break;
			
			case '?':
				/* getopt_long already printed an error message. */
				break;
			
			default:
				abort ();
		}
	
	}
	
	validate_req(types, "types");
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
	
	if (validate_opt(out_segments)) {
		batch_handler.push_back(new SegmentBatchHandler(out_segments));
	}
	
	GetMatchesParams params(types);
	params.auto_detect = autodetect;
	params.tokenize_characters = tokenize_characters;

	batch_tagger.process(threads, document_reader, params, &batch_handler);
	cerr << endl << "# Batch done." << endl;
	
	exit(0);
}

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
	TsvDocumentReader document_reader = TsvDocumentReader(stdin);
	EntityTypeMap* entity_type_map = NULL;
	GetMatchesParams params;
	MetaBatchHandler batch_handler;
	ThreadedBatchTagger batch_tagger;

    // some default values for the command line arguments
    int threads = 16;
    char entities[MAXFILENAMELEN] = "";
    char names[MAXFILENAMELEN] = "";
    char stoplist[MAXFILENAMELEN] = "";
    char local[MAXFILENAMELEN] = "";
    char groups[MAXFILENAMELEN] = "";
    char pairs[MAXFILENAMELEN] = "";
    char all_matches[MAXFILENAMELEN] = "";
    char all_pairs[MAXFILENAMELEN] = "";
    float document_weight = 1;
    float paragraph_weight = 2;
    float sentence_weight = 0.2;
    float factor_exponent = 0.6;

    int c; // parse command line arguments
    while (1) {
        static struct option long_options[] =
        {
            {"threads", required_argument, 0, 't'},
            {"entities", required_argument, 0, 'e'},
            {"names", required_argument, 0, 'n'},
            {"stoplist", required_argument, 0, 's'},
            {"local", optional_argument, 0, 'l'},
            {"groups", optional_argument, 0, 'g'},
            {"document_weight", optional_argument, 0, 'd'},
            {"paragraph_weight", optional_argument, 0, 'r'},
            {"sentence_weight", optional_argument, 0, 'c'},
            {"factor_exponent", optional_argument, 0, 'f'},
            {"all_matches", optional_argument, 0, 'm'},
            {"all_pairs", optional_argument, 0, 'a'},
            {"pairs", optional_argument, 0, 'p'},
            {"help", no_argument, 0, 'h'},
            {0, 0, 0, 0}
        };

        int option_index = 0;

        c = getopt_long (argc, argv, "t:e:n:s:l:g:p:d:r:c:f:", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
           
            case 'h':
                printf("Usage: ... \n");
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
            
            case 's':
                if (optarg) {
                    strncpy(stoplist, optarg, min(MAXFILENAMELEN, int(sizeof(stoplist))));
                }
                break;

            case 'l':
                if (optarg) {
                    strncpy(local, optarg, min(MAXFILENAMELEN, int(sizeof(local))));
                }
                break;

            case 'g':
                if (optarg) {
                    strncpy(groups, optarg, min(MAXFILENAMELEN, int(sizeof(groups))));
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
                    factor_exponent = atof(optarg);
                }
                break;

            case 'm':
                if (optarg) {
                    strncpy(all_matches, optarg, min(MAXFILENAMELEN, int(sizeof(all_matches))));
                }
                break;

            case 'a':
                if (optarg) {
                    strncpy(all_pairs, optarg, min(MAXFILENAMELEN, int(sizeof(all_pairs))));
                }
                break;

            case 'p':
                if (optarg) {
                    strncpy(pairs, optarg, min(MAXFILENAMELEN, int(sizeof(pairs))));
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
    validate_req(stoplist, "stoplist");


	cerr << "# Loading data ...";
	batch_tagger.load_names(entities, names);

    if (validate_opt(stoplist)) {
		batch_tagger.load_global(stoplist);
    }
    if (validate_opt(local)) {
		batch_tagger.load_local(local);
    }
    if (validate_opt(groups)) {
		entity_type_map = new EntityTypeMap(entities);
		batch_tagger.load_groups(entity_type_map, groups);
    }

	cerr << " done." << endl;

    
    
    if (validate_opt(all_pairs)) {
        if (validate_opt(all_matches)) {
		    batch_handler.push_back(new PrintBatchHandler(all_matches));
        }
        if (validate_opt(pairs)) {
		    batch_handler.push_back(new SelectiveScoreBatchHandler(all_pairs, entity_type_map, document_weight, paragraph_weight, sentence_weight, factor_exponent, pairs));
        }
        else {
            batch_handler.push_back(new ScoreBatchHandler(all_pairs, entity_type_map, document_weight, paragraph_weight, sentence_weight, factor_exponent));
        }
    }
    else {
        if (validate_opt(pairs)) {
            batch_handler.push_back(new SelectiveScoreBatchHandler(stdout, entity_type_map, document_weight, paragraph_weight, sentence_weight, factor_exponent, pairs));
        }
        else {
            batch_handler.push_back(new ScoreBatchHandler(stdout, entity_type_map, document_weight, paragraph_weight, sentence_weight, factor_exponent));
        }
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


    exit(0);
}



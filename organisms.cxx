#include "document.h"
#include "match_handlers.h"
#include "batch_tagger.h"

using namespace std;

class SimpleBatchHandler : public BatchHandler
{
	public:
		void on_match(Document& document, Match* match)
		{
			char replaced = document.text[match->stop+1];
			document.text[match->stop+1] = '\0';
			Entity* entity = match->entities;
			for (int i = 0; i < match->size; i++) {
				printf("%s\t%d\t%d\t%s\t%d\n", document.name, match->start, match->stop, (const char*)(document.text+match->start), entity->id.serial);
				entity++;
			}
			document.text[match->stop+1] = replaced;
		};
};

////////////////////////////////////////////////////////////////////////////////

int main (int argc, char *argv[])
{
	assert(argc >= 2);
	
	BatchTagger batch_tagger;
	batch_tagger.load_names(-2, "organisms_names.tsv");
	batch_tagger.load_groups(-2, "organisms_groups.tsv");
	batch_tagger.load_global("organisms_global.tsv");
	
	DirectoryDocumentReader document_reader = DirectoryDocumentReader(argv[1]);
	GetMatchesParams params;
	params.auto_detect = false;
	params.entity_types.push_back(-2);
	params.max_tokens = 6;
	SimpleBatchHandler batch_handler;
	
	batch_tagger.process(&document_reader, params, &batch_handler);	
}

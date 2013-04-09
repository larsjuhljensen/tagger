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
				if (entity->id.serial >= 10000 && entity->id.serial < 10200) {
					printf("%s\t%d\t%d\t%s\tENVO:%07d\n", document.name, match->start, match->stop, (const char*)(document.text+match->start), entity->id.serial);
				}
				else {
					printf("%s\t%d\t%d\t%s\tENVO:%08d\n", document.name, match->start, match->stop, (const char*)(document.text+match->start), entity->id.serial);
				}
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
	batch_tagger.load_names(-27, "environments_names.tsv");
	batch_tagger.load_groups(-27, "environments_groups.tsv");
	batch_tagger.load_global("environments_global.tsv");
	
	DirectoryDocumentReader document_reader = DirectoryDocumentReader(argv[1]);
	GetMatchesParams params;
	params.auto_detect = false;
	params.entity_types.push_back(-27);
	params.max_tokens = 6;
	SimpleBatchHandler batch_handler;
	
	batch_tagger.process(&document_reader, params, &batch_handler);	
}

#ifndef __REFLECT_BATCH_TAGGER_HEADER__
#define __REFLECT_BATCH_TAGGER_HEADER__

#include "base_handlers.h"
#include "tagger.h"

#include <utility>
#include <vector>
#include <tr1/unordered_set>
#include <tr1/unordered_map>

using namespace std;
using namespace std::tr1;

class DocumentTagger : public Tagger
{
	private:
		EntityTypeMap* entity_type_map;
 		IMatchHandler* match_handler;
		
	public:
		DocumentTagger();
		~DocumentTagger();
		
	public:
		void load_groups(int type, const char* groups_filename);
		void load_groups(EntityTypeMap* entities_type_map, const char* groups_filename);
		void load_groups(const char* entities_filename, const char* groups_filename);
		
	public:
		void process(Document& document, const GetMatchesParams& params, IDocumentHandler* document_handler);
		void process(IDocumentReader* document_reader, const GetMatchesParams& params, IDocumentHandler* document_handler);
};

class BatchTagger : public DocumentTagger
{
	public:
		BatchTagger();
		~BatchTagger();
		
	public:
		void process(IDocumentReader* document_reader, const GetMatchesParams& params, IBatchHandler* batch_handler);
};


////////////////////////////////////////////////////////////////////////////////

DocumentTagger::DocumentTagger()
 : Tagger(true)
{
	this->match_handler = new DisambiguationMatchHandler();
}

DocumentTagger::~DocumentTagger()
{
}

void DocumentTagger::load_groups(int type, const char* groups_filename)
{
	this->match_handler = new GroupMatchHandler(type, groups_filename);
}

void DocumentTagger::load_groups(EntityTypeMap* entity_type_map, const char* groups_filename)
{
	this->entity_type_map = entity_type_map;
	delete this->match_handler;
	this->match_handler = new GroupMatchHandler(this->entity_type_map, groups_filename);
}

void DocumentTagger::load_groups(const char* entities_filename, const char* groups_filename)
{
	load_groups(new EntityTypeMap(entities_filename), groups_filename);
}

void DocumentTagger::process(Document& document, const GetMatchesParams& params, IDocumentHandler* document_handler)
{
	Matches matches = Tagger::get_matches(document.text, NULL, params);
	this->match_handler->process(matches);
	document_handler->process(document, matches);
	for (Matches::iterator it = matches.begin(); it != matches.end(); it++) {
		delete *it;
	}
}

void DocumentTagger::process(IDocumentReader* document_reader, const GetMatchesParams& params, IDocumentHandler* document_handler)
{
	while (Document* document = document_reader->read_document()) {
		process(*document, params, document_handler);
		delete document;
	}
}

////////////////////////////////////////////////////////////////////////////////

BatchTagger::BatchTagger()
 : DocumentTagger()
{
}

BatchTagger::~BatchTagger()
{
}

void BatchTagger::process(IDocumentReader* document_reader, const GetMatchesParams& params, IBatchHandler* batch_handler)
{
	IDocumentHandler* document_handler = batch_handler->create_document_handler();
	batch_handler->on_batch_begin();
	document_handler->on_batch_begin();
	DocumentTagger::process(document_reader, params, document_handler);
	document_handler->on_batch_end();
	batch_handler->on_batch_end();
	delete document_handler;
}

#endif

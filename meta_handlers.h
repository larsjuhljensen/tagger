#ifndef __REFLECT_META_HANDLERS_HEADER__
#define __REFLECT_META_HANDLERS_HEADER__

#include "base_handlers.h"

#include <vector>

using namespace std;

class MetaDocumentHandler : public DocumentHandler, public vector<IDocumentHandler*>
{
	public:
		MetaDocumentHandler(IBatchHandler* batch_handler);
		
	public:
		void on_batch_begin();
		void on_batch_end();
		void process(Document& document, Matches& matches);
};

class MetaBatchHandler : public BatchHandler, public vector<IBatchHandler*>
{
	public:
		void on_batch_begin();
		void on_batch_end();
		virtual IDocumentHandler* create_document_handler();
};

////////////////////////////////////////////////////////////////////////////////

MetaDocumentHandler::MetaDocumentHandler(IBatchHandler* batch_handler)
 : DocumentHandler(batch_handler)
{
}

void MetaDocumentHandler::on_batch_begin()
{
	for (MetaDocumentHandler::iterator it = this->begin(); it != this->end(); it++) {
		(*it)->on_batch_begin();
	}
}

void MetaDocumentHandler::on_batch_end()
{
	for (MetaDocumentHandler::iterator it = this->begin(); it != this->end(); it++) {
		(*it)->on_batch_end();
	}
}

void MetaDocumentHandler::process(Document& document, Matches& matches)
{
	for (MetaDocumentHandler::iterator it = this->begin(); it != this->end(); it++) {
		(*it)->process(document, matches);
	}
}

////////////////////////////////////////////////////////////////////////////////

IDocumentHandler* MetaBatchHandler::create_document_handler()
{
	MetaDocumentHandler* document_handler = new MetaDocumentHandler(this);
	for (MetaBatchHandler::iterator it = this->begin(); it != this->end(); it++) {
		document_handler->push_back((IDocumentHandler*)(*it)->create_document_handler());
	}
	return document_handler;
}

void MetaBatchHandler::on_batch_begin()
{
	for (MetaBatchHandler::iterator it = this->begin(); it != this->end(); it++) {
		(*it)->on_batch_begin();
	}
}

void MetaBatchHandler::on_batch_end()
{
	for (MetaBatchHandler::iterator it = this->begin(); it != this->end(); it++) {
		(*it)->on_batch_end();
	}
}

#endif

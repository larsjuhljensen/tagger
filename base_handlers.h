#ifndef __REFLECT_BASE_HANDLERS_HEADER__
#define __REFLECT_BASE_HANDLERS_HEADER__

#include "document.h"
#include "tagger_types.h"

using namespace std;
using namespace std::tr1;

typedef vector<SERIAL> SERIALS;
typedef unordered_set<SERIAL> ENTITY_SET;

class IBatchHandler;

class IDocumentHandler
{
	public:
		virtual void on_batch_begin() = 0;
		virtual void on_batch_end() = 0;
		virtual void on_document_begin(Document& document) = 0;
		virtual void on_document_end(Document& document) = 0;
		virtual void on_match(Document& document, Match* match) = 0;
		virtual void process(Document& document, Matches& matches) = 0;
};

class DocumentHandler : public IDocumentHandler
{
	protected:
		IBatchHandler* batch_handler;
		
	public:
		DocumentHandler(IBatchHandler* batch_handler);
		
	public:
		virtual void on_batch_begin();
		virtual void on_batch_end();
		virtual void on_document_begin(Document& document);
		virtual void on_document_end(Document& document);
		virtual void on_match(Document& document, Match* match);
		void process(Document& document, Matches& matches);
};

class StructuredDocumentHandler : public DocumentHandler
{
	public:
		StructuredDocumentHandler(IBatchHandler* batch_handler);
		
	public:
		virtual void on_paragraph_begin();
		virtual void on_paragraph_end();
		virtual void on_sentence_begin();
		virtual void on_sentence_end();
		void process(Document& document, Matches& matches);
};

class IBatchHandler
{
	public:
		virtual IDocumentHandler* create_document_handler() = 0;
		virtual void on_batch_begin() = 0;
		virtual void on_batch_end() = 0;
		virtual void on_document_begin(Document& document) = 0;
		virtual void on_document_end(Document& document) = 0;
		virtual void on_match(Document& document, Match* match) = 0;
};

class BatchHandler : public IBatchHandler
{
	public:
		virtual IDocumentHandler* create_document_handler();
		virtual void on_batch_begin();
		virtual void on_batch_end();
		virtual void on_document_begin(Document& document);
		virtual void on_document_end(Document& document);
		virtual void on_match(Document& document, Match* match);
};

////////////////////////////////////////////////////////////////////////////////

DocumentHandler::DocumentHandler(IBatchHandler* batch_handler)
{
	this->batch_handler = batch_handler;
}

void DocumentHandler::on_batch_begin() {
}

void DocumentHandler::on_batch_end() {
}

void DocumentHandler::on_document_begin(Document& document) {
}

void DocumentHandler::on_document_end(Document& document) {
}

void DocumentHandler::on_match(Document& document, Match* match)
{
}

void DocumentHandler::process(Document& document, Matches& matches)
{
	this->batch_handler->on_document_begin(document);
	this->on_document_begin(document);
	for (Matches::iterator match = matches.begin(); match != matches.end(); match++) {
		this->on_match(document, *match);
		this->batch_handler->on_match(document, *match);
	}
	this->on_document_end(document);
	this->batch_handler->on_document_end(document);
}

////////////////////////////////////////////////////////////////////////////////

StructuredDocumentHandler::StructuredDocumentHandler(IBatchHandler* batch_handler)
 : DocumentHandler(batch_handler)
{
}

void StructuredDocumentHandler::on_paragraph_begin() {
}

void StructuredDocumentHandler::on_paragraph_end() {
}

void StructuredDocumentHandler::on_sentence_begin() {
}

void StructuredDocumentHandler::on_sentence_end() {
}

void StructuredDocumentHandler::process(Document& document, Matches& matches)
{
	this->batch_handler->on_document_begin(document);
	this->on_document_begin(document);
	Matches::iterator match = matches.begin();
	Segments segments = document.get_segments();
	Segments::iterator segment = segments.begin();
	while (match != matches.end()) {
		if (segment->paragraph_begin) {
			this->on_paragraph_begin();
		}
		this->on_sentence_begin();
		while (match != matches.end() && document.text+(*match)->start < segment->begin) {
			match++;
		}
		if (match == matches.end()) {
			this->on_sentence_end();
			break;
		}
		if (document.text+(*match)->stop < segment->end) {
			while (match != matches.end() && document.text+(*match)->stop < segment->end) {
				this->on_match(document, *match);
				this->batch_handler->on_match(document, *match);
				match++;
			}
		}
		this->on_sentence_end();
		if (segment->paragraph_end) {
			this->on_paragraph_end();
		}
		segment++;
	}
	this->on_document_end(document);
	this->batch_handler->on_document_end(document);
}

////////////////////////////////////////////////////////////////////////////////

IDocumentHandler* BatchHandler::create_document_handler()
{
	return new DocumentHandler(this);
}

void BatchHandler::on_batch_begin()
{
}

void BatchHandler::on_batch_end()
{
}

void BatchHandler::on_document_begin(Document& document)
{
}

void BatchHandler::on_document_end(Document& document)
{
}

void BatchHandler::on_match(Document& document, Match* match)
{
}

#endif

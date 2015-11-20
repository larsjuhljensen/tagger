#ifndef __REFLECT_PRINT_HANDLERS_HEADER__
#define __REFLECT_PRINT_HANDLERS_HEADER__

#include "base_handlers.h"
#include "file.h"
#include "mutex.h"

class PrintDocumentHandler : public StructuredDocumentHandler
{
	protected:
		int paragraph;
		int sentence;
	
	public:
		PrintDocumentHandler(IBatchHandler* batch_handler);
		
	public:
		void on_document_begin(Document& document);
		void on_paragraph_begin();
		void on_sentence_begin();
		void on_match(Document& document, Match* match);
};

class PrintBatchHandler : public BatchHandler, protected OutputFile, protected Mutex
{
	public:
		PrintBatchHandler(FILE* file);
		PrintBatchHandler(const char* filename);
		
	public:
		IDocumentHandler* create_document_handler();
		void on_document_begin(Document& document);
		void on_document_end(Document& document);
		virtual void print(Document* document, int paragraph, int sentence, Match* match);
};

////////////////////////////////////////////////////////////////////////////////

PrintDocumentHandler::PrintDocumentHandler(IBatchHandler* batch_handler)
 : StructuredDocumentHandler(batch_handler)
{
}

void PrintDocumentHandler::on_document_begin(Document& document)
{
	this->paragraph = 0;
}

void PrintDocumentHandler::on_paragraph_begin()
{
	this->paragraph++;
	this->sentence = 0;
}

void PrintDocumentHandler::on_sentence_begin()
{
	this->sentence++;
}

void PrintDocumentHandler::on_match(Document& document, Match* match)
{
	((PrintBatchHandler*) this->batch_handler)->print(&document, this->paragraph, this->sentence, match);
}

////////////////////////////////////////////////////////////////////////////////

PrintBatchHandler::PrintBatchHandler(FILE* file)
 : BatchHandler(), OutputFile(file), Mutex()
{
}

PrintBatchHandler::PrintBatchHandler(const char* filename)
 : BatchHandler(), OutputFile(filename), Mutex()
{
}

IDocumentHandler* PrintBatchHandler::create_document_handler()
{
	return new PrintDocumentHandler(this);
}

void PrintBatchHandler::on_document_begin(Document& document)
{
	this->lock();
}

void PrintBatchHandler::on_document_end(Document& document)
{
	this->unlock();
}

void PrintBatchHandler::print(Document* document, int paragraph, int sentence, Match* match)
{
	char replaced = document->text[match->stop+1];
	document->text[match->stop+1] = '\0';
	Entity* entity = match->entities;
	for (int i = 0; i < match->size; i++) {
		fprintf(this->file, "%d\t%d\t%d\t%d\t%d\t%s\t%d\t%u\n", document->key, paragraph, sentence, match->start, match->stop, (const char*)(document->text+match->start), entity->type, entity->id.serial);
		entity++;
	}
	document->text[match->stop+1] = replaced;
}

#endif

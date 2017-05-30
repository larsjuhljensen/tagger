#ifndef __REFLECT_SEGMENT_HANDLERS_HEADER__
#define __REFLECT_SEGMENT_HANDLERS_HEADER__

#include "base_handlers.h"
#include "file.h"
#include "mutex.h"

class SegmentBatchHandler : public BatchHandler, protected OutputFile, protected Mutex
{
	public:
		SegmentBatchHandler(FILE* file);
		SegmentBatchHandler(const char* filename);
		
	public:
		IDocumentHandler* create_document_handler();
		void on_document_begin(Document& document);
};

////////////////////////////////////////////////////////////////////////////////

SegmentBatchHandler::SegmentBatchHandler(FILE* file)
 : BatchHandler(), OutputFile(file), Mutex()
{
}

SegmentBatchHandler::SegmentBatchHandler(const char* filename)
 : BatchHandler(), OutputFile(filename), Mutex()
{
}

IDocumentHandler* SegmentBatchHandler::create_document_handler()
{
	return new StructuredDocumentHandler(this);
}

void SegmentBatchHandler::on_document_begin(Document& document)
{
	Segments segments = document.get_segments();
	this->lock();
	int paragraph = 0;
	int sentence = 0;
	for (Segments::iterator it = segments.begin(); it != segments.end(); it++) {
		if (it->paragraph_begin) {
			paragraph++;
			sentence = 0;
		}
		sentence++;
		fprintf(this->file, "%d\t%d\t%d\t%ld\t%ld\n", document.key, paragraph, sentence, it->begin-document.text, it->end-document.text);
	}
	this->unlock();
}

#endif

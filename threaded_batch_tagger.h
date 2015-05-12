#ifndef __REFLECT_THREADED_BATCH_TAGGER_HEADER__
#define __REFLECT_THREADED_BATCH_TAGGER_HEADER__

#include "batch_tagger.h"
#include "thread.h"

#include <vector>

class TaggerThread : public Thread
{
	private:
		IDocumentReader* document_reader;
		DocumentTagger* document_tagger;
		const GetMatchesParams* params;
		IDocumentHandler* document_handler;
		
	public:
		TaggerThread(IDocumentReader* document_reader, DocumentTagger* document_tagger, const GetMatchesParams* params, IDocumentHandler* document_handler);
		
	public:
		void run();
};

class ThreadedBatchTagger : public BatchTagger
{
	public:
		void process(int threads, IDocumentReader* document_reader, const GetMatchesParams& params, IBatchHandler* batch_handler);
};

////////////////////////////////////////////////////////////////////////////////

TaggerThread::TaggerThread(IDocumentReader* document_reader, DocumentTagger* document_tagger, const GetMatchesParams* params, IDocumentHandler* document_handler)
 : Thread()
{
	this->document_reader = document_reader;
	this->document_tagger = document_tagger;
	this->params = params;
	this->document_handler = document_handler;
}

void TaggerThread::run()
{
	this->document_handler->on_batch_begin();
	this->document_tagger->process(this->document_reader, *this->params, this->document_handler);
	this->document_handler->on_batch_end();
	pthread_exit(NULL);
}

////////////////////////////////////////////////////////////////////////////////

void ThreadedBatchTagger::process(int threads, IDocumentReader* document_reader, const GetMatchesParams& params, IBatchHandler* batch_handler)
{
	vector<TaggerThread*> thread_vector;
	for (int i = 0; i < threads; i++) {
		thread_vector.push_back(new TaggerThread(document_reader, this, &params, batch_handler->create_document_handler()));
	}
	batch_handler->on_batch_begin();
	for (int i = 0; i < threads; i++) {
		thread_vector[i]->start();
	}
	for (int i = 0; i < threads; i++) {
		thread_vector[i]->join();
		delete thread_vector[i];
	}
	batch_handler->on_batch_end();
}

#endif

#ifndef __REFLECT_DOCUMENT_HEADER__
#define __REFLECT_DOCUMENT_HEADER__

#include "file.h"
#include "mutex.h"

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <tr1/unordered_set>
#include <tr1/unordered_map>

using namespace std;
using namespace std::tr1;

struct Segment
{
	char* begin;
	char* end;
	bool paragraph_begin;
	bool paragraph_end;
};

typedef vector<Segment> Segments;

class Document
{
	public:
		int key;
		char* name;
		char* text;
		
	public:
		Document();
		Document(const Document& other);
		Document(int key, const char* text);
		virtual ~Document();
		
	public:
		virtual Segments get_segments();
};

class TsvDocument : public Document
{
	public:
		char* line;
		
	public:
		TsvDocument();
		virtual ~TsvDocument();
		
	public:
		virtual Segments get_segments();
};

class IDocumentReader
{
	public:
		virtual Document* read_document() = 0;
};

class DocumentReader
{
	public:
		virtual Document* read_document();
};

class DirectoryDocumentReader : public IDocumentReader, protected Mutex
{
	public:
		char* path;
		DIR* dir;
		bool close;
		int key;
		
	public:
		DirectoryDocumentReader(const char* path);
		~DirectoryDocumentReader();
		
	public:
		Document* read_document();
};

class TsvDocumentReader : public IDocumentReader, protected InputFile, protected Mutex
{
	protected:
		unordered_set<int> seen;
		
	public:
		TsvDocumentReader(FILE* file);
		TsvDocumentReader(const char* filename);
		~TsvDocumentReader();
	
	public:
		Document* read_document();
};

////////////////////////////////////////////////////////////////////////////////

Document::Document()
{
	this->key = 0;
	this->name = NULL;
	this->text = NULL;
}

Document::Document(const Document& other)
{
	this->key  = other.key;
	int length = strlen(other.text);
	this->text = new char[length+1];
	memcpy(this->text, other.text, length+1);
}

Document::Document(int key, const char* text)
{
	this->key = key;
	int length = strlen(text);
	this->text = new char[length+1];
	memcpy(this->text, text, length+1);
}

Document::~Document()
{
	delete this->name;
	delete this->text;
}

Segments Document::get_segments()
{	
	Segment segment;
	segment.begin = this->text;
	segment.end = this->text+strlen(this->text);
	segment.paragraph_begin = true;
	segment.paragraph_end = true;
	Segments segments;
	segments.push_back(segment);
	return segments;
}

////////////////////////////////////////////////////////////////////////////////

TsvDocument::TsvDocument()
 : Document()
{
}

TsvDocument::~TsvDocument()
{
	free(this->line);
	this->text = NULL;
}

Segments TsvDocument::get_segments()
{
	static const unsigned char sentence_type[256] = {
		0,   0,   0,   0,   0,   0,   0,   0,   0,   3,   3,   0,   0,   3,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		3,   1,   2,   0,   0,   0,   0,   2,   2,   2,   0,   0,   0,   0,   1,   0,
		4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   0,   0,   0,   0,   0,   1,
		0,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,
		4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   4,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
	};
	
	char* marker = text;
	Segments segments;
	
	while (*marker) {
		while (sentence_type[(unsigned char)*marker] == 3) {
			++marker;
		}
		
		Segment segment;
		segment.begin = marker;
		if (marker == text || *(marker-1) == '\t') {
			segment.paragraph_begin = true;
		}
		else {
			segment.paragraph_begin = false;
		}
	
		char* p1; // First punctuation.
		while (true) {
			while (*marker && *marker != '\t' && sentence_type[(unsigned char)*marker] != 1) {
				++marker;
			}
			if (!*marker || *marker == '\t') {
				segment.end = marker;
				segment.paragraph_end = true;
				segments.push_back(segment);
				break;
			}
			p1 = marker;
			do {
				++marker;
			} while (sentence_type[(unsigned char)*marker] == 1);
			if (!*marker || *marker == '\t') {
				segment.end = marker;
				segment.paragraph_end = true;
				segments.push_back(segment);
				break;
			}
			while (sentence_type[(unsigned char)*marker] == 2 && *marker != '(') {
				++marker;
			}		
			if (!*marker || *marker == '\t') {
				segment.end = marker;
				segment.paragraph_end = true;
				segments.push_back(segment);
				break;
			}
			if (sentence_type[(unsigned char)*marker] != 3) {
				continue;
			}
			segment.end = marker;
			do {
				++marker;
			} while (sentence_type[(unsigned char)*marker] == 3);
			if (!*marker || *marker == '\t') {
				segment.end = marker;
				segment.paragraph_end = true;
				segments.push_back(segment);
				break;
			}
			while (sentence_type[(unsigned char)*marker] == 2 && *marker != ')') {
				++marker;
			}
			if (!*marker || *marker == '\t') {
				segment.end = marker;
				segment.paragraph_end = true;
				segments.push_back(segment);
				break;
			}
			while (sentence_type[(unsigned char)*marker] == 3) {
				++marker;
			}
			if (!*marker || *marker == '\t') {
				segment.end = marker;
				segment.paragraph_end = true;
				segments.push_back(segment);
				break;
			}
			if (*marker == '-') {
				++marker;
			}
			if (!*marker || *marker == '\t') {
				segment.end = marker;
				segment.paragraph_end = true;
				segments.push_back(segment);
				break;
			}
			if (sentence_type[(unsigned char)*marker] == 4 && p1-segment.begin >= 4) {
				if (*p1 == '.') {
					if (*(p1-4) == ' ' && *(p1-2) == '.') {
						if (*(p1-1) == 'e') {
							if (*(p1-3) == 'i' || *(p1-3) == 'I') {
								continue;
							}
						}
						else if (*(p1-1) == 'g') {
							if (*(p1-3) == 'e' || *(p1-3) == 'E') {
								continue;
							}
						}
					}
					else if (p1-segment.begin >= 6 && *(p1-6) == ' ' && *(p1-5) == 'e' && *(p1-4) == 't' && *(p1-3) == ' ' && *(p1-2) == 'a' && *(p1-1) == 'l') {
						continue;
					}
				}
				segment.paragraph_end = false;
				segments.push_back(segment);
				break;
			}
		}
	}
	return(segments);	
}

////////////////////////////////////////////////////////////////////////////////

Document* DocumentReader::read_document()
{
	return new Document();
}

////////////////////////////////////////////////////////////////////////////////

DirectoryDocumentReader::DirectoryDocumentReader(const char* path)
 : Mutex()
{
	int length = strlen(path);
	this->path = new char[length+1];
	memcpy(this->path, path, length+1);
	this->dir = opendir(path);
	this->close = true;
	this->key = 0;
}

DirectoryDocumentReader::~DirectoryDocumentReader()
{
	if (this->close) {
		closedir(this->dir);
	}
}

Document* DirectoryDocumentReader::read_document()
{
	Document* document = new Document();
	bool valid = false;
	struct dirent* entry;
	int path_length = strlen(this->path);
	this->lock();
	while (!valid && (entry = readdir(this->dir)) != NULL) {
		int filename_length = strlen(entry->d_name);
		int fullname_length = path_length+filename_length+1;
		char* fullname = new char[fullname_length+1];
		memcpy(fullname, this->path, path_length);
		fullname[path_length] = '/';
		memcpy(fullname+path_length+1, entry->d_name, filename_length);
		fullname[fullname_length] = '\0';
		if (fullname_length >= 5 && fullname[fullname_length-4] == '.' && fullname[fullname_length-3] == 't' && fullname[fullname_length-2] == 'x' && fullname[fullname_length-1] == 't') {
			InputFile file = InputFile(fullname);
			document->text = file.get_all();
			if (document->text != NULL) {
				this->key++;
				document->key = this->key;
				document->name = new char[filename_length+1];
				memcpy(document->name, entry->d_name, filename_length);
				document->name[filename_length] = '\0';
				valid = true;
			}
		}
		delete fullname;
	}
	this->unlock();
	if (!valid) {
		delete document;
		document = NULL;
	}
	return document;
}

////////////////////////////////////////////////////////////////////////////////

TsvDocumentReader::TsvDocumentReader(FILE* file)
 : InputFile(file), Mutex()
{
}

TsvDocumentReader::TsvDocumentReader(const char* filename)
 : InputFile(filename), Mutex()
{
}

TsvDocumentReader::~TsvDocumentReader()
{
}

Document* TsvDocumentReader::read_document()
{
	TsvDocument* document = new TsvDocument();
	bool valid = false;
	while (!valid) {
		this->lock();
		char* line = this->get_line();
		document->line = line;
		int index = 0;
		if (line) {
			// Find first key and skip additional keys.
			while (line[index] != '\0' && line[index] != '\t' && line[index] != ':') {
				++index;
			}
			if (line[index] == ':') {
				char* key = line+index+1;
				while (line[index] != '\0'  && line[index] != '\t' && line[index] != '|') {
					++index;
				}
				char replaced = line[index];
				line[index] = '\0';
				document->key = atoi(key);
				line[index] = replaced;
				// Check if key was previously seen.
				if (document->key && this->seen.find(document->key) == this->seen.end()) {
					this->seen.insert(document->key);
					valid = true;
					if (this->seen.size() % 1000 == 0) {
						cerr << "# Read documents (in thousands): " << this->seen.size()/1000 << "\r";
					}
				}
			}
		}
		this->unlock();
		if (valid) {
			while (line[index] != '\0' && line[index] != '\t') {
				++index;
			}
			// Skip authors.
			if (line[index] != '\0') {
				do {
					++index;
				} while (line[index] != '\0' && line[index] != '\t');
			}
			// Skip journal.
			if (line[index] != '\0') {
				do {
					++index;
				} while (line[index] != '\0' && line[index] != '\t');
			}
			// Skip year.
			if (line[index] != '\0') {
				do {
					++index;
				} while (line[index] != '\0' && line[index] != '\t');
			}
			// Find text.
			if (line[index] != '\0' && line[index+1] != '\0') {
				document->text = line+index+1;
			}
			else {
				valid = false;
			}
		}
		else if (line) {
			free(line);
		}
		else {
			delete document;
			document = NULL;
			valid = true;
		}
	}
	return document;
}

#endif

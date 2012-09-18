#ifndef __REFLECT_FILE_HEADER__
#define __REFLECT_FILE_HEADER__

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

using namespace std;

class InputFile
{
	public:
		FILE* file;
		bool close;
		
	public:
		InputFile(FILE* file);
		InputFile(const char* filename);
		~InputFile();
		
	public:
		char* get_all();
		vector<char*> get_fields();
		char* get_line();
};

class OutputFile
{
	public:
		FILE* file;
		bool close;
		
	public:
		OutputFile(FILE* file);
		OutputFile(const char* filename);
		~OutputFile();
};

////////////////////////////////////////////////////////////////////////////////

InputFile::InputFile(FILE* file)
{
	this->file = file;
	this->close = false;
}

InputFile::InputFile(const char* filename)
{
	if (filename) {
		this->file = fopen(filename, "r");
		this->close = true;
	}
	else {
		this->file = NULL;
		this->close = false;
	}
}

InputFile::~InputFile()
{
	if (this->close && this->file != NULL) {
		fclose(this->file);
	}
}

char* InputFile::get_all()
{
	if (this->file == NULL || feof(this->file)) {
		return NULL;
	}
	int length = 0;
	int size = 4096;
	char* buffer = (char*)malloc(size*sizeof(char));
	while (true) {
		char c = fgetc(this->file);
		if (c == EOF) {
			break;
		}
		buffer[length] = c;
		++length;
		if (length == size) {
			size *= 2;
			buffer = (char*)realloc(buffer, size*sizeof(char));
		}
	}
	if (length) {
		buffer[length] = '\0';
		buffer = (char*)realloc(buffer, length*sizeof(char));
		return buffer;
	}
	else {
		free(buffer);
		return NULL;
	}
}

vector<char*> InputFile::get_fields()
{
	vector<char*> fields;
	char* line = get_line();
	if (line) {
		char* begin = line;
		while (true) {
			char* end = begin;
			while (*end && *end != '\t') {
				++end;
			}
			int length = end-begin;
			char* field = new char[length+1];
			memcpy(field, begin, length);
			field[length] = '\0';
			fields.push_back(field);
			if (*end) {
				begin = end+1;
			}
			else {
				break;
			}
		}
		free(line);
	}
	return fields;
}

char* InputFile::get_line()
{
	if (this->file == NULL || feof(this->file)) {
		return NULL;
	}
	int length = 0;
	int size = 4096;
	char* buffer = (char*)malloc(size*sizeof(char));
	while (true) {
		char c = fgetc(this->file);
		if (c == EOF || c == '\n') {
			break;
		}
		buffer[length] = c;
		++length;
		if (length == size) {
			size *= 2;
			buffer = (char*)realloc(buffer, size*sizeof(char));
		}
	}
	if (length) {
		buffer[length] = '\0';
		buffer = (char*)realloc(buffer, (length+1)*sizeof(char));
		return buffer;
	}
	else {
		free(buffer);
		return NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////

OutputFile::OutputFile(FILE* file)
{
	this->file = file;
	this->close = false;
}

OutputFile::OutputFile(const char* filename)
{
	if (filename) {
		this->file = fopen(filename, "w");
		this->close = true;
	}
	else {
		this->file = NULL;
		this->close = false;
	}
}

OutputFile::~OutputFile()
{
	if (this->file != NULL) {
		fflush(this->file);
		if (this->close) {
			fclose(this->file);
		}
	}
}

#endif

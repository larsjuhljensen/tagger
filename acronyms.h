#ifndef __REFLECT_ACRONYMS_HEADER__
#define __REFLECT_ACRONYMS_HEADER__

#include <cstring>
#include <vector>
#include <tr1/unordered_map>
#include <boost/regex.hpp>

#include "hash.h"

using namespace std;
using namespace std::tr1;
using namespace boost;

class Acronyms : public unordered_map<const char*, const char*, OrthographHash, OrthographHash::EqualString>
{
	private:
		boost::regex re[2];
		
	public:
		Acronyms();
		~Acronyms();
		
	public:
		void add(const char* document);
};

////////////////////////////////////////////////////////////////////////////////

Acronyms::Acronyms()
{
//	pattern += "|(([a-z])-?([a-z])-?([a-z])) \\(\\11[a-z-]*[ -]\\12[a-z-]*[ -]\\13[a-z-]*\\)";
//	pattern += "|(([a-z])-?([a-z])-?([a-z])-?([a-z])) \\(\\15[a-z-]*[ -]\\16[a-z-]*[ -]\\17[a-z-]*[ -]\\18[a-z-]*\\)";
	this->re[0] = regex("(([a-z])[a-z-]*[ -]([a-z])[a-z-]*[ -]([a-z])[a-z-]*) \\((\\2-?\\3-?\\4)\\)", boost::regex::icase);
	this->re[1] = regex("(([a-z])[a-z-]*[ -]([a-z])[a-z-]*[ -]([a-z])[a-z-]*[ -]([a-z])[a-z-]*) \\((\\2-?\\3-?\\4-?\\5)\\)", boost::regex::icase);
}

Acronyms::~Acronyms()
{
	for (Acronyms::iterator it = this->begin(); it != this->end(); ++it) {
		delete it->first;
		delete it->second;
	}
}

void Acronyms::add(const char* document)
{
	int length = strlen(document);
	boost::regex_iterator<const char*> end;
	for (int i = 0; i < 2; ++i) {
		try {
			boost::regex_iterator<const char*> it(document, document+length, this->re[i]);
			while (it != end) {
				const char* acronym_start = NULL;
				const char* acronym_stop = NULL;
				const char* name_start = NULL;
				const char* name_stop = NULL;
				if (i == 0) {
					acronym_start =  (*it)[5].first;
					acronym_stop =  (*it)[5].second;
					name_start = (*it)[1].first;
					name_stop = (*it)[1].second;
				}
				else {
					acronym_start =  (*it)[6].first;
					acronym_stop =  (*it)[6].second;
					name_start = (*it)[1].first;
					name_stop = (*it)[1].second;
				}
				int acronym_length = acronym_stop-acronym_start;
				char* acronym = new char[acronym_length+1];
				memcpy(acronym, acronym_start, acronym_length);
				acronym[acronym_length] = '\0';
				Acronyms::iterator search = this->find(acronym);
				if (search == this->end()) {
					int name_length = name_stop-name_start;
					char* name = new char[name_length+1];
					memcpy(name, name_start, name_length);
					name[name_length] = '\0';
					this->insert(pair<const char*, const char*>(acronym, name));
				}
				else {
					delete acronym;
				}
				++it;
			}
		}
		catch (...) {
			break;
		}
	}
}

#endif

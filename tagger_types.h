#ifndef __REFLECT_TAGGER_TYPES_HEADER__
#define __REFLECT_TAGGER_TYPES_HEADER__

#include <list>
#include <vector>
#include <tr1/unordered_map>

#include "hash.h"
#include "tightvector.h"

using namespace std;
using namespace std::tr1;

typedef unsigned int SERIAL;

union Identifier
{
	const char* string;
	SERIAL serial;
};

struct Entity
{
	int type;
	Identifier id;
};

class Entities: public vector<Entity>
{
	public:
		bool serials_only;
};

class Match
{
	public:
		int start;
		int stop;
		int size;
		Entity* entities;
	
	public:
		Match()
		{
			this->start = -1;
			this->stop  = -1;
			this->size  = -1;
			this->entities = NULL;
		}
		
		~Match()
		{
			free(this->entities);
		}
		
};

class Matches: public vector<Match*> {
	public:
		bool serials_only;
};

class Acronyms;
class Tokens;

typedef tightvector<unsigned short, Entity> ENTITY_VECTOR;
typedef unordered_map<SERIAL, Entity> ENTITY_MAP;

typedef unordered_map<SERIAL, int> ORGANISMS;
typedef unordered_map<const char*, ENTITY_VECTOR, OrthographHash, OrthographHash::EqualString> DICTIONARY;
typedef unordered_map<const char*, bool,      StringHash, StringHash::EqualString>  NAME_BOOL;
typedef unordered_map<const char*, NAME_BOOL, StringHash, StringHash::EqualString>  DOC_NAME_BOOL;

#endif

#ifndef __REFLECT_MATCH_HANDLERS_HEADER__
#define __REFLECT_MATCH_HANDLERS_HEADER__

#include "document.h"
#include "file.h"
#include "tagger_types.h"

#include <vector>
#include <tr1/unordered_map>

using namespace std;
using namespace std::tr1;

typedef vector<SERIAL> SERIALS;
typedef unordered_set<SERIAL> ENTITY_SET;

class EntityTypeMap : public unordered_map<SERIAL, int>
{
	public:
		EntityTypeMap(const char* entities_filename);
};

class IMatchHandler
{
	public:
		virtual void process(Matches& matches) = 0;
};

class MatchHandler : public IMatchHandler
{
	public:
		virtual void process(Matches& matches);
};

class DisambiguationMatchHandler : public IMatchHandler
{
	public:
		virtual void process(Matches& matches);
};

class GroupMatchHandler : public DisambiguationMatchHandler
{
	private:
		int type;
		EntityTypeMap* group_type_map;
		unordered_map<SERIAL, SERIALS> entity_group_map;
	
	public:
		GroupMatchHandler(int type, const char* groups_filename);
		GroupMatchHandler(EntityTypeMap* group_type_map, const char* groups_filename);
	
	public:
		void load_groups(const char* groups_filename);
		virtual void process(Matches& matches);
};

////////////////////////////////////////////////////////////////////////////////

EntityTypeMap::EntityTypeMap(const char* entities_filename)
{
	InputFile entities_file(entities_filename);
	while (true) {
		vector<char *> fields = entities_file.get_fields();
		int size = fields.size();
		if (size == 0) {
			break;
		}
		if (size >= 2) {
			SERIAL serial = atoi(fields[0]);
			int type = atoi(fields[1]);
			(*this)[serial] = type;
		}
		for (vector<char*>::iterator it = fields.begin(); it != fields.end(); it++) {
			delete *it;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void MatchHandler::process(Matches& matches)
{
}

////////////////////////////////////////////////////////////////////////////////

void DisambiguationMatchHandler::process(Matches& matches)
{
	ENTITY_SET entity_set;
	for (Matches::iterator it = matches.begin(); it != matches.end(); it++) {
		if ((*it)->size == 1) {			
			entity_set.insert((*it)->entities[0].id.serial);
		}
	}
	for (Matches::iterator it = matches.begin(); it != matches.end(); it++) {
		int count = 0;
		for (int i = 0; i < (*it)->size; i++) {
			if (entity_set.find((*it)->entities[i].id.serial) != entity_set.end()) {
				count++;
			}
		}
		if (count == 0) {
			for (int i = 0; i < (*it)->size; i++) {
				entity_set.insert((*it)->entities[i].id.serial);
			}
		}
		else if (count < (*it)->size) {
			Entity* entities = new Entity[count];
			int index = 0;
			for (int i = 0; i < (*it)->size; i++) {
				if (entity_set.find((*it)->entities[i].id.serial) != entity_set.end()) {
					entities[index] = (*it)->entities[i];
					index++;
				}
			}
			delete (*it)->entities;
			(*it)->entities = entities;
			(*it)->size = count;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

GroupMatchHandler::GroupMatchHandler(int type, const char* groups_filename)
{
	this->type = type;
	this->group_type_map = NULL;
	this->load_groups(groups_filename);
}

GroupMatchHandler::GroupMatchHandler(EntityTypeMap* group_type_map, const char* groups_filename)
{
	this->type = 0;
	this->group_type_map = group_type_map;
	this->load_groups(groups_filename);
}

void GroupMatchHandler::load_groups(const char* groups_filename)
{
	InputFile groups_file(groups_filename);
	while (true) {
		vector<char *> fields = groups_file.get_fields();
		int size = fields.size();
		if (size == 0) {
			break;
		}
		if (size >= 2) {
			SERIAL serial1 = atoi(fields[0]);
			SERIAL serial2 = atoi(fields[1]);
			unordered_map<SERIAL, SERIALS>::iterator it = this->entity_group_map.find(serial1);
			if (it == this->entity_group_map.end()) {
				this->entity_group_map[serial1] = SERIALS();
				it = this->entity_group_map.find(serial1);
			}
			it->second.push_back(serial2);
		}
		for (vector<char*>::iterator it = fields.begin(); it != fields.end(); it++) {
			delete *it;
		}
	}
}

void GroupMatchHandler::process(Matches& matches)
{
	DisambiguationMatchHandler::process(matches);
	for (Matches::iterator match_it = matches.begin(); match_it != matches.end(); match_it++) {
		unordered_map<SERIAL, int> group_count;
		for (int i = 0; i < (*match_it)->size; i++) {
			unordered_map<SERIAL, SERIALS>::iterator entity_group_it = this->entity_group_map.find((*match_it)->entities[i].id.serial);
			if (entity_group_it != this->entity_group_map.end()) {
				for (SERIALS::iterator serials_it = entity_group_it->second.begin(); serials_it != entity_group_it->second.end(); serials_it++) {
					unordered_map<SERIAL,int>::iterator count_it = group_count.find(*serials_it);
					if (count_it == group_count.end()) {
						group_count[*serials_it] = 1;
					}
					else {
						count_it->second++;
					}
				}
			}
		}
		SERIALS common_groups;
		for (unordered_map<SERIAL, int>::iterator count_it = group_count.begin(); count_it != group_count.end(); count_it++) {
			if (count_it->second == (*match_it)->size) {
				common_groups.push_back(count_it->first);
			}
		}
		if (common_groups.size()) {
			size_t old_size = (*match_it)->size;
			size_t new_size = old_size+common_groups.size();
			Entity* new_entities = new Entity[new_size];
			memcpy(new_entities, (*match_it)->entities, old_size*sizeof(Entity));
			for (unsigned int i = 0; i < common_groups.size(); i++) {
				new_entities[old_size+i].id.serial = common_groups[i];
				if (this->group_type_map != NULL) {
					new_entities[old_size+i].type = (*this->group_type_map)[common_groups[i]];
				}
				else {
					new_entities[old_size+i].type = this->type;
				}
			}
			delete (*match_it)->entities;
			(*match_it)->entities = new_entities;
			(*match_it)->size = new_size;
		}
		else if ((*match_it)->size > 1) {
			delete (*match_it)->entities;
			(*match_it)->entities = NULL;
			(*match_it)->size = 0;
		}
	}
}

#endif

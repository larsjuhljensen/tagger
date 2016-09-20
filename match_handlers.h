#ifndef __REFLECT_MATCH_HANDLERS_HEADER__
#define __REFLECT_MATCH_HANDLERS_HEADER__

#include "document.h"
#include "file.h"
#include "tagger_types.h"

#include <climits>
#include <vector>
#include <unordered_map>

using namespace std;

typedef vector<SERIAL> SERIALS;
typedef unordered_set<SERIAL> ENTITY_SET;
typedef unordered_map<int, int> TYPE_COUNT_MAP;

class EntityTypeMap : public unordered_map<SERIAL, int>
{
	public:
		EntityTypeMap(const char* entities_filename);
};

class IMatchHandler
{
	public:
		virtual ~IMatchHandler() {};
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
		EntityTypeMap* entity_type_map;
		unordered_map<SERIAL, SERIALS> entity_group_map;
	
	public:
		GroupMatchHandler(int type, const char* groups_filename);
		GroupMatchHandler(EntityTypeMap* entity_type_map, const char* groups_filename);
	
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
	this->entity_type_map = NULL;
	this->load_groups(groups_filename);
}

GroupMatchHandler::GroupMatchHandler(EntityTypeMap* entity_type_map, const char* groups_filename)
{
	this->type = 0;
	this->entity_type_map = entity_type_map;
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
	TYPE_COUNT_MAP type_count_map;
	for (Matches::iterator match_it = matches.begin(); match_it != matches.end(); match_it++) {
		for (int i = 0; i < (*match_it)->size; i++) {
			int type = (*match_it)->entities[i].type;
			TYPE_COUNT_MAP::iterator type_count_it = type_count_map.find(type);
			if (type_count_it == type_count_map.end()) {
				type_count_map[type] = 1;
			}
			else {
				type_count_it->second++;
			}
		}
	}
	for (Matches::iterator match_it = matches.begin(); match_it != matches.end(); match_it++) {
		unordered_set<int> excluded;
		bool ambiguous = true;
		while (ambiguous) {
			unordered_map<SERIAL, int> group_count;
			int size = 0;
			for (int i = 0; i < (*match_it)->size; i++) {
				if (excluded.find((*match_it)->entities[i].type) == excluded.end()) {
					unordered_map<SERIAL, SERIALS>::iterator entity_group_it = this->entity_group_map.find((*match_it)->entities[i].id.serial);
					if (entity_group_it != this->entity_group_map.end()) {
						for (SERIALS::iterator serials_it = entity_group_it->second.begin(); serials_it != entity_group_it->second.end(); serials_it++) {
							unordered_map<SERIAL,int>::iterator group_count_it = group_count.find(*serials_it);
							if (group_count_it == group_count.end()) {
								group_count[*serials_it] = 1;
							}
							else {
								group_count_it->second++;
							}
						}
					}
					size++;
				}
			}
			SERIALS common_groups;
			for (unordered_map<SERIAL, int>::iterator group_count_it = group_count.begin(); group_count_it != group_count.end(); group_count_it++) {
				if (group_count_it->second == size) {
					common_groups.push_back(group_count_it->first);
				}
			}
			if (size == 0) {
				delete (*match_it)->entities;
				(*match_it)->entities = NULL;
				(*match_it)->size = 0;
				ambiguous = false;
			}
			else if (size == 1 or common_groups.size()) {
				int n = common_groups.size();
				size += n;
				Entity* entities = new Entity[size];
				int i = 0;
				for (int j = 0; j < (*match_it)->size; j++) {
					if (excluded.find((*match_it)->entities[j].type) == excluded.end()) {
						entities[i] = (*match_it)->entities[j];
						i++;
					}
				}
				for (int j = 0; j < n; j++) {
					entities[i].id.serial = common_groups[j];
					if (this->entity_type_map != NULL) {
						entities[i].type = (*this->entity_type_map)[common_groups[j]];
					}
					else {
						entities[i].type = this->type;
					}
					i++;
				}
				delete (*match_it)->entities;
				(*match_it)->entities = entities;
				(*match_it)->size = size;
				ambiguous = false;
			}
			else {
				int min_count = INT_MAX;
				for (int i = 0; i < (*match_it)->size; i++) {
					int type = (*match_it)->entities[i].type;
					if (excluded.find(type) == excluded.end()) {
						int count = type_count_map[type];
						if (count < min_count) {
							min_count = count;
						}
					}
				}
				for (int i = 0; i < (*match_it)->size; i++) {
					int type = (*match_it)->entities[i].type;
					if (type_count_map[type] == min_count) {
						excluded.insert(type);
					}
				}
			}
		}
	}
}

#endif

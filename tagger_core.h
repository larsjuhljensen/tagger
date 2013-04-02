#ifndef __REFLECT_TAGGER_CORE_HEADER__
#define __REFLECT_TAGGER_CORE_HEADER__

#include <boost/regex.hpp>
#include <iostream>

#include "acronyms.h"
#include "file.h"
#include "tokens.h"

using namespace std;
using namespace boost;

GetMatchesParams::GetMatchesParams()
{
	this->auto_detect         = true;
	this->allow_overlap       = false;
	this->find_acronyms       = true;
	this->protect_tags        = false;
	this->tokenize_characters = false;
	this->max_tokens          = 15;
}

void GetMatchesParams::add_entity_type(int entity_type)
{
	this->entity_types.push_back(entity_type);
}

Tagger::Tagger(bool serials_only)
{
	// -------------------------------------------------------------------------
	// CAUTION !!!
	// Do not alter this expression without storing an unaltered copy first.
	// We did a lot of debugging and tweeking to port the original Perl
	// expression so please do not alter this expression at any point without
	// safely storing it first.
	// -------------------------------------------------------------------------
	string pattern;
	pattern += "\\A&|\\A(.{0,2}|.[ \t\r\n]+.|[^A-Za-z]*([ \t\r\n]|\\Z)|[ACDEFGHIKLMNPQRSTVWY]-?[0-9]{1,4}|[CJnsXx][ -]*[0-9.-]+|[0-9]{1,4}[A-Za-z]|[0-9.-]+[ -]*[cmnp]?[AgLlMmSsVXx][0-9-]*)\\Z";
	pattern += "|\\A([DGNSTdgnst]o|[Aa][nst]?|[Aa]nd|[Aa]re|[Bb][ey]|[Bb]ut|[Cc]an|[Dd]id|[Ff]or|[Hh]a[ds]|[Hh]ave]|[Ii][fnst]|[Ii]ts|[Oo][fnr]|[Tt]he|[Tt]his|[Ww]as|[Ww]ere)[ \t\r\n]";
	pattern += "|[ \t\r\n]([dgs]o|are|but|can|did|et|ha[ds]|have|i[fst]|its|this|was|were)[ \t\r\n]";
	pattern += "|[ \t\r\n]([dgnst]o|a[nst]?|and|are|b[ey]|but|can|did|et|for|ha[ds]|have|i[fnst]|its|o[fnr]|the|this|was|were)\\Z";
	this->re_stop = regex(pattern.c_str());
	this->re_html = regex("<!DOCTYPE[ \t\r\n][^>]*?>|<!--.*?-->|<(head|pre|script)[> \t\r\n].*?</\\1[ \t\r\n]*>|</?[a-z0-9_]+(([ \t\r\n]+[a-z0-9_:-]+([ \t\r\n]*=[ \t\r\n]*(\".*?\"|'.*?'|[^\"'> \t\r\n]+))?)+[ \t\r\n]*|[ \t\r\n]*)/?>", boost::regex::icase);
	this->re_reflect = regex("\\A(<span [^>]*?startReflectPopupTimer.*?>(.*?)</span>|<div [^>]*?reflect_.*?>.*?</div>\n?|</body>|</html>)", boost::regex::icase);
	this->serials_only = serials_only;
}

Tagger::~Tagger()
{
}

void Tagger::add_name(const char* name, int type, const char* identifier)
{
	size_t n = 0;
	DICTIONARY::iterator search = this->names.find(name);
	if (search == this->names.end()) {
		n = strlen(name)+1;
		char* name_copy = new char[n];
		memcpy(name_copy, name, n);
		n = strlen(identifier)+1;
		Identifier identifier_copy;
		identifier_copy.string = new char[n];
		memcpy((char*)identifier_copy.string, identifier, n);
		ENTITY_VECTOR entities = ENTITY_VECTOR();
		Entity entity;
		entity.type = type;
		entity.id.string = identifier_copy.string;
		entities.push_back(entity);
		this->names[name_copy] = entities;
	}
	else {
		bool already_loaded = false;
		ENTITY_VECTOR& entities = search->second;
		for (ENTITY_VECTOR::iterator it = entities.begin(); it != entities.end(); ++it) {
			if (it->type == type && strcmp(it->id.string, identifier) == 0) {
				already_loaded = true;
				break;
			}
		}
		if (!already_loaded) {
			n = strlen(identifier)+1;
			Identifier identifier_copy;
			identifier_copy.string = new char[n];
			memcpy((char*)identifier_copy.string, identifier, n);
			Entity entity;
			entity.type = type;
			entity.id.string = identifier_copy.string;
			entities.push_back(entity);
		}
	}
}

void Tagger::add_name(const char* name, int type, int serial)
{
}

void Tagger::allow_block_name(const char* name, const char* document_id, bool block)
{
	size_t n = 0;
	char* name_copy = NULL;
	char* document_id_copy = NULL;
	
	if (document_id == NULL) {
		NAME_BOOL::iterator b = global.find(name);
		if (b == global.end()) {
			n = strlen(name)+1;
			name_copy = new char[n];
			memcpy(name_copy, name, n);
			global[name_copy] = block;
		}
		else {
			(*b).second = block;
		}
	}
	else {
		DOC_NAME_BOOL::iterator a = this->local.find(document_id);
		if (a == this->local.end()) {
			n = strlen(name)+1;
			name_copy = new char[n];
			memcpy(name_copy, name, n);
			n = strlen(document_id)+1;
			document_id_copy = new char[n];
			memcpy(document_id_copy, document_id, n);
			NAME_BOOL dl;
			dl[name_copy] = block;
			this->local[document_id_copy] = dl;
		}
		else {
			NAME_BOOL::iterator b = (*a).second.find(name);
			if (b == (*a).second.end()) {
				n = strlen(name)+1;
				name_copy = new char[n];
				memcpy(name_copy, name, n);
				(*a).second[name_copy] = block;
			}
			else {
				(*b).second = block;
			}
		}
	}
}

bool Tagger::check_name(const char* name, int type, const char* identifier)
{
	DICTIONARY::iterator search = this->names.find(name);
	if (search != this->names.end()) {
		ENTITY_VECTOR& entities = search->second;
		for (ENTITY_VECTOR::iterator it = entities.begin(); it != entities.end(); ++it) {
			if (it->type == type && strcmp(it->id.string, identifier) == 0) {
				return true;
			}
		}
	}
	return false;
}

bool Tagger::is_blocked(const char* document_id, const char* name)
{
	if (document_id != NULL) {
		DOC_NAME_BOOL::iterator local_hit = this->local.find(document_id);
		if (local_hit != this->local.end()) {
			NAME_BOOL::iterator doc_hit = local_hit->second.find(name);
			if (doc_hit != local_hit->second.end()) {
				return doc_hit->second;
			}
		}
	}
	NAME_BOOL::iterator global_hit = this->global.find(name);
	if (global_hit != this->global.end()) {
		return global_hit->second;
	}
	match_results<const char*> stop_match;
	try {
		return regex_search(name, name+strlen(name), stop_match, this->re_stop);
	}
	catch (...) {
		return false;
	}
}

Entities Tagger::resolve_name(const char* name) {
	DICTIONARY::iterator search = this->names.find(name);
	Entities entities;
	entities.serials_only = this->serials_only;
	if (search != this->names.end()) {
		for (ENTITY_VECTOR::iterator it = search->second.begin(); it != search->second.end(); ++it) {
			Entity& entity = *it;
			entities.push_back(entity);
		}
	}
	return entities;
}

void Tagger::load_global(const char* global_filename)
{
	if (global_filename == NULL) {
		return;
	}
	InputFile global_file(global_filename);
	while (true) {
		vector<char *> fields = global_file.get_fields();
		int size = fields.size();
		if (size == 0) {
			break;
		}
		if (size >= 2) {
			const char* name = fields[0];
			bool blocked = (fields[1][0] == '1' || fields[1][0] == 't' || fields[1][0] == 'y');
			this->global[name] = blocked;
			DICTIONARY::iterator search = this->names.find(name);
			if (search == this->names.end()) {
				Identifier id;
				if (this->serials_only) {
					id.serial = 0;
				}
				else {
					id.string = NULL;
				}
				Entity entity;
				entity.type = 0;
				entity.id = id;
				ENTITY_VECTOR entities = ENTITY_VECTOR();
				entities.push_back(entity);
				this->names[name] = entities;
			}
			delete fields[1];
		}
	}
}

void Tagger::load_local(const char* local_filename)
{
	if (local_filename == NULL) {
		return;
	}
	InputFile local_file(local_filename);
	while (true) {
		vector<char *> fields = local_file.get_fields();
		int size = fields.size();
		if (size == 0) {
			break;
		}
		if (size >= 3) {
			const char* name = fields[0];
			const char* document_id = fields[1];
			bool blocked = (fields[2][0] == '1' || fields[2][0] == 't' || fields[2][0] == 'y');
			DOC_NAME_BOOL::iterator it = this->local.find(document_id);
			if (it == this->local.end()) {
				NAME_BOOL dl;
				dl[name] = blocked;
				this->local[document_id] = dl;
				delete fields[2];
			}
			else {
				this->local[document_id][name] = blocked;
				delete fields[1];
				delete fields[2];
			}
		}
	}
}

void Tagger::load_names(const char* entities_filename, const char* names_filename)
{
	if (entities_filename == NULL or names_filename == NULL) {
		return;
	}
	ENTITY_MAP entities;
	InputFile entities_file(entities_filename);
	while (true) {
		vector<char *> fields = entities_file.get_fields();
		int size = fields.size();
		if (size == 0) {
			break;
		}
		if (size >= 3) {
			SERIAL serial;
			int type;
			serial = atoi(fields[0]);
			type = atoi(fields[1]);
			if (this->serials_only) {
				if (type == -3) {
					this->organisms[serial] = atoi(fields[2]);
				}
				Entity entity;
				entity.type = type;
				entity.id.serial = serial;
				entities[serial] = entity;
			}
			else {
				Entity entity;
				entity.type = type;
				entity.id.string = fields[2];
				entities[serial] = entity;
				fields[2] = NULL;
			}
		}
		for (vector<char*>::iterator it = fields.begin(); it != fields.end(); it++) {
			delete *it;
		}
	}
	InputFile names_file(names_filename);
	while (true) {
		vector<char *> fields = names_file.get_fields();
		int size = fields.size();
		if (size == 0) {
			break;
		}
		if (fields.size() >= 2) {
			SERIAL serial = atoi(fields[0]);
			char* name = fields[1];
			delete fields[0];
			ENTITY_MAP::iterator it1 = entities.find(serial);
			DICTIONARY::iterator it2 = this->names.find(name);
			if (it1 == entities.end()) {
				delete fields[1];
			}
			else if (it2 == this->names.end()) {
				ENTITY_VECTOR entities = ENTITY_VECTOR();
				entities.push_back(it1->second);
				this->names[name] = entities;
			}
			else {
				bool entity_already_added = false;
				for (int i = 0; i < it2->second.size(); i++) {
					if (it2->second[i].id.serial == it1->second.id.serial) {
						entity_already_added = true;
						break;
					}
				}
				if (not entity_already_added) {
					it2->second.push_back(it1->second);
				}
				delete fields[1];
			}
		}
	}
}

void Tagger::load_names(int type, const char* names_filename)
{
	assert(this->serials_only);
	if (names_filename == NULL) {
		return;
	}
	InputFile names_file(names_filename);
	while (true) {
		vector<char *> fields = names_file.get_fields();
		int size = fields.size();
		if (size == 0) {
			break;
		}
		if (size >= 2) {
			SERIAL serial = atoi(fields[0]);
			const char* name = fields[1];
			delete fields[0];
			Entity entity;
			entity.type = type;
			entity.id.serial = serial;
			DICTIONARY::iterator it2 = this->names.find(name);
			if (it2 == this->names.end()) {
				ENTITY_VECTOR entities = ENTITY_VECTOR();
				entities.push_back(entity);
				this->names[name] = entities;
			}
			else {
				bool entity_already_added = false;
				for (int i = 0; i < it2->second.size(); i++) {
					if (it2->second[i].id.serial == entity.id.serial) {
						entity_already_added = true;
						break;
					}
				}
				if (not entity_already_added) {
					it2->second.push_back(entity);
				}
				delete fields[1];
			}
		}
	}
}

const char trim_left_right[256] = {
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   1,   1,   0,   0,   0,   0,   1,  41,   1,   0,   0,   1,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   0,   0,   0,   1,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  93,   0,   1,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 125,   0,   1,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

const char trim_right_left[256] = {
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   1,   1,   0,   0,   0,   0,   1,   1,  40,   0,   0,   1,   0,   1,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   1,   0,   0,   0,   1,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0,  91,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   1,   0, 123,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
	  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

void Tagger::trim_boundaries(const char* document, int& start, int& stop)
{
	bool changed = true;
	char left;
	char right;
	while (changed && start < stop) {
		changed = false;
		left = document[start];
		right = trim_left_right[(unsigned char)left];
		while (right && start < stop) {
			if (right == 1) {
				changed = true;
				left = document[++start];
				right = trim_left_right[(unsigned char)left];
			}
			else {
				int count = 1;
				int i = start+1;
				while (i <= stop) {
					char c = document[i];
					if (c == left) {
						++count;
					}
					else if (c == right) {
						--count;
						if (count == 0) {
							break;
						}
					}
					++i;
				}
				if (i >= stop) {
					changed = true;
					left = document[++start];
					right = trim_left_right[(unsigned char)left];
					if (count == 0) {
						--stop;
					}
				}
				else {
					break;
				}
			}
		}
		right = document[stop];
		left = trim_right_left[(unsigned char)right];
		while (left && start < stop) {
			if (left == 1) {
				changed = true;
				right = document[--stop];
				left = trim_right_left[(unsigned char)right];
			}
			else {
				int count = 1;
				int i = stop-1;
				while (i >= start) {
					char c = document[i];
					if (c == right) {
						++count;
					}
					else if (c == left) {
						--count;
						if (count == 0) {
							break;
						}
					}
					--i;
				}
				if (i <= start) {
					changed = true;
					right = document[--stop];
					left = trim_right_left[(unsigned char)right];
					if (count == 0) {
						++start;
					}
				}
				else {
					break;
				}
			}
		}
	}
}

void Tagger::find_matches(Matches& matches, Acronyms& acronyms, Tokens& tokens, char* document, const char* document_id, const GetMatchesParams& params, const unordered_set<int>& entity_types_map)
{
	for (Tokens::iterator it = tokens.begin(); it != tokens.end(); ++it) {
		int tsize = (*it).size();
		for (int i = 0; i < tsize; ++i) {
			int j = i + params.max_tokens;
			if (j > tsize) {
				j = tsize;
			}
			while (j > i) {
				--j;
				int start = (*it)[i].start;
				int stop = (*it)[j].stop;
				int end = stop+1;
				char* name = document+start;
				char replaced = document[end];
				document[end] = '\0';
				bool match_found = false;
				Acronyms::iterator search1 = acronyms.find(document+start);
				DICTIONARY::iterator search2;
				if (search1 != acronyms.end()) {
					name = (char *)search1->second;
				}
				search2 = this->names.find(name);
				if (search2 != this->names.end()) {
					this->trim_boundaries(document, start, stop);
					name = document+start;
					document[end] = replaced;
					end = stop+1;
					replaced = document[end];
					document[end] = '\0';
					ENTITY_VECTOR& entities = search2->second;
					if (entities.size() == 1) {
						if (this->serials_only) {
							if (entities[0].id.serial == 0) {
								match_found = true;
							}
						}
						else {
							if (entities[0].id.string == NULL) {
								match_found = true;
							}
						}
					}
					if (!match_found && !this->is_blocked(document_id, name)) {
						ENTITY_VECTOR  entity_subset;
						for (ENTITY_VECTOR::iterator et = entities.begin(); et != entities.end(); ++et) {
							Entity& entity = *et;
							if ((entity.type >= 0 && entity_types_map.find(0) != entity_types_map.end()) || entity_types_map.find(entity.type) != entity_types_map.end()) {
								entity_subset.push_back(entity);
							}
						}
						if (entity_subset.size() > 0) {
							Match* m = new Match();
							matches.push_back(m);
							m->start = start;
							m->stop  = stop;
							m->size  = entity_subset.size();
							m->entities = new Entity[entity_subset.size()];
							int x = 0;
							for (ENTITY_VECTOR::iterator it = entity_subset.begin(); it != entity_subset.end(); ++it) {
								Entity& entity = *it;
								m->entities[x].type = entity.type;
								if (this->serials_only) {
									m->entities[x].id.serial = entity.id.serial;
								}
								else {
									m->entities[x].id.string = entity.id.string;
								}
								x++;
							}
							match_found = true;
							if (!params.allow_overlap) {
								i = j;
							}
						}
					}
				}
				document[end] = replaced;
				if (match_found && !params.allow_overlap)
					break;
			}
		}
	}
}

Matches Tagger::get_matches(char* document, char* document_id, const GetMatchesParams& params)
{
	int str_len = strlen(document);
	const char* from = document;
	const char* last = document + str_len;
	Acronyms acronyms;
	Tokens tokens;
	Matches matches;
	matches.serials_only = this->serials_only;
	// -----------------------------------------------------------
	// Protect HTML tags by finding them via a regular expression.
	// Tokenize the text before each match and the text after the
	// last tag. Produces a list of tokens between tags.
	// -----------------------------------------------------------
	if (params.protect_tags) {
		try {
			while (from < last) {
				const char* search = from;
				while (*search != '<' && *search != '\0') {
					++search;
				}
				match_results<const char*> tag;
				if (!regex_search(search, last, tag, this->re_html)) {
					break;
				}
				if (tag[0].first > from) {
					char replaced = *(tag[0].first);
					*(char*)(tag[0].first) = '\0';
					if (params.find_acronyms) {
						acronyms.add(from);
					}
					tokens.add(document, from-document, params);
					*(char*)(tag[0].first) = replaced;
				}
				match_results<const char*> reflect;
				if (regex_search(tag[0].first, last, reflect, this->re_reflect)) {
					if (reflect[2].matched) { // Matched <span> tag.
						Match* remove  = new Match();
						remove->start  = reflect[0].first - document;
						remove->stop   = reflect[2].first - document - 1;
						remove->size = -1;
						remove->entities = NULL;
						matches.push_back(remove);
						remove  = new Match();
						remove->start = reflect[2].second - document;
						remove->stop  = reflect[0].second - document - 1;
						remove->size = -1;
						remove->entities = NULL;
						matches.push_back(remove);
						char replaced = *((char*)reflect[2].second);
						*((char*)reflect[2].second) = '\0';
						if (params.find_acronyms) {
							acronyms.add(from);
						}
						tokens.add(document, reflect[2].first-document, params);
						*((char*)reflect[2].second) = replaced;
					}
					else {
						Match* remove = new Match();
						remove->start = reflect[0].first  - document;
						remove->stop  = reflect[0].second - document - 1;
						remove->size  = -1;
						remove->entities = NULL;
						matches.push_back(remove);
					}
					from = reflect[0].second;
				}
				else {
					from = tag[0].second;
				}
			}
		}
		catch (...) {
		}
	}
	if (from < last) {
		if (params.find_acronyms) {
			acronyms.add(from);
		}
		tokens.add(document, from-document, params);
	}
	// ----------------------------------
	// Auto-detect organisms (type = -3).
	// ----------------------------------
	unordered_set<int> entity_types_map;
	if (params.auto_detect) {
		entity_types_map.insert(-3);
		Matches entity_type_matches;
		this->find_matches(entity_type_matches, acronyms, tokens, document, document_id, params, entity_types_map);
		entity_types_map.clear();
		for (Matches::iterator it = entity_type_matches.begin(); it < entity_type_matches.end(); ++it) {
			Entity* p = (**it).entities;
			Entity* q = p+(**it).size;
			while (p < q) {
				if (this->serials_only) {
					entity_types_map.insert(this->organisms[(*p).id.serial]);
				}
				else {
					entity_types_map.insert(atoi((*p).id.string));
				}
				++p;
			}
			delete *it;
		}
	}
	for (vector<int>::const_iterator n = params.entity_types.begin(); n != params.entity_types.end(); ++n) {
		entity_types_map.insert(*n);
	}
	this->find_matches(matches, acronyms, tokens, document, document_id, params, entity_types_map);
	return matches;
}

void quit()
{
	_exit(0);
}

#endif

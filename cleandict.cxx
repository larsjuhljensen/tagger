#include "file.h"
#include "tagger.h"

#include <iostream>
#include <fstream>

typedef unordered_set<const char*, OrthographHash, OrthographHash::EqualString> ORTHO_NAME_SET;
typedef unordered_map<int, ORTHO_NAME_SET> REMOVE_NAME_MAP;

class CleaningTagger : public Tagger {
	
	private:
		REMOVE_NAME_MAP removed;
	
	public:
		CleaningTagger() : Tagger(true)
		{}
		
	private:
		void remove_name(int serial, const char* name)
		{
			REMOVE_NAME_MAP::iterator itserial = this->removed.find(serial);
			if (itserial == this->removed.end()) {
				ORTHO_NAME_SET name_set;
				name_set.insert(name);
				this->removed[serial] = name_set;
			}
			else {
				itserial->second.insert(name);
			}
		}
		
		bool is_removed(int serial, const char* name)
		{
			REMOVE_NAME_MAP::iterator itserial = this->removed.find(serial);
			if (itserial != this->removed.end()) {
				return itserial->second.find(name) != itserial->second.end();
			}
			else {
				return false;
			}
		}
		
	public:
		void cleanup_dictionary(const char* filename_filtered)
		{
			unordered_set<int> filter_types;
			filter_types.insert(-2);
			filter_types.insert(-22);
			filter_types.insert(-25);
			filter_types.insert(-26);
			filter_types.insert(-27);
			
			unordered_set<int> protected_types;
			protected_types.insert(-2);
			protected_types.insert(-3);
			protected_types.insert(-21);
			protected_types.insert(-22);
			protected_types.insert(-23); // Because _filter.tsv has not been made yet.
			protected_types.insert(-25);
			protected_types.insert(-26);
			protected_types.insert(-27);
			
			ofstream ffilter(filename_filtered);
			
			for (DICTIONARY::iterator i = this->names.begin(); i != this->names.end(); i++) {
				const char* name = i->first;
				ENTITY_VECTOR& entities = i->second;
				// Count the number of entities per type associated with this name.
				unordered_map<int, int> type_count;
				vector<Entity*> conflicts;
				for (ENTITY_VECTOR::iterator j = entities.begin(); j != entities.end(); j++) {
					int type = (*j).type;
					if (type >= 0 || filter_types.find(type) != filter_types.end()) {
						conflicts.push_back(j);
					}
					unordered_map<int, int>::iterator k = type_count.find(type);
					if (k == type_count.end()) {
						type_count[type] = 1;
					}
					else {
						k->second++;
					}
				}
				
				// Find the maximal count of anyone type to this name.
				int max_prot_count = -1;
				for (unordered_map<int, int>::iterator i = type_count.begin(); i != type_count.end(); i++) {
					if (i->first >= 0 && i->second > max_prot_count) {
						max_prot_count = i->second;
					}
				}
				
				// Loop over entities again, through out large families,
				// global or local, and too short and too long names.
				for (ENTITY_VECTOR::iterator j = entities.begin(); j != entities.end(); j++) {
					
					int type   = (*j).type;
					int serial = (*j).id.serial;
					int count = type_count.find(type)->second;
					
					if (protected_types.find(type) == protected_types.end() && conflicts.size() > 0) {
						for (vector<Entity*>::iterator k = conflicts.begin(); k != conflicts.end(); k++) {
							if (type != (*k)->type && (type < 0 || (*k)->type < 0)) {
								ffilter << "CONFLICT\t" << name << "\t" << type << "\t" << serial << "\t(" << count << ")";
								ffilter << "\tMASKED-BY\t" << (*k)->type << "\t" << (*k)->id.serial << endl;
								this->remove_name(serial, name);
							}
						}
					}
					
					// Throw away large protein families.
					if (max_prot_count >= 1000) {
						ffilter << "FAMILY-GLOBAL\t" << name << "\t" << type << "\t" << serial << "\t(" << count << ")" << endl;
						this->remove_name(serial, name);
					}
					else if (count > 20) {
						ffilter << "FAMILY-LOCAL\t" << name << "\t" << type << "\t" << serial << "\t(" << count << ")" << endl;
						this->remove_name(serial, name);
					}
					
					// Throw away very short names.
					if (strlen(name) < 2) {
						ffilter << "SHORT-NAME\t" << name << "\t" << type << "\t" << serial << "\t(" << count << ")" << endl;
						this->remove_name(serial, name);
					}
					
					// WIKIPEDIA AND ONTOLOGIES.
					if (type == -11 && strlen(name) <= 3) {
						ffilter << "SHORT-WIKI\t" << name << "\t" << type << "\t" << serial << "\t(" << count << ")" << endl;
						this->remove_name(serial, name);
					}
					
				}
			}
		}
		
		void print_clean_names(const char* names_filename, const char* cleaned_filename)
		{
			InputFile names_file(names_filename);
			OutputFile cleaned_file(cleaned_filename);
			while (true) {
				vector<char *> fields = names_file.get_fields();
				int size = fields.size();
				if (size == 0) {
					break;
				}
				if (size >= 2) {
					int serial = atoi(fields[0]);
					const char* name = fields[1];
					if (!this->is_removed(serial, name)) {
						fprintf(cleaned_file.file, "%d\t%s\n", serial, name);
					}
				}
				for (vector<char*>::iterator it = fields.begin(); it != fields.end(); it++) {
					delete *it;
				}
			}
		}
};

int main (int argc, char *argv[])
{
	const char* filename_entities = argv[1];
	const char* filename_names    = argv[2];
	const char* filename_cleaned  = argv[3];
	const char* filename_removed  = argv[4];
	
	CleaningTagger tagger;
	
	tagger.load_names(filename_entities, filename_names);
	tagger.cleanup_dictionary(filename_removed);
	tagger.print_clean_names(filename_names, filename_cleaned);
	
	_exit(0);
}

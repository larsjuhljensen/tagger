#ifndef __REFLECT_SCORE_HANDLERS_HEADER__
#define __REFLECT_SCORE_HANDLERS_HEADER__

#include "base_handlers.h"
#include "file.h"
#include "mutex.h"

#include <math.h>

typedef double SCORE;
typedef unsigned long SERIAL_PAIR;
typedef unordered_set<SERIAL> ENTITY_SET;
typedef unordered_set<SERIAL_PAIR> PAIR_SET;
typedef unordered_map<SERIAL, unordered_map<int, double> > ENTITY_TYPE_SCORE_MAP;
typedef unordered_map<int, unordered_map<int, double> > TYPE_TYPE_SCORE_MAP;
typedef unordered_set<int> TYPE_SET;
typedef unordered_map<int, TYPE_SET > TYPE_TYPE_SET;

class PairScoreMap : public unordered_map<SERIAL_PAIR, SCORE>
{
	public:
		PairScoreMap& operator+=(PairScoreMap const & other);
};

class ScoreDocumentHandler : public StructuredDocumentHandler
{
	private:
		PairScoreMap pair_score_map;
		ENTITY_SET document_entity_set;
		ENTITY_SET paragraph_entity_set;
		ENTITY_SET sentence_entity_set;
		PAIR_SET document_pair_set;
		PAIR_SET paragraph_pair_set;
		PAIR_SET sentence_pair_set;
		
	public:
		ScoreDocumentHandler(IBatchHandler* batch_handler);
		
	private:
		void commit_pairs(PAIR_SET& pair_set, SCORE weight);
		void create_pairs(PAIR_SET& pair_set, ENTITY_SET& entity_set, Match* match);
		
	public:
		void on_batch_end();
		void on_document_begin(Document& document);
		void on_document_end(Document& document);
		void on_paragraph_begin();
		void on_sentence_begin();
		void on_match(Document& document, Match* match);
};

class ScoreBatchHandler : public BatchHandler, protected OutputFile, public Mutex
{
	public:
		EntityTypeMap* entity_type_map;
		PairScoreMap pair_score_map;
		SCORE document_weight;
		SCORE paragraph_weight;
		SCORE sentence_weight;
		SCORE factor_exponent;
		
	public:
		ScoreBatchHandler(FILE* file, EntityTypeMap* entity_type_map, SCORE document_weight, SCORE paragraph_weight, SCORE sentence_weight, SCORE factor_exponent);
		ScoreBatchHandler(const char* filename, EntityTypeMap* entity_type_map, SCORE document_weight, SCORE paragraph_weight, SCORE sentence_weight, SCORE factor_exponent);
		
	public:
		IDocumentHandler* create_document_handler();
		void on_batch_end();
		virtual bool validate_pair(int type1, int type2);
};

class SelectiveScoreBatchHandler : public ScoreBatchHandler
{
	public:
		TYPE_TYPE_SET valid_type_pairs;
		
	public:
		SelectiveScoreBatchHandler(FILE* file, EntityTypeMap* entity_type_map, SCORE document_weight, SCORE paragraph_weight, SCORE sentence_weight, SCORE factor_exponent, const char* valid_type_pairs_filename);
		SelectiveScoreBatchHandler(const char* filename, EntityTypeMap* entity_type_map, SCORE document_weight, SCORE paragraph_weight, SCORE sentence_weight, SCORE factor_exponent, const char* valid_type_pairs_filename);
		
	public:
		virtual bool validate_pair(int type1, int type2);
};

////////////////////////////////////////////////////////////////////////////////

PairScoreMap& PairScoreMap::operator+=(PairScoreMap const & other)
{
	for (PairScoreMap::const_iterator it1 = other.begin(); it1 != other.end(); it1++) {
		PairScoreMap::iterator it2 = this->find(it1->first);
		if (it2 != this->end()) {
			it2->second += it1->second;
		}
		else {
			this->insert(*it1);
		}
	}
	return *this;
}

////////////////////////////////////////////////////////////////////////////////

ScoreDocumentHandler::ScoreDocumentHandler(IBatchHandler* batch_handler)
 : StructuredDocumentHandler(batch_handler)
{
}

void ScoreDocumentHandler::commit_pairs(PAIR_SET& pair_set, SCORE weight) {
	if (weight > 0) {
		for (PAIR_SET::iterator it1 = pair_set.begin(); it1 != pair_set.end(); it1++) {
			PairScoreMap::iterator it2 = this->pair_score_map.find(*it1);
			if (it2 == this->pair_score_map.end()) {
				this->pair_score_map[*it1] = weight;
			}
			else {
				it2->second += weight;
			}
		}
	}
}

void ScoreDocumentHandler::create_pairs(PAIR_SET& pair_set, ENTITY_SET& entity_set, Match* match) {
	ScoreBatchHandler* score_batch_handler = (ScoreBatchHandler*) this->batch_handler;
	ENTITY_SET match_set;
	for (int i = 0; i < match->size; i++) {
		Entity* entity = match->entities+i;
		SERIAL serial = entity->id.serial;
		if (entity_set.find(serial) == entity_set.end()) {
			match_set.insert(serial);
		}
	}
	for (int i = 0; i < match->size; i++) {
		Entity* entity = match->entities+i;
		SERIAL serial = entity->id.serial;
		if (entity_set.find(serial) == entity_set.end()) {
			int type1 = (*score_batch_handler->entity_type_map)[serial];
			for (ENTITY_SET::iterator it = entity_set.begin(); it != entity_set.end(); it++) {
				if (match_set.find(*it) == match_set.end()) {
					int type2 = (*score_batch_handler->entity_type_map)[*it];
					if (score_batch_handler->validate_pair(type1, type2)) {
						SERIAL_PAIR serial_pair;
						if (serial < *it) {
							serial_pair = (((SERIAL_PAIR)serial)<<32)|((SERIAL_PAIR)*it);
						}
						else {
							serial_pair = (((SERIAL_PAIR)*it)<<32)|((SERIAL_PAIR)serial);
						}
						pair_set.insert(serial_pair);
					}
				}
			}
		}
	}
	for (int i = 0; i < match->size; i++) {
		entity_set.insert((match->entities+i)->id.serial);
	}
}

void ScoreDocumentHandler::on_batch_end()
{
	ScoreBatchHandler* score_batch_handler = (ScoreBatchHandler*) this->batch_handler;
	score_batch_handler->lock();
	score_batch_handler->pair_score_map += this->pair_score_map;
	score_batch_handler->unlock();
	this->pair_score_map.clear();
}

void ScoreDocumentHandler::on_document_begin(Document& document)
{
	this->document_entity_set.clear();
	this->document_pair_set.clear();
	this->paragraph_pair_set.clear();
	this->sentence_pair_set.clear();
}

void ScoreDocumentHandler::on_document_end(Document& document)
{
	ScoreBatchHandler* score_batch_handler = (ScoreBatchHandler*) this->batch_handler;
	this->commit_pairs(this->document_pair_set, score_batch_handler->document_weight);
	this->commit_pairs(this->paragraph_pair_set, score_batch_handler->paragraph_weight);
	this->commit_pairs(this->sentence_pair_set, score_batch_handler->sentence_weight);
	if (this->pair_score_map.size() >= 10000) {
		score_batch_handler->lock();
		score_batch_handler->pair_score_map += this->pair_score_map;
		score_batch_handler->unlock();
		this->pair_score_map.clear();
	}
	else if (this->pair_score_map.size() >= 1000 and score_batch_handler->trylock()) {
		score_batch_handler->pair_score_map += this->pair_score_map;
		score_batch_handler->unlock();
		this->pair_score_map.clear();
	}
}

void ScoreDocumentHandler::on_paragraph_begin()
{
	this->paragraph_entity_set.clear();
}

void ScoreDocumentHandler::on_sentence_begin()
{
	this->sentence_entity_set.clear();
}

void ScoreDocumentHandler::on_match(Document& document, Match* match)
{
	create_pairs(this->document_pair_set, this->document_entity_set, match);
	create_pairs(this->paragraph_pair_set, this->paragraph_entity_set, match);
	create_pairs(this->sentence_pair_set, this->sentence_entity_set, match);
}

////////////////////////////////////////////////////////////////////////////////

ScoreBatchHandler::ScoreBatchHandler(FILE* file, EntityTypeMap* entity_type_map, SCORE document_weight, SCORE paragraph_weight, SCORE sentence_weight, SCORE factor_exponent)
 : BatchHandler(), OutputFile(file), Mutex()
{
	this->entity_type_map = entity_type_map;
	this->document_weight = document_weight;
	this->paragraph_weight = paragraph_weight;
	this->sentence_weight = sentence_weight;
	this->factor_exponent = factor_exponent;
}

ScoreBatchHandler::ScoreBatchHandler(const char* filename, EntityTypeMap* entity_type_map, SCORE document_weight, SCORE paragraph_weight, SCORE sentence_weight, SCORE factor_exponent)
 : BatchHandler(), OutputFile(filename), Mutex()
{
	this->entity_type_map = entity_type_map;
	this->document_weight = document_weight;
	this->paragraph_weight = paragraph_weight;
	this->sentence_weight = sentence_weight;
	this->factor_exponent = factor_exponent;
}

IDocumentHandler* ScoreBatchHandler::create_document_handler()
{
	return new ScoreDocumentHandler(this);
}

void ScoreBatchHandler::on_batch_end()
{
	ENTITY_TYPE_SCORE_MAP serial_type_score;
	for (PairScoreMap::iterator it = this->pair_score_map.begin(); it != this->pair_score_map.end(); it++) {
		SERIAL_PAIR serial_pair = it->first;
		SERIAL serial1 = (SERIAL)((serial_pair & 0xFFFFFFFF00000000) >> 32);
		SERIAL serial2 = (SERIAL)( serial_pair & 0x00000000FFFFFFFF);
		double score = this->pair_score_map[serial_pair];
		int type1 = (*this->entity_type_map)[serial1];
		int type2 = (*this->entity_type_map)[serial2];
		ENTITY_TYPE_SCORE_MAP::iterator it1 = serial_type_score.find(serial1);
		ENTITY_TYPE_SCORE_MAP::iterator it2 = serial_type_score.find(serial2);
		if (it1 == serial_type_score.end()) {
			serial_type_score[serial1] = unordered_map<int, double>();
			serial_type_score[serial1][type2] = score;
		}
		else {
			unordered_map<int, double>::iterator it3 = it1->second.find(type2);
			if (it3 == it1->second.end()) {
				it1->second[type2] = score;
			}
			else {
				it3->second += score;
			}
		}
		if (it2 == serial_type_score.end()) {
			serial_type_score[serial2] = unordered_map<int, double>();
			serial_type_score[serial2][type1] = score;
		}
		else {
			unordered_map<int, double>::iterator it3 = it2->second.find(type1);
			if (it3 == it2->second.end()) {
				it2->second[type1] = score;
			}
			else {
				it3->second += score;
			}
		}
	}
	TYPE_TYPE_SCORE_MAP type_type_score;
	for (ENTITY_TYPE_SCORE_MAP::iterator it1 = serial_type_score.begin(); it1 != serial_type_score.end(); it1++) {
		int type1 = (*this->entity_type_map)[it1->first];
		TYPE_TYPE_SCORE_MAP::iterator it2 = type_type_score.find(type1);
		if (it2 == type_type_score.end()) {
			type_type_score[type1] = unordered_map<int, double>();
			it2 = type_type_score.find(type1);
		}
		for (unordered_map<int, double>::iterator it3 = it1->second.begin(); it3 != it1->second.end(); it3++) {
			int type2 = it3->first;
			double score = it3->second;
			if (type1 == type2) {
				score *= 0.5;
			}
			unordered_map<int, double>::iterator it4 = it2->second.find(type2);
			if (it4 == it2->second.end()) {
				it2->second[type2] = score;
			}
			else {
				it4->second += score;
			}
		}
	}
	for (PairScoreMap::iterator it = this->pair_score_map.begin(); it != this->pair_score_map.end(); it++) {
		SERIAL_PAIR serial_pair = it->first;
		SERIAL serial2 = (SERIAL)( serial_pair & 0x00000000FFFFFFFF);
		SERIAL serial1 = (SERIAL)((serial_pair & 0xFFFFFFFF00000000) >> 32);
		int type1    = (*this->entity_type_map)[serial1];
		int type2    = (*this->entity_type_map)[serial2];
		double s_ij  = this->pair_score_map[serial_pair];
		double s_i   = serial_type_score[serial1][type2];
		double s_j   = serial_type_score[serial2][type1];
		double s     = type_type_score[type1][type2];
		double ratio = (s_ij * s)/(s_i * s_j);
		double score = pow(s_ij, this->factor_exponent) * pow(ratio, 1 - this->factor_exponent);
		fprintf(this->file, "%u\t%u\t%f\t%f\t%f\t%f\t%f\t%f\n", serial1, serial2, score, ratio, s, s_i, s_j, s_ij);
	}
}

bool ScoreBatchHandler::validate_pair(int type1, int type2)
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////

SelectiveScoreBatchHandler::SelectiveScoreBatchHandler(FILE* file, EntityTypeMap* entity_type_map, SCORE document_weight, SCORE paragraph_weight, SCORE sentence_weight, SCORE factor_exponent, const char* valid_type_pairs_filename)
 : ScoreBatchHandler(file, entity_type_map, document_weight, paragraph_weight, sentence_weight, factor_exponent)
{
	InputFile valid_type_pairs_file(valid_type_pairs_filename);
	while (true) {
		vector<char *> fields = valid_type_pairs_file.get_fields();
		int size = fields.size();
		if (size == 0) {
			break;
		}
		if (size >= 2) {
			int type1 = atoi(fields[0]);
			int type2 = atoi(fields[1]);
			if (this->valid_type_pairs.find(type1) == this->valid_type_pairs.end()) {
				this->valid_type_pairs[type1] = TYPE_SET();
			}
			this->valid_type_pairs[type1].insert(type2);
			if (this->valid_type_pairs.find(type2) == this->valid_type_pairs.end()) {
				this->valid_type_pairs[type2] = TYPE_SET();
			}
			this->valid_type_pairs[type2].insert(type1);
		}
		for (vector<char*>::iterator it = fields.begin(); it != fields.end(); it++) {
			delete *it;
		}
	}
}

SelectiveScoreBatchHandler::SelectiveScoreBatchHandler(const char* filename, EntityTypeMap* entity_type_map, SCORE document_weight, SCORE paragraph_weight, SCORE sentence_weight, SCORE factor_exponent, const char* valid_type_pairs_filename)
 : ScoreBatchHandler(filename, entity_type_map, document_weight, paragraph_weight, sentence_weight, factor_exponent)
{
	InputFile valid_type_pairs_file(valid_type_pairs_filename);
	while (true) {
		vector<char *> fields = valid_type_pairs_file.get_fields();
		int size = fields.size();
		if (size == 0) {
			break;
		}
		if (size >= 2) {
			int type1 = atoi(fields[0]);
			int type2 = atoi(fields[1]);
			if (this->valid_type_pairs.find(type1) == this->valid_type_pairs.end()) {
				this->valid_type_pairs[type1] = TYPE_SET();
			}
			this->valid_type_pairs[type1].insert(type2);
			if (this->valid_type_pairs.find(type2) == this->valid_type_pairs.end()) {
				this->valid_type_pairs[type2] = TYPE_SET();
			}
			this->valid_type_pairs[type2].insert(type1);
		}
		for (vector<char*>::iterator it = fields.begin(); it != fields.end(); it++) {
			delete *it;
		}
	}
}

bool SelectiveScoreBatchHandler::validate_pair(int type1, int type2)
{
	TYPE_TYPE_SET::iterator it1 = this->valid_type_pairs.find(type1);
	if (it1 == this->valid_type_pairs.end()) {
		return false;
	}
	else {
		return it1->second.find(type2) != it1->second.end();
	}
}

#endif

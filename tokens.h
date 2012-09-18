#ifndef __REFLECT_TOKENS_HEADER__
#define __REFLECT_TOKENS_HEADER__

class Token {
	
	public:
		int start;
		int stop;
		int length;
		
	public:
		Token();
		Token(const Token& other);
		Token(int start, int stop);
};

class Tokens : public list< vector<Token> >
{
	public:
		void add(char* document, int offset, const GetMatchesParams& params);
};

////////////////////////////////////////////////////////////////////////////////

Token::Token()
{
	this->start = 0;
	this->stop = 0;
	this->length = 0;
}

Token::Token(const Token& other)
{
	this->start = other.start;
	this->stop = other.stop;
	this->length = other.length;
}

Token::Token(int start, int stop) {
	this->start = start;
	this->stop = stop;
	this->length = stop-start+1;
}

////////////////////////////////////////////////////////////////////////////////

/* Lookup table with tokenization characters (whitespaces and symbols) */
const unsigned char tokenize_type[256] = {
	  0,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   1,   1,   2,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  2,   2,   1,   1,   1,   1,   1,   1,   2,   2,   1,   2,   2,   3,   2,   2,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   1,   1,   1,   2,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   1,   2,   1,   2,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   2,   2,   2,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,
	  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1
};

void Tokens::add(char* document, int offset, const GetMatchesParams& params)
{
	vector<Token> tokens;
	if (params.tokenize_characters) {
		for (int i = offset; document[i]; ++i) {
			tokens.push_back(Token(i, i));
		}
	}
	else {
		int start = -1;
		int hyphen = -1;
		int i = offset;
		while (true) {
			unsigned char type = tokenize_type[(unsigned char)document[i]];
			if (type == 1) {
				if (start == -1) {
					start = i;
				}
				else if (hyphen != -1) {
					Token cur(start, hyphen-1);
					tokens.push_back(cur);
					start = i;
					hyphen = -1;
				}
			}
			else if (type == 3) {
				if (start != -1 && hyphen == -1) {
					hyphen = i;
				}
			}
			else {
				if (start != -1) {
					Token cur(start, i-1);
					tokens.push_back(cur);
					start = -1;
					hyphen = -1;
				}
				if (type == 0) {
					break;
				}
			}
			++i;
		}
	}
	this->push_back(tokens);
}

#endif

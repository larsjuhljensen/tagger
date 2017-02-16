#ifndef __REFLECT_HASH_HEADER__
#define __REFLECT_HASH_HEADER__

#include <cstring>
#include <unordered_map>

using namespace std;

namespace std
{
	template<> size_t hash<const char*>::operator()(const char* val) const noexcept;
}

/* Simple string hash that does not allow for orthographic variation */
class StringHash : public hash<const char*>
{
	public:
		size_t operator()(const char* val) const;
		
	public:
		struct EqualString {
			bool operator()(const char* s1, const char* s2) const;
		};
};

/* Special string hash and string compare that allow for trimming of matches */
class TrimHash : public hash<const char*>
{	
	public:
		size_t operator()(const char* s) const;
		
	public:	
		struct EqualString {
			bool operator()(const char* p1, const char* p2) const;
		};
};

/* Special string hash and string compare that allow for orthographic variation */
class OrthographHash : public hash<const char*>
{	
	public:
		size_t operator()(const char* s) const;
		
	public:	
		struct EqualString {
			bool operator()(const char* p1, const char* p2) const;
		};
};

////////////////////////////////////////////////////////////////////////////////

/* Lookup table for special hash that skips special characters */
const unsigned char trim_hash[256] = {
	  0,   1,   2,   3,   4,   5,   6,   7,   8,   0,   0,  11,  12,   0,  14,  15,
	 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
	  0,   0,   0,  35,  36,  37,  38,   0,   0,   0,  42,  43,   0,   0,   0,  47,
	 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,   0,   0,  60,  61,  62,   0,
	 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
	 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,   0,  92,   0,  94,   0,
	 96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124,   0, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

/* Lookup table for special hash that upper-cases letters and skips special characters */
const unsigned char orthograph_hash[256] = {
	  0,   1,   2,   3,   4,   5,   6,   7,   8,   0,   0,  11,  12,   0,  14,  15,
	 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
	  0,   0,   0,  35,  36,  37,  38,   0,   0,   0,  42,  43,   0,   0,   0,  47,
	 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,   0,   0,  60,  61,  62,   0,
	 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
	 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,   0,  92,   0,  94,   0,
	 96,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
	 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,   0, 124,   0, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

const unsigned char compare_upper[256] = {
	  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
	 16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
	 32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
	 48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
	 64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
	 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
	 96,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,  75,  76,  77,  78,  79,
	 80,  81,  82,  83,  84,  85,  86,  87,  88,  89,  90, 123, 124, 125, 126, 127,
	128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
	144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159,
	160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175,
	176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191,
	192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207,
	208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223,
	224, 225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
	240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254, 255
};

const unsigned char compare_type[256] = {
	  0,   3,   3,   3,   3,   3,   3,   3,   3,   1,   1,   3,   3,   1,   3,   3,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
	  1,   2,   2,   3,   3,   3,   3,   2,   2,   2,   3,   3,   2,   1,   2,   3,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   2,   2,   3,   3,   3,   2,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   2,   3,   2,   3,   1,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   2,   3,   2,   3,   3,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,
	  3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3,   3
};

#define _NULL_        0
#define _SPACE_       1
#define _PUNCTUATION_ 2
#define _NORMAL_      3

////////////////////////////////////////////////////////////////////////////////

inline size_t StringHash::operator()(const char* s) const
{
	size_t h = *s;
	while (*(++s)) 
		h = ((h<<5)+h)+*s;
	return h;
}

inline bool StringHash::EqualString::operator()(const char* s1, const char* s2) const
{
	return (s1 == s2) || (s1 && s2 && strcmp(s1, s2) == 0);
}

////////////////////////////////////////////////////////////////////////////////

inline size_t TrimHash::operator()(const char* s) const {
	register unsigned long h = 0;
	register const char* p = s;
	while (*p) {
		register unsigned long c = trim_hash[(unsigned char)*(p++)];
		if (c) h = ((h<<5)+h)+c;
	}
	return h;
}

inline bool TrimHash::EqualString::operator()(const char* p1, const char* p2) const {
	
	if (p1 == p2) {
		return true;
	}
	else if (p1 == NULL || p2 == NULL){
		return false;
	}
	
	while (compare_type[(unsigned char)*p1] == _PUNCTUATION_) {
		++p1;
	}
	
	while (compare_type[(unsigned char)*p2] == _PUNCTUATION_) {
		++p2;
	}
	
	while (*p1 == *p2) {
		if (*p1 != _NULL_) {
			++p1;
			++p2;
		}
		else{
			return true;
		}
	}
	
	while (compare_type[(unsigned char)*p1] == _PUNCTUATION_) {
		++p1;
	}
	
	while (compare_type[(unsigned char)*p2] == _PUNCTUATION_) {
		++p2;
	}
	
	return *p1 == _NULL_ && *p2 == _NULL_;
	
}

////////////////////////////////////////////////////////////////////////////////

/* Special string hash that allows for orthographic variation */
inline size_t OrthographHash::operator()(const char* s) const {
	register unsigned long h = 0;
	register const char* p = s;
	while (*p) {
		register unsigned long c = orthograph_hash[(unsigned char)*(p++)];
		if (c) h = ((h<<5)+h)+c;
	}
	return h;
}


/*
 Special string comparison function which ignores special chars sqeezed
 inbetween streaches of non-special chars (normal chars.)
 
 This function also uses three states just like the hashing function.
 The main idea is to cleverly skip through streaches of special chars that
 should be ignored. The state is 0 to begin with. In the initial state
 all chars are compared (special or not). Differences while in state 0
 results in returning a non-zero value indicating the strings were different.
 From state 0 we can move to state 1 or 2. We move to state 1 if both chars
 currently traversed in both strings are normal. If just one char is special
 we move to state 2 regardless of the current state. In state 1 and 2 we may
 just anvance one char in one of the strings because that string is starting
 a special streach. We then traverse that string only until we reach the end
 or until we find a normal char.
*/

/* Special string compare that allows for orthographic variation */
inline bool OrthographHash::EqualString::operator()(const char* p1, const char* p2) const {
	
	#ifdef REFLECT_PROFILE
	++profile_orthograph_equal_count;
	#endif
	
	if (p1 == p2) {
		return true;
	}
	else if (p1 == NULL || p2 == NULL){
		return false;
	}
	
	while (compare_type[(unsigned char)*p1] == _PUNCTUATION_) {
		++p1;
	}
	
	while (compare_type[(unsigned char)*p2] == _PUNCTUATION_) {
		++p2;
	}
	
	while (compare_type[(unsigned char)*p1] == _SPACE_ || compare_type[(unsigned char)*p2] == _SPACE_) {
		if (*p1 == *p2) {
			++p1;
			++p2;
		}
		else {
			return false;
		}
	}
	
	bool e2 = true;
	bool e3 = true;
	while (1) {
		unsigned char c1 = compare_upper[(unsigned char)*p1];
		unsigned char c2 = compare_upper[(unsigned char)*p2];
		unsigned char type1 = compare_type[c1];
		if (c1 == c2) {
			if (type1 == _NULL_) {
				return e2;
			}
			else if (type1 != _PUNCTUATION_ && !e3) {
				return false;
			}
			else if (type1 == _NORMAL_) {
				e2 = true;
			}
			++p1;
			++p2;
			continue;
		}
		unsigned char type2 = compare_type[c2];
		if (type1 == type2) {
			if (type1 == _PUNCTUATION_) {
				e3 = false;
			}
			else if (type1 == _SPACE_ && e3) {
				e2 = false;
			}
			else {
				return false;
			}
			++p1;
			++p2;
		}
		else if (type1 > type2) {
			if (type1 == _NORMAL_) {
				if (type2 == _SPACE_ && e3) {
					e2 = false;
					++p2;
				}
				else {
					return false;
				}
			}
			else if (type1 == _PUNCTUATION_) {
				if (type2 == _NULL_) {
					e3 = false;
					++p1;
				}
				else if (type2 == _SPACE_ && e3) {
					e2 = false;
					++p2;
				}
				else {
					return false;
				}
			}
			else {
				return false;
			}
		}
		else {
			if (type2 == _NORMAL_) {
				if (type1 == _SPACE_ && e3) {
					e2 = false;
					++p1;
				}
				else {
					return false;
				}
			}
			else if (type2 == _PUNCTUATION_) {
				if (type1 == _NULL_) {
					e3 = false;
					++p2;
				}
				else if (type1 == _SPACE_ && e3) {
					e2 = false;
					++p1;
				}
				else {
					return false;
				}
			}
			else {
				return false;
			}
		}
	}
}

#endif

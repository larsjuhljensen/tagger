#ifndef __REFLECT_TIGHTVECTOR_HEADER__
#define __REFLECT_TIGHTVECTOR_HEADER__

#include <cstdlib>

template <typename T1, typename T2> class tightvector
{
	
	public:
		typedef T2* iterator;
	
	private:
		T1 n;
		union {T2 single; T2* array;} data;
	
	public:
		tightvector<T1, T2>() {
			n = 0;
		}
		
		tightvector<T1, T2>(const tightvector<T1, T2>& x) {
			n = x.n;
			if (n == 0) {
			}
			else if (n == 1) {
				data.single = x.data.single;
			}
			else {
				data.array = (T2*)malloc(n*sizeof(T2));
				memcpy(data.array, x.data.array, n*sizeof(T2));
			}
		}
		
		~tightvector<T1, T2>() {
			if (n > 1) {
				free(data.array);
			}
		}
	
	public:
		tightvector<T1, T2>& operator=(const tightvector<T1, T2>& x) {
			if (n > 1) {
				free(data.array);
			}
			n = x.n;
			if (n == 0) {
			}
			else if (n == 1) {
				data.single = x.data.single;
			}
			else {
				data.array = (T2*)malloc(n*sizeof(T2));
				memcpy(data.array, x.data.array, n*sizeof(T2));
			}
			return *this;
		}
		
		T2& operator[](T1 index) {
			T2* p;
			if (index >= n) {
				p = NULL;
			}
			else if (n == 1) {
				p = &data.single;
			}
			else {
				p = data.array+index;
			}
			return *p;
		}
		
		iterator begin() {
			if (n == 0) {
				return NULL;
			}
			else if (n == 1) {
				return &data.single;
			}
			else {
				return data.array;
			}
		}
		
		iterator end() {
			if (n == 0) {
				return NULL;
			}
			else if (n == 1) {
				return &data.single+1;
			}
			else {
				return data.array+n;
			}
		}
		
		void push_back(const T2& value) {
			if (n == 0) {
				data.single = T2(value);
			}
			else if (n == 1) {
				T2* p = (T2*)malloc(2*sizeof(T2));
				p[0] = data.single;
				p[1] = T2(value);
				data.array = p;
			}
			else {
				data.array = (T2*)realloc(data.array, (n+1)*sizeof(T2));
				data.array[n] = value;
			}
			n++;
		}
		
		T1 size(void) const {
			return n;
		}
};

#endif
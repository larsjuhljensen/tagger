CC     = g++
CFLAGS = -fpic -Wall -O3 -std=c++11
LFLAGS = -fpic -shared -lboost_regex
PYTHON = -I/usr/include/python2.7 -I/usr/include/python

all: tagger_swig.py _tagger_swig.so libtagger.so libtagger.a tagcorpus cleandict environments organisms species

clean:
	rm -f tagger_swig.py tagcorpus cleandict environments organisms species spring *.pyc *_wrap.cxx *.o *.a *.so

#
# Make C-style Python interpreter wrapper C-code layer.
#
tagger_swig.py tagger_swig_wrap.cxx: tagger_swig.i tagger.h
	swig -python -c++ -threads $<
	
tagger.o: tagger.cxx acronyms.h tagger.h tagger_types.h tagger_core.h acronyms.h hash.h tokens.h
	$(CC) $(CFLAGS) -c $<

tagger_swig_wrap.o: tagger_swig_wrap.cxx tagger.h tagger_types.h tagger_core.h acronyms.h hash.h tokens.h
	$(CC) $(CFLAGS) $(PYTHON) -c $<

#
# Link to make the Python wrapped C/C++ tagger interface in
# the Shared Object: _tagger_swig.so.
#
_tagger_swig.so: tagger_swig_wrap.o
	$(CC) $(LFLAGS) -o $@ $<

#
# Link to make the C/C++ stand-alone Shared Object for
# inclusing the C/C++ tagger in other C++ programs.
#
libtagger.so: tagger.o
	$(CC) $(LFLAGS) -o $@ $<

libtagger.a: tagger.o
	ar -rfs -o $@ $<

tagcorpus: tagcorpus.cxx acronyms.h document.h file.h hash.h mutex.h thread.h match_handlers.h base_handlers.h meta_handlers.h print_handlers.h score_handlers.h segment_handlers.h batch_tagger.h threaded_batch_tagger.h tagger.h tagger_core.h tagger_types.h tightvector.h tokens.h
	$(CC) $(CFLAGS) -lboost_regex -pthread -o $@ $< -lm

cleandict: cleandict.cxx acronyms.h file.h hash.h tagger.h tagger_core.h tagger_types.h
	$(CC) $(CFLAGS) -lboost_regex -pthread -o $@ $< -lm

%: %.cxx acronyms.h document.h file.h hash.h mutex.h match_handlers.h base_handlers.h batch_tagger.h tagger.h tagger_core.h tagger_types.h tightvector.h tokens.h
	$(CC) $(CFLAGS) -lboost_regex -pthread -o $@ $< -lm

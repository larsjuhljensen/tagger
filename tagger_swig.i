%module tagger_swig

%typemap(in) int* entity_types {
	int size = PyList_Size($input);
	$1 = new int[size];
	for (int i = 0; i < size; i++) {
		$1[i] = PyInt_AsLong(PyList_GetItem($input, i));
	}
}

%typemap(freearg) int* entity_types {
	delete $1;
}

%typemap(out) Entities {
	PyObject* entities = PyTuple_New((int)$1.size());
	for (int i = 0; i < (int)$1.size(); i++) {
		PyObject* entity = PyTuple_New(2);
		PyTuple_SetItem(entity, 0, PyInt_FromLong($1.at(i).type));
		if ($1.serials_only) {
			PyTuple_SetItem(entity, 1, PyInt_FromLong($1.at(i).id.serial));
		}
		else {
			PyTuple_SetItem(entity, 1, PyString_FromString($1.at(i).id.string));
		}
		PyTuple_SetItem(entities, i, entity);
	}
	$result = entities;
}

%typemap(out) Matches {
	PyObject* matches = PyList_New((int)$1.size());
	for (int i = 0; i < (int)$1.size(); i++) {
		PyObject* match = PyTuple_New(3);
		PyTuple_SetItem(match, 0, PyInt_FromLong($1.at(i)->start));
		PyTuple_SetItem(match, 1, PyInt_FromLong($1.at(i)->stop));
		if ($1.at(i)->size > 0) {
			PyObject* entities = PyTuple_New((int)$1.at(i)->size);
			for (int j = 0; j < (int)$1.at(i)->size; j++) {
				PyObject* entity = PyTuple_New(2);
				PyTuple_SetItem(entity, 0, PyInt_FromLong($1.at(i)->entities[j].type));
				if ($1.serials_only) {
					PyTuple_SetItem(entity, 1, PyInt_FromLong($1.at(i)->entities[j].id.serial));
				}
				else {
					PyTuple_SetItem(entity, 1, PyString_FromString($1.at(i)->entities[j].id.string));
				}
				PyTuple_SetItem(entities, j, entity);
			}
			PyTuple_SetItem(match, 2, entities);
		}
		else {
			Py_INCREF(Py_None);
			PyTuple_SetItem(match, 2, Py_None);
		}
		PyList_SetItem(matches, i, match);
	}
	$result = matches;
}

%include "tagger.h"

%{
#include "tagger.h"
%}


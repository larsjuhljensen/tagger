# README #

## What is this repository for? ##

This repository holds tagcorpus, a C++ program that, most generally, allows you to tag a corpus of documents with search terms that you provide.  It is often used to find mentions of proteins, species, diseases, tissues, chemicals and drugs, GO terms, and so forth, in articles in the Medline corpus.  

## Textmining in a bit more detail ##

"Tagging" a document is the process of recording where in the document given entities are mentioned.

Let's say you are interested in the human protein P53.  You want to find all the mentions of P53 in the literature.  However, P53 can be spelled in a variety of ways P53, P-53, etc, and you would like to find all of them.

Some of this expansion is done automatically.  


## Running tagcorpus ##

### Prerequisites ###

tagcorpus is known to compile under Suse Linux (version?).  If you are compiling under MacOSX or another version of linux, you will need to install and configure the following libraries (beyond the scope of this document). 

* swig
* boost

### Compiling and installation ###

There is no configure step, just

~~~~
make
~~~~

and then ensure tagcorpus is in your path.

### Input Files ###

You will need several input files to run tagcorpus.

#### Entities ####

The entities file specifies some metadata about the things that you want to tag.  You will have a line in this file for every entity that you want to tag.  Entities are things like proteins, species, diseases, tissues, chemicals and drugs, GO terms.  

The entities file contains the following three tab separated columns:
* **serialno**, a unique positive integer.  Serial numbers do not need to occur in sorted order, they just need to be unique.
* **entity type**, an integer that specifies what kind of thing (protein, species, disease, ...) the entity is.  If the entity is a species, it's entity type will be its NCBI taxonomic identifier.  All other types have negative entity types, as specified below.
* **your identifier** for the entity.  If it's a protein, this is the string identifier, without the ncbitaxid prepended. 

Some examples:

A protein will look like this:
* serialno
* ncbitax
* myproteinid

A species will look like this:
* serialno
* -3
* ncbitax


#### Names ####

The names file specifies the actual strings of text that might be written in a document that refer to a specific entity. 

The names file contains 
* serialno
* synonym


### Running tagcorpus ###

### Who do I talk to? ###

* Contact: Lars Juhl Jensen, lars.juhl.jensen (at) cpr.ku.dk
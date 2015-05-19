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

The names file contains the following two tab separated columns:
* serialno
* synonym

There may be multiple lines for each serialno.



#### Typepairs ####

Often, one wants to record that two terms have occurred together.  To do this, a file that specifies the types of pairs that are allowed to pair 


#### Stopwords ####

Some of the synonyms that are specified in the names file may match more than just occurrences of the gene.  For example, there are human genes named RAN, etc.  We don't want this to match every  mention of someone running a gel, so we can explicitly specify 'ran' as a stopword.  

The tagger is case sensitive, so the stopword 'ran' will match exactly this string, but RAN will continue to be tagged as a gene.  LARS, please verify.

The format of the stopwords file is two tab separated columns:
* the word, which may be a string containing spaces
* either the string "t" or the string "f", according to whether it is a stopword or not

Only stopwords with t in the second column are used as stopwords.  This format makes it relatively easy to convert the output of the tagger run without stopwords into a stopword list that can be manually curated.  

#### Groups ####

Groups specifies the orthology groups of proteins that should be treated as the same protein.  For example, we can use this to specify that if we find a mention of the mouse protein X (LARS, need good example) then this is similar enough (== in the same orthology group) to the human protein that we will count it as "the same protein", but if we find a mention of the drosophila protein, then this is different enough that it is not the same protein.  

### Running tagcorpus ###

## Contact ##

* Contact: Lars Juhl Jensen, lars.juhl.jensen (at) cpr.ku.dk
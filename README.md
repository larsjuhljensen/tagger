# README #

## What is this repository for? ##

This repository contains the code for tagcorpus, a C++ program that, most generally, allows you to tag a corpus of documents with search terms that you provide.  It is often used to find mentions of proteins, species, diseases, tissues, chemicals and drugs, GO terms, and so forth, in articles in the Medline corpus.  

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

### Input Files and Options ###

You will need several input files to run tagcorpus.  Entities and names are required, and the others are optional.

There are two modes that tagcorpus can be run in, auto-detect and no auto-detect. 

By default, proteins are assumed to be human proteins if no specifying organism is mentioned.

#### Entities ####

Specified with the --entities command line option.

The entities file specifies some metadata about the things, or entities, that you want to tag.  You will have a line in this file for every entity that you want to tag.  Entities are things like proteins, species, diseases, tissues, chemicals and drugs, GO terms.  

The entities file contains the following three tab separated columns:

1. **serialno**, a unique positive integer.  Serial numbers do not need to occur in sorted order, they just need to be unique.
2. **entity type**, an integer that specifies what kind of thing (protein, species, disease, ...) the entity is.  If the entity is a species, it's entity type will be its NCBI taxonomic identifier.  All other types have negative entity types, as specified below.
3. **your identifier** for the entity.  If it's a protein, this is the string identifier, without the ncbitaxid prepended. 


| Type | entity type              |
| ---- | ------------------------ |
| -1   | chemicals                |
| -2   | ????                     |
| -3   | NCBI species taxonomy id |
| -11  | Wikipedia                |
| -21  | GO biological process    |
| -22  | GO cellular component    |
| -23  | GO molecular function    |
| -24  | GO other (unused)        |
| -25  | BTO tissues              |
| -26  | DOID diseases            |
| -27  | ENVO environments        |
| -28  | APO phenotypes           |
| -29  | FYPO phenotypes          |
| -30  | MPheno phenotypes        |
| -31  | NBO behaviors            |



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

The names file specifies the actual strings of text that might be written in a document that refer to a specific entity.  It is specified with the --names command line option.

The names file contains the following two tab separated columns:

1. **serialno**
2. **synonym**

There may be multiple lines for each serialno.



#### Type-pairs ####

Often, one wants to record that two terms have occurred together, not just the fact that either has occurred individually.  For example, we might be interested in viral proteins that interact with human proteins, and we want to generate a score for how often influenza and human proteins are mentioned together.  

To do this, a file that specifies the types of entities that form interesting pairs should be generated.  It is specified with the --type-pairs command line option.  The type-pairs file contains the following two tab separated columns:

* type1
* type2

Then, I would specify a line containing the taxid for human (9606) and the taxid for influenza (11320).  

* 9606
* 11320

Order in this file doesn't matter.  LARS, please confirm.

As a further example, if I wanted to pair human proteins with diseases, I would specify

* 9606
* -26

NB: additionally, an output file for the scored pairs output must also be specified, or the output will not be recorded anywhere.  This is done with the --out-pairs option. 

#### Optional options for scoring pairs ####

By default, pairs are given a score of 1 if they occur in the same document, a score of 2 if they occur in the same paragraph, and a score of 0.2 if they occur in the same sentence.  The parameter a in the following formula controls the weight of the normalization factor.

score = c_jk^a * ( c_ij * c_.. / c_i. * c_.j )^(1-a)

Where . is shorthand for "count over all entities of the given type".  

These values can be set with the --document-weight, --paragraph-weight, --sentence-weight and --normalization-factor command line options respectively.


#### Organism ####

--organism=ncbitaxid

This is equivalent to specifying the species entities in the entities file.  LARS, please verify.






#### Stopwords ####

Some of the synonyms that are specified in the names file may match more than just occurrences of the gene.  For example, there are human genes named RAN, etc.  We don't want this to match every  mention of someone running a gel, so we can explicitly specify 'ran' as a stopword.  

The tagger is case sensitive, so the stopword 'ran' will match exactly this string, but RAN will continue to be tagged as a gene.  LARS, please verify.

The format of the stopwords file is two tab separated columns:

* the word, which may be a string containing spaces
* either the string "t" or the string "f", according to whether it is a stopword or not

Only stopwords with t in the second column are used as stopwords.  This format makes it relatively easy to convert the output of the tagger run without stopwords into a stopword list that can be manually curated.  

#### Groups ####

Groups specifies the orthology groups of proteins that should be treated as the same protein.  For example, we can use this to specify that if we find a mention of the mouse protein CDK1 then this is similar enough (== in the same orthology group) to the human protein CDK1 that we will count it as "the same protein", but if we find a mention of the drosophila protein CDK1, then this is different enough that it is not the same protein.  


#### Threads ####

--threads 

By default tagcorpus runs single threaded.

### Running tagcorpus ###

## Contact ##

* Contact: Lars Juhl Jensen, lars.juhl.jensen (at) cpr.ku.dk
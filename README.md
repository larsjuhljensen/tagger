# README #

## What is this repository for? ##

This repository holds tagcorpus, a C++ program that, most generally, allows you to tag a corpus of documents with search terms that you provide.  It is often used to tag articles in the Medline corpus with names of proteins, species, diseases, tissues, chemicals and drugs, GO terms, and so forth.

## How do I get set up? ##

### Prerequisites ###

tagcorpus is known to compile under Suse Linux (version?).  

* swig
* boost

### Installation ###

  make

Make sure tagcorpus is in your path.

### Input Files ###

You will need several input files to run tagcorpus.

#### Entities ####

The entities file is required, and specifies what the things are that you want to tag.  You will have a line in this file for every entity that you want to tag.  Entities are things like proteins, species, diseases, tissues, chemicals and drugs, GO terms.  

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

The format is 
* serialno
* synonym


### Running tagcorpus ###

### Who do I talk to? ###

* Contact: Lars Juhl Jensen, lars.juhl.jensen (at) cpr.ku.dk
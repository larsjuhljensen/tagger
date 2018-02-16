# README #

## What is this repository for? ##

This repository contains the code for tagcorpus, a C++ program that, most generally, allows you to tag a corpus of documents with search terms that you provide.  It is often used to find mentions of proteins, species, diseases, tissues, chemicals and drugs, GO terms, and so forth, in articles in the Medline corpus.  

## Textmining in a bit more detail ##

"Tagging" a document is the process of recording where in the document given entities are mentioned.  Often two types of information are tagged concurrently to find co-mentions.  For example, proteins and diseases, or human proteins and viral proteins.  

Let's say you are interested in the human protein P53.  You want to find all the mentions of P53 in the literature.  However, P53 can be spelled in a variety of ways P53, P-53, p53, "Tumor protein p53", etc, so this is a harder problem than it originally seems.  This tagger will do some of this expansion automatically: case insensitivity, and adding dashes before terminal numbers.  


## Running tagcorpus ##

### Prerequisites ###

tagcorpus is known to compile under Suse Linux.  If you are compiling under MacOSX or another distribution of linux, you will need to install and configure the following libraries (beyond the scope of this document). 

* swig
* boost

### Docker ###

This software is also available as a Docker from http://hub.docker.com/r/larsjuhljensen/tagger/.

### Compiling and installation ###

To configure, edit the makefile and ensure that the CFLAGS variable includes a -I (capital i) option that points to your Python.h wherever it is installed on your system.

~~~~
make
~~~~

and then ensure tagcorpus is in your path.

Alternatively, you can download a Docker image with the tagger already compiled.

### Predefined dictionaries ###

Dictionaries for [tissues](http://download.jensenlab.org/tissues_dictionary.tar.gz), [organisms](http://download.jensenlab.org/organisms_dictionary.tar.gz), [environments](http://download.jensenlab.org/environments_dictionary.tar.gz), [diseases](http://download.jensenlab.org/diseases_dictionary.tar.gz), [compartments](http://download.jensenlab.org/compartments_dictionary.tar.gz), [phenotypes](http://download.jensenlab.org/phenotypes_dictionary.tar.gz), proteins (from [arabidopsis](http://download.jensenlab.org/arabidopsis_dictionary.tar.gz), [human](http://download.jensenlab.org/human_dictionary.tar.gz), [fly](http://download.jensenlab.org/fly_dictionary.tar.gz), [mouse](http://download.jensenlab.org/mouse_dictionary.tar.gz), [pig](http://download.jensenlab.org/pig_dictionary.tar.gz), [rat](http://download.jensenlab.org/rat_dictionary.tar.gz), [viruses](http://download.jensenlab.org/virus_dictionary.tar.gz), [worm](http://download.jensenlab.org/worm_dictionary.tar.gz), or [yeast](http://download.jensenlab.org/yeast_dictionary.tar.gz)) and [everything](http://download.jensenlab.org/tagger_dictionary.tar.gz) are available for use.


### Input Files and Options ###

You will need several input files to run tagcorpus.  Documents, entities, names and types are required, and the others are optional.

There are two modes that tagcorpus can be run in, auto-detect and no auto-detect.  Autodetect is specified with the --autodetect option and is by default off.  See below for more details.

By default, proteins are assumed to be human proteins if no other species (positive integers) are given in the types file.

#### Documents ####

The documents to be searched are by default provided on stdin, unless a file is specified with the --documents command line argument, in which case this file is read as input instead. 

If you are using the CPR server infrastructure, the Medline documents are available under /home/purple1/databases/Medline

The format of the input files are tab separated columns that contain the following data:

* identifier
* authors
* journal
* year
* title
* text

#### Types ####

Specified with the --types command line option.  Types is a tab delimited file that contains the integer representing the types of entities you are interested in tagging.  

| Type | entity type              |
| ---- | ------------------------ |
| any > 0 | Proteins of species with this NCBI tax id |
| -1   | chemicals                |
| -2   | NCBI species taxonomy id (tagging species) |
| -3   | NCBI species taxonomy id (tagging proteins) |
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
| -36  | mammalian phenotypes     |

The difference between -2 and -3 is as follows.  Use -2 if you are only interested in tagging species.  Use -3 if you ultimately want to tag proteins -- -3 will turn on autodetect, which will add another tagging step in which your document will be tagged for the species you specify, and then will be tagged again for proteins.  Protein mentions must occur near mentions of their species to be recorded (so that protein names that are the same across different species are not confused for each other).  (Use the --groups option if you DO want to allow proteins of the same name in certain species to be considered the same entity, for example mouse CDK1 and human CDK1 which are close orthologs.)


#### Entities ####

Specified with the --entities command line option.

The entities file specifies some metadata about the things (called entities) that you want to tag.  These can be proteins, species, diseases, tissues, chemicals and drugs, GO terms, etc.  There should be a line in this file for each entity that you want to tag.  

The entities file contains the following three tab separated columns:

1. **serialno**, a unique positive integer.  Serial numbers do not need to occur in sorted order, they just need to be unique.
2. **entity type**, an integer that specifies what kind of thing (protein, species, disease, ...) the entity is.  If the entity is a protein for a specific species, it's entity type will be the NCBI taxonomic identifier of the species.  All other types have negative entity types, as specified below.
3. **your identifier** for the entity.  If it's a protein, this is the string identifier, without the ncbitaxid prepended. 


Some examples:

A protein will be described in the entities file with the following columns:

* serialno
* ncbitax
* myproteinid

And a species will look like this:

* serialno
* -3
* ncbitax


#### Names ####

The names file specifies the actual strings of text that might be written in a document that refer to a specific entity.  The names file is specified with the --names command line option.

The names file contains the following two tab separated columns:

1. **serialno**
2. **synonym**

There may be multiple lines for each serialno.



#### Type-pairs ####

Often, one wants to record that two terms have occurred together, not just the fact that either has occurred individually.  For example, we might be interested in viral proteins that interact with human proteins, and would want to find out which human proteins the influenza protein hemagglutinin interacts with, and to generate a score for each pair.

To do this, a file that specifies the types of entities that can form interesting pairs should be generated.  It is specified with the --type-pairs command line option.  The type-pairs file contains the following two tab separated columns:

* type1
* type2

Then, in the human and influenza protein example, I would specify a line containing the taxid for human (9606) and the taxid for influenza (11320).  

* 9606
* 11320

Order in this file doesn't matter.

As a further example, if I wanted to pair human proteins with diseases, I would specify

* 9606
* -26

NB: additionally, an output file for the scored pairs output must also be specified, or the output will not be recorded anywhere.  This is done with the --out-pairs option. 

#### Optional options for scoring pairs ####

By default, pairs are given a score of 1 if they occur in the same document, a score of 2 if they occur in the same paragraph, and a score of 0.2 if they occur in the same sentence.  The parameter a in the following formula controls the weight of the normalization factor (actually, 1-a is the exponent on the normalization factor, but let's not be too pedantic).

score = c_ij^a * ( c_ij * c_.. / c_i. * c_.j )^(1-a)  

Where . is shorthand for "count over all entities of the given type".  

These values can be set with the --document-weight, --paragraph-weight, --sentence-weight and --normalization-factor command line options respectively.


#### Deprecated: Organism ####

--organism=ncbitaxid

This is equivalent to specifying the species entities in the entities file. Specifying an organism turns off autodectect (which was turned on by specifying entities of type -3). 


#### Autodetect ####

--autodetect will turn on autodetect.  


#### Stopwords ####

Some of the synonyms that are specified in the names file may match more than just occurrences of the gene.  For example, there are human genes named RAN, etc.  We don't want to match every instance that a researcher "ran a gel", so we can explicitly specify 'ran' as a stopword.  

The tagger is case sensitive, so the stopword 'ran' will match exactly this string, but RAN will continue to be tagged as a gene.

The format of the stopwords file is two tab separated columns:

* the word, which may be a string containing spaces
* either the string "t" or the string "f", according to whether it is a stopword ("t") or is whitelisted ("f")

Only stopwords with t in the second column are used as stopwords.  This format makes it relatively easy to convert the output of the tagger run without stopwords into a stopword list that can be manually curated.  

Whitelisting: the tagger will ignore words of less than 3 characters by default.  To explicitly whitelist such words, place them on the stoplist with "f" in the second column.

#### Groups ####

The groups file, specified with the --groups command line argument contains the orthology groups of proteins that should be treated as the same protein (and analogously for other entity types like tissues from BTO, diseases, etc).  For example, if we are interested in tagging the human protein CDK1, and we know that humans are similar enough to mice that a paper containing findings about the mouse CDK1 are likely to also be applicable to the human CDK1.  We specify "similar enough", using the groups file to say that human CDK1 and mouse CDK1 are in the same orthology group.

Thus, groups have an impact on named entity recognition (NER) and on disambiguation.  Each group must be added as an entity, but it doesn't necessarily need to have any names associated with it. For OGs, the group will have the type of the taxid that the OG is calculated at (mammals, metazoa, etc).  

Regarding disambiguation, if a protein name is recognized that normalizes to two different entities, then neither entity will be returned.  However, if the entities are in the same group, then both entities will be returned.  For example, the protein TP53 is called "TP53" in both mouse and human, and these proteins are orthologs, so a mention of "TP53" in text will result in both the human and mouse TP53 entities being returned. 

If you want to have results returned for the groups (as opposed to just using them to return more results), then these entities must be specified in the types and type-pairs files.

Adding groups will not change the scoring, but will return more matches (of different types), because the scoring is based on type and the number of matches of each type will not change. 

The format of the groups file is as follows

* child
* parent

Specify each edge of the tree (child -> parent; parent -> grandparent; child -> grandparent) in the groups file.  Non-tree-like edges (shortcuts) between nodes can be added manually.  


#### Threads ####

By default tagcorpus runs single threaded, but can be configured using the --threads command line option to run with any integer number of threads (best matched to the number of CPU cores that are available to you). 

### Output Formats ###

#### Mentions ####

The output of tagging individual entities is output on stdout unless a file is specified with the --out-matches command line argument.  This output contains the following 8 tab separated columns.

1. pubmedid
2. paragraph number
3. sentence number
4. first character of the match
5. last character of the match
6. term matched
7. species taxid
8. serialno

#### Pairs ####

If --type-pairs and --output-pairs are specified, then output will be written to the file specified by --output-pairs.  This output contains 8 tab separated columns, but only the first three are relevant.  (The others can be used if the output is to be incrementally updated, but that is beyond the scope of this document.)

1. serialno1
2. serialno2
3. final score
4. all remaining columns are unused

#### Sentence segmentation ####

If --out-segments is specified, the sentence segmentation will be written to the given file. The output contains the following tab separated columns:

1. pubmedid
2. paragraph number
3. sentence number
4. sentence segment start character
5. sentence segment end character

## Examples of use ##

Example: specify stopwords and pairs, and output pairs output to a file called output-pairs.
~~~~
gzip -cd `ls -1r /home/purple1/databases/Medline/*.tsv.gz` | tagcorpus --entities=entities \\
--names=names --stopwords=all_global.tsv --type-pairs=typepairs --threads=16 \\
--out-pairs=output-pairs --types=types --out-segments=all_segments.tsv > output-mentions
~~~~

## TODO ##

* Autogenerating the dictionary
* post processing scripts to write the output into string db

## Contact ##

* Contact: Lars Juhl Jensen, lars.juhl.jensen (at) cpr.ku.dk
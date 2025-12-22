#ifndef _CIRCUITREADER_H
#define _CIRCUITREADER_H

#include "cnrbtree.h"

#define MAXNETSPERNODE 4
#define MAXMODELPARS 4

typedef struct
{
	char * id;
	char * pars[MAXMODELPARS];
} model;

typedef struct
{
	char * id;
	char * type;
	int    numnets;
	int    nets[MAXNETSPERNODE];
} component;

typedef component * componentptr;
typedef char * str;

CNRBTREETEMPLATE( str, int, RBstrcmp, RBstrcpy, RBstrdel );
CNRBTREETEMPLATE( str, str, RBstrcmp, RBstrcpy, RBstrdel );
CNRBTREETEMPLATE( str, componentptr, RBstrcmp, RBstrcpy, RBstrdel );
CNRBTREETEMPLATE( str, model, RBstrcmp, RBstrcpy, RBstrdel );

typedef struct cir_reader_t
{
	int     nNets;

	component * components;
	int numComponents;

	char ** netNames;
	int numNets;

	cnrbtree_strint   * netmap;

	cnrbtree_strmodel * modelmap;
	cnrbtree_strcomponentptr * componentmap;

	cnrbtree_strint * tpmap;
} cir_reader;

int CircuitLoad( cir_reader * s, char * cir );

int ProcessEngineeringNumber( const char * e, float * num );

#endif


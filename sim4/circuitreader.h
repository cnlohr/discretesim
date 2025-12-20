#ifndef _CIRCUITREADER_H
#define _CIRCUITREADER_H

#include "cnrbtree.h"

typedef char * str;
CNRBTREETEMPLATE( str, int, RBstrcmp, RBstrcpy, RBstrdel );
CNRBTREETEMPLATE( str, str, RBstrcmp, RBstrcpy, RBstrdel );


typedef struct cir_sim_t
{
	int     nNets;
//	void * arena;
//	int    arenalen;
//	float * vVoltages;
//	float * vCurrents;

	cnrbtree_strint * netmap;
	cnrbtree_strstr * modelmap;
} cir_sim;

int CircuitLoad( cir_sim * s, char * cir );

#endif


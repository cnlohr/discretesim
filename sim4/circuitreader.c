#include <stdio.h>

#define CNRBTREE_IMPLEMENTATION
#include "cnrbtree.h"

#include "circuitreader.h"

static char * pull_sym( char ** s, char * term )
{
	char * sp = *s, * sr = sp;
	char c;
	do
	{
		c = *(sr++);
		if( !c )
		{
			*term = c;
			*s = sr;
			return sp;
		}
	}
	while( c == ' ' || c == '\t' || c == '\r' || c == '\n' );

	sp = sr-1;

	do
	{
		c = *(sr++);
	}
	while( !( c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '\0' ) );
	*term   = c;
	*(sr-1) = 0;
	*s = sr;
	return sp;
}

static char * dump_line( char ** sp )
{
	char * s = *sp;
	char * st = s;
	int c;
	do
	{
		c = *s;
		if( c == 0 )
		{
			*sp = s;
			return s;
		}
		s++;
	} while( c != '\n' && c != '\r' );
	*(s-1) = 0;
	*sp = s;
	return st;
}

int CircuitLoad( cir_sim * s, char * cir )
{
	/*
		.title KiCad schematic
		.model __N1 VDMOS NCHAN
		MN1 B0NQ B0S GND __N1
		R2 B0Q B0S 1k
		R3 VDD Net-_D1-A_ 1k
		.end
	*/
	int topnetid = 0;

	s->netmap = cnrbtree_strint_create();
	s->modelmap = cnrbtree_strstr_create();

	char term;
	char * sp;
	do
	{
		sp = pull_sym( &cir, &term );
		printf( "*%s\n", sp );
		if( term == 0 ) break;
		if( strcmp( sp, ".title" ) == 0 ) dump_line( &cir );
		else if( strcmp( sp, ".model" ) == 0 )
		{
			char * nameid = pull_sym( &cir, &term );
			RBA( s->modelmap, nameid ) = dump_line( &cir );
		}
		else
		{
			printf( "-%s-(%d)-", sp, term );
		}
	} while( term );

	s->nNets = s->netmap->size;

	return 0;
}


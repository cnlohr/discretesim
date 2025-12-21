#include <stdio.h>

#define CNRBTREE_IMPLEMENTATION
#include "cnrbtree.h"

#include "circuitreader.h"


char * pullstr( char ** line )
{
	char * ret = *line;
	int c;
	do
	{
		c = *((*line));
		if( c == '\0' ) break;
		if( c == ' ' || c == '\t' ) break;
		(*line)++;
	} while( 1 );
	**line = 0;
	if( c ) (*line)++;
	return ret;
}

int CircuitLoad( cir_reader * s, char * cir )
{
	memset( s, 0, sizeof( *s ) );

	s->netmap = cnrbtree_strint_create();
	s->modelmap = cnrbtree_strmodel_create();
	s->componentmap = cnrbtree_strcomponentptr_create();

	int lineno = 0;
	int iseof = 0;
	do
	{
		char * line = cir;
		int i;
		do
		{
			char c = *(cir);
			if( c == '\n' ) { *cir = 0; cir++; break; }
			if( c == '\0' ) { iseof = 1; *cir = 0; cir++; break; }
			cir++;
		} while( 1 );

		lineno++;

		if( line[0] == '.' )
		{
			char * command = pullstr( &line );
			if( strcmp( command, ".model" ) == 0 )
			{
				model m;
				char * id = pullstr( &line );
				m.id = id;
				int nhas = 0;
				while( *line )
				{
					if( nhas >= MAXMODELPARS )
					{
						fprintf( stderr, "Error: too many parameters on line %d or fix MAXMODELPARS\n", lineno );
						exit( -5 );
					}
					m.pars[nhas++] = pullstr( &line );
				}

				RBA( s->modelmap, id ) = m;
			}
		}
		else if( strlen( line ) > 1 )
		{
			char * identifier = pullstr( &line );
			char * pars[MAXNETSPERNODE+1];
			char * last;
			int numparsmets = 0;

			while( *line )
			{
				if( numparsmets >= MAXNETSPERNODE+1 )
				{
					fprintf( stderr, "Error: too many parameters on line %d. Or fix MAXNETSPERNODE\n", lineno );
					exit( -5 );
				}
				last = pars[numparsmets++] = pullstr( &line );
			}

			component m;
			m.id = identifier;
			m.type = last;
			m.numnets = numparsmets-1;
			int i;
			for( i = 0; i < numparsmets-1; i++ )
			{
				char * par = pars[i];
				if( RBHAS( s->netmap, par ) )
					m.nets[i] = RBA( s->netmap, par );
				else
				{
					int nid = s->numNets++;
					s->netNames = realloc( s->netNames, sizeof( char*) * s->numNets );
					s->netNames[nid] = par;
					m.nets[i] = nid;
					RBA( s->netmap, par ) = nid;
				}
			}

			int nc = s->numComponents++;
			s->components = realloc( s->components, s->numComponents * sizeof( component ) );
			s->components[nc] = m;
			RBA( s->componentmap, identifier ) = &s->components[nc];
		}
	} while( !iseof );

	return 0;
}


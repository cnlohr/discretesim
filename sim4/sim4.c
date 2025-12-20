#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#include "circuitreader.h"

struct supercomp_t
{
	void (*cb)( struct supercomp_t * s );
	// Other stuff...
};

typedef struct
{
	int     numNodes;
	float * nodeCaps;
	float * nodeVoltages;

	// Meta components
	int     numComps;
	float * compCaps;
	float * compRes;
	int * compTerms;

	struct supercomp_t ** superComps;
	int numSuperComps;
} ckt_sim;

int AddMetaComp( ckt_sim * s );

double Rand01()
{
	uint64_t ri = rand()&(0xfffff);
	ri<<=20;
	ri += rand()&(0xfffff);
	return ri / (double)(0xffffffffffULL);
}

//////////////////////////////////////////////////////////////////////

struct semicomp_nfet_t
{
	void (*cb)( struct semicomp_nfet_t * s );

	float * vG;
	float * vS;
	float * vD;

	float qGate;
	float qDiode;

	int mcompGS, mcompSD, mcompDG;
};

void NFetCB( struct semicomp_nfet_t * s )
{
}

void PFetCB( struct semicomp_nfet_t * s )
{
}

int AddFET( ckt_sim * sim, component * c, const char * type )
{
	if( c->numnets != 3 )
	{
		fprintf( stderr, "Error: FET has wrong number of nets\n" );
		return -5;
	}

	int nsc = sim->numSuperComps;
	int nscp1 = nsc+1;
	sim->numSuperComps = nscp1;
	sim->superComps = realloc( sim->superComps, sizeof( struct supercomp_t * ) * nscp1 );
	sim->superComps[nsc] =
		calloc( sizeof( struct semicomp_nfet_t ), 1 );
	struct semicomp_nfet_t * sc = (struct semicomp_nfet_t *)sim->superComps[nsc];

	int mcompGS = sc->mcompGS = AddMetaComp( sim );
	int mcompSD = sc->mcompSD = AddMetaComp( sim );
	int mcompDG = sc->mcompDG = AddMetaComp( sim );

	int * mcompGSterms = &sim->compTerms[mcompGS*2];
	int * mcompSDterms = &sim->compTerms[mcompSD*2];
	int * mcompDGterms = &sim->compTerms[mcompDG*2];

	sc->qGate  = Rand01() / 1.e-12; //in 0-1 pC
	sc->qDiode = Rand01() / 1.e-12; //in 0-1 pC

	// Nets are drain gate source
	int nG = c->nets[1];
	int nS = c->nets[2];
	int nD = c->nets[0];

	sc->vG = &sim->nodeVoltages[nG];
	sc->vS = &sim->nodeVoltages[nS];
	sc->vD = &sim->nodeVoltages[nD];

	mcompGSterms[0] = nG;
	mcompGSterms[1] = nS;
	mcompSDterms[0] = nS;
	mcompSDterms[1] = nD;
	mcompDGterms[0] = nD;
	mcompDGterms[1] = nG;

	sim->compCaps[mcompGS] = 1.e-12; // 1pF
	sim->compRes[mcompGS] = 1.e6;

	// Or PFet.
	if( strcmp( type, "NCHAN" ) == 0 )
	{
		sc->cb = NFetCB;
	}
	else if( strcmp( type, "PCHAN" ) == 0 )
	{
		sc->cb = PFetCB;
	}
	else
	{
		fprintf( stderr, "Error: Unknown fet type \"%s\"\n", type );
	}

	return 0;
}


int AddMetaComp( ckt_sim * s )
{
	int cid = s->numComps;
	int cp1 = cid + 1;
	s->numComps = cp1;
	s->compCaps = realloc( s->compCaps, cp1 * sizeof( float ) );
	s->compRes = realloc( s->compRes, cp1 * sizeof( float ) );
	s->compTerms = realloc( s->compTerms, cp1 * 2 * sizeof( int ) );
	return cid;
}

int ProcessEngineeringNumber( const char * e, float * num )
{
	int n = 0;
	int started = 0;
	int c;
	int isNeg = 0;
	double ret = 0.0;
	do
	{
		c = e[n++];
		int isnum = (c >= '0' && c <= '9' );
		if( !c ) break;
		if( !started && !isnum )
		{
			if( (c == ' ' || c == '\t' ) ) continue;
			if( c == '-' ) { isNeg = 1; started = 1; continue; }
			if( c == '+' ) { isNeg = 0; started = 1; continue; }
			fprintf( stderr, "Error: can't parse engineering number \"%s\"\n", e );
			return -1;
		}

		started = 1;

		if( isnum )
		{
			ret = ret * 10 + (c - '0');
		}
		else if( c == 'k' || c == 'K' )
		{
			ret *= 1000;
			break;
		}
		else if( c == 'M' )
		{
			ret *= 1000000;
			break;
		}
		else if( c == 'G' )
		{
			ret *= 1000000;
			break;
		}
		else if( c == 'm' )
		{
			ret /= 1000;
			break;
		}
		else if( c == 'u' )
		{
			ret /= 1000000;
			break;
		}
		else if( c == 'n' )
		{
			ret /= 1000000000;
			break;
		}
		else if( c == 'p' )
		{
			ret /= 1000000000000;
			break;
		}
		else
		{
			fprintf( stderr, "Error: unknown parameter in engineering number %c\n", c );
			return -5;
		}
	} while( 1 );
	
	do
	{
		c = e[n++];
		if( !c ) break;
		if( c == ' ' || c == '\t' ) continue;
		fprintf( stderr, "Error: junk after end of engineering number %c\n", c );
		return -6;
	} while( 1 );

	*num = (float)ret;
	return 0;
}

cir_reader cktfile = { 0 };
ckt_sim sim;

int main()
{
	int i;

	FILE * f = fopen( "kicad/kicad.cir", "r" );
	if( !f )
	{
		fprintf( stderr, "Error: can't open file\n" );
		return -5;
	}
	fseek( f, 0, SEEK_END );
	int len = ftell( f );
	fseek( f, 0, SEEK_SET );
	char * buffer = malloc( len );
	if( fread( buffer, len, 1, f ) != 1 )
	{
		fprintf( stderr, "Error reading file\n" );
		return -6;
	}
	
	int r = CircuitLoad( &cktfile, buffer );
	if( r )
	{
		printf( "Circuit Load failed: %d\n", r );
		return r;
	}

	printf( "Loaded %d nets\n", cktfile.numNets );
	for( i = 0; i < cktfile.numNets; i++ )
	{
		printf( "\t%s\n", cktfile.netNames[i] );
	}
	printf( "Loaded %d components\n", cktfile.numComponents );

	int numNodes = sim.numNodes = cktfile.numNets;
	sim.nodeCaps = calloc( sizeof( float ), numNodes );

	sim.numComps = 0;
	int numUnknown = 0;

	for( i = 0; i < cktfile.numComponents; i++ )
	{
		component * c = &cktfile.components[i];
		model * m = &RBA( cktfile.modelmap, c->type );
		char * par0 = m->pars[0];
		char * par1 = m->pars[1];
		char * cid = cktfile.components[i].id;
		printf( "\t%s; %s; %s ", cid, c->type, par1 );
		if( cid[0] == 'R' )
		{
			// A raw resistor.
			int rnum = AddMetaComp( &sim );
			sim.compCaps[rnum] = -1;
			if( ProcessEngineeringNumber( c->type, &sim.compRes[rnum] ) )
			{
				return -5;
			}
			printf( "\t\tAdded R %f\n", sim.compRes[rnum] );
		}
		else if( par1 && strcmp( par0, "VDMOS" ) == 0 )
		{
			printf( "\t\tAdding FET\n" );
			if( AddFET( &sim, c, par1 ) ) return -5;
		}
		else if( strncmp( cid, "LED", 3 ) == 0 )
		{
			printf( "\t\tTODO: Adding LED\n" );
		}
		else if( strncmp( cid, "TP", 2 ) == 0 )
		{
			printf( "\t\tTODO: Adding TP\n" );
		}
		else
		{
			printf( "Unknown component\n" );
			numUnknown++;
		}
	}

	if( numUnknown )
		printf( "Unknown components %d\n", numUnknown );

	printf( "Metacomponents: %d\n", sim.numComps );

	return 0;



#if 0

	// Testing for a setup like:
	//   (R+C) for all capacitors.
	//   Possibly (R1||(R2+C))

	float tdelta = 1.0e-12;
	float mintargetnodecap = 1.0e-12; // Target capacitance for node (tell simulator not to go below this, even if it would be numerically stable.)

	#define NODES 6
	#define COMPS 5
	float NodeVoltages[NODES] = { 0 };
	float ComponentCurrents[COMPS] = { 0 };

	// Instantaneous I deltas
	float NodeIs[NODES] = { 0 };

	const float ComponentRs[COMPS] = { 2, 1, 50, 1000, 1000 };
	const float ComponentCs[COMPS] = { -1, 30.e-12, -1, 1.e-12, -1 };
	float ComponentVDiff[COMPS] = { 0 };
	int ComponentTerms[COMPS][2] = {
		{ 0, 1 },
		{ 1, 2 },
		{ 2, 3 },
		{ 2, 4 },
		{ 4, 5 } };
	float NodeCaps[NODES] = { 0 };
	
	int c;
	for( c = 0; c < COMPS; c++ )
	{
		NodeCaps[ComponentTerms[c][0]] += tdelta / 1.9 / ComponentRs[c];
		NodeCaps[ComponentTerms[c][1]] += tdelta / 1.9 / ComponentRs[c];
	}
	int n;
	for( n = 0; n < NODES; n++ )
	{
		if( NodeCaps[n] < mintargetnodecap )
			NodeCaps[n] = mintargetnodecap;
		//printf( "NC %d %f\n", n, NodeCaps[n]*1.e12 );
	}

	int iteration;
	for( iteration = 0; iteration < 10000; iteration++ )
	{
		NodeVoltages[0] = (sin(iteration/1000.0) * 2.50) > 0.0 ? 5.0 : 0.0;
		NodeVoltages[3] = 0.0;
		NodeVoltages[5] = 5.0;
		for( n = 0; n < NODES; n++ )
		{
			NodeIs[n] = 0;
		}

		for( c = 0; c < COMPS; c++ )
		{
			int n0 = ComponentTerms[c][0];
			int n1 = ComponentTerms[c][1];
			float v0 = NodeVoltages[n0];
			float v1 = NodeVoltages[n1];
			float cdiff = ComponentVDiff[c];
			float vDiff = v1 - v0 + cdiff;

			float cv = ComponentCs[c];
			float rv = ComponentRs[c];

			float deltaI = vDiff / rv;
			NodeIs[n0] += deltaI/2;
			NodeIs[n1] -= deltaI/2;
			
			if( cv > 0 )
				cdiff -= deltaI / cv * tdelta;
			ComponentVDiff[c] = cdiff;
			//printf( "%f,%f,%f,+%c", deltaI, cdiff, vDiff, (c == COMPS-1) ? '\n':',' );
		}
		
		for( n = 0; n < NODES; n++ )
		{
			float nv;
			NodeVoltages[n] = nv = NodeVoltages[n] + NodeIs[n] / (NodeCaps[n]) * tdelta;
			//if( (iteration % 100) == 0)
			printf( "%f,%f,+%c", nv, NodeIs[n], (n == NODES-1) ? '\n':',' );
		}

	}
#endif
}

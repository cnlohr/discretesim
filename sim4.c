// TODO: Make inductor go into resistive mode.

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>

#include "circuitreader.h"

float tdelta = 4.0e-12;

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
	float * nodeCurrents;


	// Testing for a setup like:
	//   (R+C) for all capacitors.
	//   Possibly (R1||(R2+C))
	//   For R only, use C=-1

	// Meta components
	int     numComps;
	float * compCaps;
	float * compRes;
	float * compVDiff;
	float * compVDiffResidue;
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

void * AddSuperComp( ckt_sim * sim, int size )
{
	int nsc = sim->numSuperComps;
	int nscp1 = nsc+1;
	sim->numSuperComps = nscp1;
	sim->superComps = realloc( sim->superComps, sizeof( struct supercomp_t * ) * nscp1 );
	void * ret = sim->superComps[nsc] = calloc( size, 1 );
	return ret;
}

//////////////////////////////////////////////////////////////////////

struct semicomp_nfet_t
{
	void (*cb)( struct semicomp_nfet_t * s );

	float * vG;
	float * vS;
	float * vD;

	int termRindex;
	float * termR;
	float gateCur;

	float gateTau;

	int mcompGS, mcompSD, mcompSDR, mcompDG;

	ckt_sim * sim;
};

void NFetCB( struct semicomp_nfet_t * s )
{
	const float vgsTh = 1.1;

	float vGS = *s->vG - *s->vS;
	float dGSQ = (vGS - vgsTh);

	float gc = s->gateCur * (1-s->gateTau) + dGSQ * s->gateTau;
	if( gc < -1 ) gc = -1;
	if( gc > 5  ) gc = 5;
	s->gateCur = gc;

	float fetR;
	if( gc < 0.00001 )
		fetR = 10.e6;
	else
		fetR = 15/gc;

	float vfDiode = (*s->vS - *s->vD - 1.6);
	float rDiode = 10.e6;
	if( vfDiode > 0.00001 )
		rDiode = 20/vfDiode;

	//printf( "NFetR: %f %f %f -> %d %d  %d %d\n", fetR, rDiode, vGS, s->sim->compTerms[s->mcompGS*2+0], s->sim->compTerms[s->mcompGS*2+1], s->sim->compTerms[s->mcompSDR*2+0], s->sim->compTerms[s->mcompSDR*2+1] );

	float * r = s->termR;
	if( !r ) r = s->termR = &s->sim->compRes[s->termRindex];
	*r = 1.0/(1.0/fetR + 1.0/rDiode);
}

void PFetCB( struct semicomp_nfet_t * s )
{
	const float vgsTh = 1.2;

	float vSG = *s->vS - *s->vG;
	float dGSQ = (vSG - vgsTh);

	// This is a little strange - we are chating here, and simulating
	// a RC falloff here.
	float gc = s->gateCur * (1.-s->gateTau) + dGSQ * s->gateTau;

	if( gc < -1 ) gc = -1;
	if( gc > 5  ) gc = 5;
	s->gateCur = gc;

	float fetR;
	if( gc < 0.00001 )
		fetR = 10.e6;
	else
		fetR = 20/gc;

	float vfDiode = (*s->vD - *s->vS - 1.6);
	float rDiode = 10.e6;
	if( vfDiode > 0.00001 )
		rDiode = 20/vfDiode;
	
	//printf( "PFetR: %f %f %f -> %d %d  %d %d\n", fetR, rDiode, vSG, s->sim->compTerms[s->mcompGS*2+0], s->sim->compTerms[s->mcompGS*2+1], s->sim->compTerms[s->mcompSDR*2+0], s->sim->compTerms[s->mcompSDR*2+1] );
	float * r = s->termR;
	if( !r ) r = s->termR = &s->sim->compRes[s->termRindex];
	*r = 1.0/(1.0/fetR + 1.0/rDiode);
}

int AddFET( ckt_sim * sim, component * c, const char * type )
{
	// FETs are a combination of several components:
	//        Crss
	//   +----| |----------+     +----+
	//   |                 |     |    |
	//   |             |   |     |    /
	//   |  RG         |---+--+--+    \
	//  -+-/\/\--------|      = Coss  / SDR
	//   |             |---+--+--+    \
	//   |             |   |     |    |
	//   |    Ciss         |     |----+
	//   +----| |----------+     
	//

	if( c->numnets != 3 )
	{
		fprintf( stderr, "Error: FET has wrong number of nets\n" );
		return -5;
	}

	struct semicomp_nfet_t * sc = AddSuperComp( sim, sizeof( struct semicomp_nfet_t ) );
	int mcompGS = sc->mcompGS = AddMetaComp( sim );
	int mcompSD = sc->mcompSD = AddMetaComp( sim );
	int mcompSDR = sc->mcompSDR = AddMetaComp( sim );
	int mcompDG = sc->mcompDG = AddMetaComp( sim );

	int * mcompGSterms = &sim->compTerms[mcompGS*2];
	int * mcompSDterms = &sim->compTerms[mcompSD*2];
	int * mcompSDRterms = &sim->compTerms[mcompSDR*2];
	int * mcompDGterms = &sim->compTerms[mcompDG*2];

	sc->gateCur  = Rand01() * 1.e-12; //in 0-1 pC

	// This is a little strange - we are chating here, and simulating
	// a RC falloff here.
	// RG, and Ciss are our time constant calculations.
	sc->gateTau = 1.0-exp(-tdelta/(20 * 15e-12));

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
	mcompSDRterms[0] = nS;
	mcompSDRterms[1] = nD;
	mcompDGterms[0] = nD;
	mcompDGterms[1] = nG;

	sim->compCaps[mcompGS] = 15.e-12; // ciss
	sim->compRes[mcompGS] = 10; //RG

	sim->compCaps[mcompDG] = 3.e-12; // crss
	sim->compRes[mcompDG] = 10;

	sim->compCaps[mcompSD] = 10.e-12; // coss (Averaged across the range)
	sim->compRes[mcompSD] = 1; //RG

	sim->compCaps[mcompSDR] = -1; // Between drain and source
	sim->compRes[mcompSDR] = 1e6;

	sc->termRindex = mcompSDR;

	sc->sim = sim;

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



struct semicomp_led_t
{
	void (*cb)( struct semicomp_led_t * s );

	float * vAnnode;
	float * vCathode;
	float vF;
	float rInflex;

	int compRindex;
	float * termR;

	int mcomp;

	ckt_sim * sim;
};


void LEDCB( struct semicomp_led_t * s )
{
	float dF = *s->vAnnode - *s->vCathode;
	float vF = s->vF;

	dF -= vF;

	float * r = s->termR;
	if( !r ) r = s->termR = &s->sim->compRes[s->compRindex];

	float rout = 0.0;

	if( dF < 0.00001 )
		rout = 10.e6;
	else
		rout = s->rInflex/dF;
	if( rout < 1.0 ) rout = 1.0;
	*r = rout;
}

int AddDiode( ckt_sim * sim, component * c, const char * type, int is_led )
{
	struct semicomp_led_t * sc = AddSuperComp( sim, sizeof( struct semicomp_led_t ) );

	sc->cb = LEDCB;

	int nA, nC;
	if( is_led )
	{
		nA = c->nets[1];
		nC = c->nets[0];
	}
	else
	{
		nC = c->nets[1];
		nA = c->nets[0];
	}

	int mcomp = sc->mcomp = AddMetaComp( sim );
	int mcompCap = sc->mcomp = AddMetaComp( sim );

	int * compterms = &sim->compTerms[mcomp*2];
	compterms[0] = nA;
	compterms[1] = nC;
	int * comptermsCap = &sim->compTerms[mcompCap*2];
	comptermsCap[0] = nA;
	comptermsCap[1] = nC;

	sim->compRes[mcompCap] = 1;
	sim->compCaps[mcompCap] = 5.e-12;

	if( is_led )
	{
		sc->vF = 1.6;
		sc->rInflex = 20;
	}
	else
	{
		sc->vF = .5;
		sc->rInflex = 0.5;
	}

	sc->vAnnode = &sim->nodeVoltages[nA];
	sc->vCathode = &sim->nodeVoltages[nC];
	sc->compRindex = mcomp;
	sc->sim = sim;
	return 0;
}






struct semicomp_inductor_t
{
	void (*cb)( struct semicomp_inductor_t * s );

/*
	float * vAnnode;
	float * vCathode;
	float vF;
	float rInflex;

	int compRindex;
	float * termR;
*/
	float h;
	float q;
	float isat;
	int node1;
	int node2;
	ckt_sim * sim;
};


void InductorCB( struct semicomp_inductor_t * s )
{
	ckt_sim * sim = s->sim;
	int n1 = s->node1;
	int n2 = s->node2;
	float nv1 = sim->nodeVoltages[n1];
	float nv2 = sim->nodeVoltages[n2];

	float dQ = ( nv1 - nv2 ) * tdelta / s->h;
	float q = s->q + dQ;
	float sisat = s->isat;
	if( q > sisat ) q = sisat;
	if( q <-sisat ) q =-sisat;
	s->q = q;
	sim->nodeCurrents[n1] -= q;
	sim->nodeCurrents[n2] += q;
}

int AddInductor( ckt_sim * sim, component * c )
{
	struct semicomp_inductor_t * sc = AddSuperComp( sim, sizeof( struct semicomp_inductor_t ) );

	sc->cb = InductorCB;
	sc->node1 = c->nets[0];
	sc->node2 = c->nets[1];
	sc->isat = 1; // in amps.

	if( ProcessEngineeringNumber( c->type, &sc->h ) )
	{
		fprintf( stderr, "Invalid number on component %s\n", c->id );
		return -5;
	}

	sc->q = Rand01() * 1.e-12;

	sc->sim = sim;
	return 0;
}


////////////////////////////////////////////////////////////////////////////////

int AddMetaComp( ckt_sim * s )
{
	int cid = s->numComps;
	int cp1 = cid + 1;
	s->numComps = cp1;
	s->compCaps = realloc( s->compCaps, cp1 * sizeof( float ) );
	s->compRes = realloc( s->compRes, cp1 * sizeof( float ) );
	s->compVDiff = realloc( s->compVDiff, cp1 * sizeof( float ) );
	s->compVDiffResidue = realloc( s->compVDiffResidue, cp1 * sizeof( float ) );
	s->compTerms = realloc( s->compTerms, cp1 * 2 * sizeof( int ) );

	s->compCaps[cid] = -1;
	s->compRes[cid] = 1;
	s->compVDiff[cid] = Rand01() * 1.e-6;
	s->compVDiffResidue[cid] = Rand01() * 1.e-6;
	s->compTerms[cid*2+0] = -1;
	s->compTerms[cid*2+1] = -1;

	return cid;
}

int main( int argc, char ** argv )
{
	int c;
	int n;
	int i;

	if( argc != 2 )
	{
		fprintf( stderr, "Error: Invalid usage: [tool] [.cir file]\n" );
		return -5;
	}

	const char * cirFilename = argv[1];

	cir_reader cktfile = { 0 };
	ckt_sim sim = { 0 };
	cnrbtree_strint  * testpoints = cnrbtree_strint_create();
	FILE * fTestPoints = fopen( "timeseries.csv", "w" );
	FILE * f = fopen( cirFilename, "r" );
	if( !f )
	{
		fprintf( stderr, "Error: can't open file\n" );
		return -5;
	}
	fseek( f, 0, SEEK_END );
	int len = ftell( f );
	fseek( f, 0, SEEK_SET );
	char * buffer = malloc( len + 1 );
	if( fread( buffer, len, 1, f ) != 1 )
	{
		fprintf( stderr, "Error reading file\n" );
		return -6;
	}
	buffer[len] = 0;
	int r = CircuitLoad( &cktfile, buffer );
	if( r )
	{
		printf( "Circuit Load failed: %d\n", r );
		return r;
	}

	printf( "Loaded %d nets\n", cktfile.numNets );
	printf( "Loaded %d components\n", cktfile.numComponents );

	int numNodes = sim.numNodes = cktfile.numNets;
	sim.nodeCaps = calloc( sizeof( float ), numNodes );
	sim.nodeVoltages = calloc( sizeof( float ), numNodes );
	sim.nodeCurrents = calloc( sizeof( float ), numNodes );


	for( i = 0; i < numNodes; i++ )
	{
		sim.nodeCaps[i] = 5.e-13;
		sim.nodeVoltages[i] = Rand01() * 1.e-12;
		sim.nodeCurrents[i] = Rand01() * 1.e-12;
	}

	sim.numComps = 0;
	int numUnknown = 0;

	for( i = 0; i < cktfile.numComponents; i++ )
	{
		component * c = &cktfile.components[i];
		model * m = &RBA( cktfile.modelmap, c->type );
		char * par0 = m->pars[0];
		char * par1 = m->pars[1];
		char * cid = cktfile.components[i].id;
		char * type = cktfile.components[i].type;
		//printf( "\t%s; %s; %s\n", cid, c->type, par1 );
		if( cid[0] == 'R' )
		{
			// A raw resistor.
			int rnum = AddMetaComp( &sim );
			sim.compCaps[rnum] = -1;
			if( ProcessEngineeringNumber( c->type, &sim.compRes[rnum] ) )
			{
				return -5;
			}

			int * compterms = &sim.compTerms[rnum*2];

			compterms[0] = c->nets[0];
			compterms[1] = c->nets[1];

			//printf( "\t\tAdded R %f\n", sim.compRes[rnum] );
		}
		else if( cid[0] == 'C' )
		{
			int cnum = AddMetaComp( &sim );
			sim.compRes[cnum] = 0.1;
			if( ProcessEngineeringNumber( c->type, &sim.compCaps[cnum] ) )
			{
				return -5;
			}

			int * compterms = &sim.compTerms[cnum*2];

			compterms[0] = c->nets[0];
			compterms[1] = c->nets[1];
		}
		else if( cid[0] == 'L' )
		{
			if( AddInductor( &sim, c ) ) return -8;
		}
		else if( cid[0] == 'D' )
		{
			if( AddDiode( &sim, c, par1, 0 ) ) return -6;
		}
		else if( par1 && strcmp( par0, "VDMOS" ) == 0 )
		{
			if( AddFET( &sim, c, par1 ) ) return -5;
		}
		else if( strncmp( cid, "LED", 3 ) == 0 )
		{
			if( AddDiode( &sim, c, par1, 1 ) ) return -6;
		}
		else if( strncmp( cid, "TP", 2 ) == 0 )
		{
			//printf( "TP: ID:%s TYPE:%s %d\n", c->id, c->type, c->nets[0] );
			RBA( testpoints, cktfile.netNames[c->nets[0]] ) = c->nets[0];
		}
		else
		{
			printf( "Unknown component CID:%s TYPE:%s PAR0:%s\n", cid, type, par0 );
			numUnknown++;
		}
	}

	if( numUnknown )
		printf( "Unknown components %d\n", numUnknown );

	printf( "Metacomponents: %d\n", sim.numComps );

	for( c = 0; c < sim.numComps; c++ )
	{
		double delta = tdelta / 1.9 / sim.compRes[c];
		int n1 = sim.compTerms[c*2+0];
		int n2 = sim.compTerms[c*2+1];
		//printf( "%d %d %d\n", c, n1, n2 );
		sim.nodeCaps[n1] += delta;
		sim.nodeCaps[n2] += delta;
	}

	float mintargetnodecap = 1.0e-12; // Target capacitance for node (tell simulator not to go below this, even if it would be numerically stable.)

	for( n = 0; n < sim.numNodes; n++ )
	{
		if( sim.nodeCaps[n] < mintargetnodecap )
			sim.nodeCaps[n] = mintargetnodecap;
	}


	if( !RBHAS( cktfile.netmap, "VDD" ) )
	{
		fprintf( stderr, "Error: Need VDD node\n" );
		return -9;
	}

	if( !RBHAS( cktfile.netmap, "GND" ) )
	{
		fprintf( stderr, "Error: Need GND node\n" );
		return -10;
	}

	int vddNode = RBA( cktfile.netmap, "VDD" );
	int gndNode = RBA( cktfile.netmap, "GND" );

	int numTestPoints = 0;
	fprintf( fTestPoints, "Time (ns),");
	RBFOREACH( strint, testpoints, i )
	{
		if( numTestPoints != 0 ) fprintf( fTestPoints,"," );
		fprintf( fTestPoints, "%s", i->key );
		numTestPoints++;
	}
	fprintf( fTestPoints, "\n" );

	int iteration;
	for( iteration = 0; iteration < 4000000; iteration++ )
	{
		sim.nodeVoltages[gndNode] = 0;
		sim.nodeVoltages[vddNode] = 3.3;

		for( n = 0; n < sim.numNodes; n++ )
		{
			sim.nodeCurrents[n] = 0;
		}

		for( c = 0; c < sim.numSuperComps; c++ )
		{
			struct supercomp_t * sc = sim.superComps[c];
			sc->cb( sc );
		}

		for( c = 0; c < sim.numComps; c++ )
		{
			int n0 = sim.compTerms[c*2+0];
			int n1 = sim.compTerms[c*2+1];
			float v0 = sim.nodeVoltages[n0];
			float v1 = sim.nodeVoltages[n1];
			float cdiff = sim.compVDiff[c];

			// Optional: Capacitor Residue, to allow it
			float cdiffresidue = sim.compVDiffResidue[c];
			float vDiff = v1 - v0 + cdiff;

			float cv = sim.compCaps[c];
			float rv = sim.compRes[c];

			float deltaI = vDiff / rv;
			sim.nodeCurrents[n0] += deltaI/2;
			sim.nodeCurrents[n1] -= deltaI/2;

			float cdiffout = cdiff;
			float vdiff = deltaI / cv * tdelta;
			if( cv > 0 )
				cdiffout = cdiffout - vdiff;

			// Optional: Simulate with residue on capacitors.
			// This makes it so adjustments don't get lost but charge accumulates.
			cdiffout -= cdiffresidue;
			float deltaresidue = (cdiffout - cdiff) + vdiff;
			sim.compVDiffResidue[c] = cdiffresidue + deltaresidue;

			sim.compVDiff[c] = cdiffout;
		}
		
		for( n = 0; n < sim.numNodes; n++ )
		{
			float nv;
			sim.nodeVoltages[n] = nv = sim.nodeVoltages[n] + sim.nodeCurrents[n] / (sim.nodeCaps[n]) * tdelta;
		}

		if( ( iteration % 1000 ) == 0 )
		{
			// Only output every 10 steps
			int n = 0;
			fprintf( fTestPoints, "%f,", iteration*tdelta*1000000000 );
			RBFOREACH( strint, testpoints, i )
			{
				fprintf( fTestPoints, "%f%c", sim.nodeVoltages[i->data], (n == numTestPoints-1) ? '\n' : ',' );
				n++;
			}
		}
	}


	return 0;

}

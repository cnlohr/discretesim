#include <stdio.h>
#include <math.h>

int main()
{
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
}

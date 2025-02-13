#include <stdio.h>
#include <math.h>

int main()
{
	// Testing for a setup like:
	//   (R+C) for all capacitors.
	//   Possibly (R1||(R2+C))
	// This method works well.

	float tdelta = 1.0e-12;
	float mintargetnodecap = 1.0e-12;

	#define NODES 5
	#define COMPS 2
	float NodeVoltages[NODES] = { 0 };
	float ComponentCurrents[COMPS] = { 0 };

	// Instantaneous I deltas
	float NodeIs[NODES] = { 0 };

	const float ComponentRs[COMPS] = { 1, 100 };
	const float ComponentCs[COMPS] = { 100.e-12, -1 };
	float ComponentVDiff[COMPS] = { 0 };
	int ComponentTerms[COMPS][2] = {
		{ 0, 1 },
		{ 3, 4 },
	};
	float NodeCaps[NODES] = { 0 };
	
	int c;
	NodeCaps[0] = 10000;
	NodeCaps[1] = 2.e-12;
	NodeCaps[2] = 10000;
	NodeCaps[3] = 2.e-12;
	NodeCaps[4] = 10000;
	
	int n;
	for( n = 0; n < NODES; n++ )
	{
		if( NodeCaps[n] < mintargetnodecap )
			NodeCaps[n] = mintargetnodecap;
		//printf( "NC %d %f\n", n, NodeCaps[n]*1.e12 );
	}
	
	float MOSFetGateCharge = 0.0;
	float MOSFetVSD = 0.0;

	int iteration;
	for( iteration = 0; iteration < 2000; iteration++ )
	{
		NodeVoltages[0] = (sin(iteration/200.0-1) * 2.50) > 0.0 ? 5.0 : 0.0;
		NodeVoltages[2] = 0.0;
		NodeVoltages[4] = 5.0;
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
		
		// Emulate mosfet.
		{
			int nG = 1;
			int nS = 2;
			int nD = 3;
			float vG = NodeVoltages[nG];
			float vS = NodeVoltages[nS];
			float vD = NodeVoltages[nD];
			float gQ = MOSFetGateCharge;

			const float cISS = 15.e-12;
			const float cOSS = 15.e-12;
			const float cRSS = 15.e-12; // TODO.
			const float vGSth = 1.6;
			const float rDSOn = 5;
			const float rG = 2;

			// Computed based on gQ.
			float vGS = gQ / cISS;
			float rFET = 1.e6;
			
			// Pretend: Pretend the miller plateau is nearly flat.
			if( vGS > vGSth )
			{
				const float plateauSize = vGSth;
				float vGSTmp = (vGS - vGSth);

				float percentOn = vGSTmp / plateauSize;
				if( percentOn > 1 ) percentOn = 1;
				//printf( "%f %f %f\n", vGSTmp, plateauSize, percentOn );
				rFET = rFET * (1-percentOn) + rDSOn;

				if( vGSTmp < plateauSize )
				{
					vGSTmp = 0;
				}
				else
				{
					vGSTmp -= plateauSize;
				}
				vGS = vGSth + vGSTmp;
			}
			
			float rVDSCapR = 1;
			float iSDCap = (vD - vS - MOSFetVSD)/rVDSCapR;
			float iSD = iSDCap + (vD-vS)/rFET;
			float iG = (vG - vS - vGS) / rG;
			printf( "%f,%f,%f,%f\n", vG, vS, gQ / cISS, vGS );
			//printf( "%f,%f,%f,%f,%f,%f\n", vD, vS, MOSFetVSD, rFET, iSD, (vD-vS)/rFET );
			//printf( "%f-%f VGS: %f (%f) ->rFET %f  VDS: %f\n", vG, vS, vGS, iG, rFET, vD - vS );
			
			MOSFetGateCharge += iG * tdelta;
			MOSFetVSD += iSDCap / cOSS * tdelta;
			
			NodeIs[nS] += iG/2;
			NodeIs[nG] -= iG/2;
			NodeIs[nS] += iSD/2;
			NodeIs[nD] -= iSD/2;
		}
		
		
		for( n = 0; n < NODES; n++ )
		{
			float nv;
			NodeVoltages[n] = nv = NodeVoltages[n] + NodeIs[n] / (NodeCaps[n]) * tdelta;
			//if( (iteration % 100) == 0)
			//printf( "%f,%f,+%c", nv, NodeIs[n], (n == NODES-1) ? '\n':',' );
		}
		//printf( "\n" );

	}
}

#include <stdio.h>

int main()
{
	// Testing for a setup like:
	//   (R+C) for all capacitors.
	//   Possibly (R1||(R2+C))

	#define NODES 6
	#define COMPS 5
	float NodeVoltages[NODES] = { 0 };
	float ComponentCurrents[COMPS] = { 0 };

	float NodeIs[NODES] = { 0 };

	const float ComponentRs[COMPS] = { 100, 1, 1, 1, 1000 };
	const float ComponentCs[COMPS] = { -1.e-12, 15.e-12, 35.e-12, 1000.e-12, -1 };
	const float ComponentVDiff[COMPS] = { 0 };
	int ComponentTerms[COMPS][2] = {
		{ 0, 1 },
		{ 1, 2 },
		{ 2, 4 },
		{ 2, 3 },
		{ 3, 5 } };
		

	int iteration;
	float tdelta = 1.e-12;
	for( iteration = 0; iteration < 100; iteration++ )
	{
		NodeVoltages[0] = (sin(iteration/10.0)>0) ? 5.0 : 0.0;
		NodeVoltages[4] = 0.0;
		NodeVoltages[5] = 5.0;
		int n;
		for( n = 0; n < NODES; n++ )
		{
			NodeIs[n] = 0;
		}

		int c;
		for( c = 0; c < COMPS; c++ )
		{
			int n0 = ComponentTerms[c][0];
			int n1 = ComponentTerms[c][1];
			float v0 = NodeVoltages[n0];
			float v1 = NodeVoltages[n1];
			float vDiff = v1 - v0 + ComponentCharge[c];

			float cv = ComponentCs[c];
			float rv = ComponentRs[c];

			float deltaI = vDiff / rv;
			NodeIs[n0] += deltaI/2;
			NodeIs[n1] -= deltaI/2;
		}

		for( c = 0; c < COMPS; c++ )
		{
			
		}
	}
}

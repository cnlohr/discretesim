#ifndef _YSPICE_H
#define _YSPICE_H

typedef struct yspice_sim_t
{
	//int     nNets;
	//void * arena;
	//int    arenalen;
	//float * vVoltages;
	//float * vCurrents;
} yspice_sim;

int YSpiceLoad( yspice_sim * s, char * cir );

#endif


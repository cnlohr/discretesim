#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "yspice.h"

int main()
{
	FILE * f = fopen( "schtest/schtest.cir", "r" );
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
	
	yspice_sim sim = { 0 };
	int r = YSpiceLoad( &sim, buffer );
	free( buffer );

	return 0;
}


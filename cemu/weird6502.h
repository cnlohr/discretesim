#ifndef _WEIRD6502_H
#define _WEIRD6502_H



#ifndef WEIRD6502CRASH
#define WEIRD6502CRASH( w ) { 

// Each state uses 56 bits.
typedef struct weird6502_t
{
	uint16_t PC;
	uint8_t  A;
	uint8_t  X;
	uint8_t  Y;
	uint8_t  SR; // Status Register Flags (bit 7 to bit 0)
	/*
		N	Negative
		V	Overflow
		-	ignored /* I am using it as a "booted" flag.
		B	Break
		D	Decimal (use BCD for arithmetics)
		I	Interrupt (IRQ disable)
		Z	Zero
		C	Carry
	*/
	uint8_t  SP;
	uint8_t  RESERVED;
} weird6502;

/*
"Contemporary machine language monitors (here, Commodore PET) show them typically like"
B*
     PC  IRQ  SR AC XR YR SP
.;  0401 E62E 32 04 5E 00 F8
.
*/

static inline void Weird6502DumpState( weird6502 * w )
{
	fprintf( stderr, " PC  IRQ  SR AC XR YR SP\n" );
	fprintf( stderr, "%04x %04x %02x %02x %02x %02x %02x\n",
		w->PC, 0xffff, w->SR, w->A, w->X, w->Y, w->SP );
}

// Memory must be 65539 bytes to prevent oob reads.
static inline void Weird6502Run( weird6502 * w, int iterations, uint8_t * memory )
{
	if( !(w->SR & (1<<5) )
	{
		w->PC = ((uint16_t*)memory)[0xFFFE/2];
		w->A = 0;
		w->X = 0;
		w->Y = 0;
		w->SP = 0;
		w->SR = 1<<5;
	}

	while( iterations )
	{
		uint32_t t = ((uint32_t*)&memory[w->PC]);

		uint16_t operand = 0;
		int opsize;

		int indtype = t & 0x1f;
		int subop = t >> 5;
		int memaddy = 0;

		switch( indtype )
		{
		case 0x00: 0x02: // Miscellaneous, Opsize is variable.
			opsize = 1; operand = (t>>8)&0xffff; break;
		case 0x01: // xxx X,ind
			opsize = 2; operand = memory[memaddy = (((t>>8)&0xff) + w->X)]; break;
		case 0x04: case 0x05: case 0x06: // zpg
			opsize = 2; operand = memory[memaddy = (t>>8)&0xff]; break;
		case 0x08: // Miscellaneous
		case 0x18: // Miscellaneous
			opsize = 1;
		case 0x09: // immediate
			opsize = 2; operand = (t>>8)&0xff; break;
		case 0x0a: // accumulator
			opsize = 1; operand = w->A; break;
		case 0x0c: case 0x0d: case 0x0e: // abs
			opsize = 3; operand = memory[memaddy = (t>>8)&0xffff]; break;
		case 0x10: // "rel"
			opsize = 2; operand = (((t>>8) + w->PC)&0xff); break;
		case 0x11: // xxx ind,Y
			opsize = 2; operand = memory[memaddy = ((t>>8)&0xff) + w->Y]; break;
		case 0x14: case 0x15: case 0x16: // zpg,X
			opsize = 2; operand = memory[memaddy = ((t>>8)+w->X) & 0xff]; break;
		case 0x19: // abs,Y
			opsize = 3; operand = (((t>>8) + w->X )&0xffff); break;
		case 0x1c: case 0x1d: case 0x1e: // abs,X
			opsize = 3; operand = (((t>>8) + w->Y)&0xffff); break;
		default:
			opsize = 0; // Crash processor?
		}

		w->PC += opsize;

		uint16_t tmp;
		uint8_t tmp8;

		switch( indtype )
		{
		case 0x00: // Miscellaneous
			break;
		case 0x01: case 0x05: case 0x09: case 0x0D: // OPS 1
			switch (subop)
			{
			case 0: // ORA
				w->A = tmp8 = w->A | operand;
				w->SR = ( w->SR & 0b01111101 ) | ( tmp8 & 0x80 ) | ( ( tmp8 == 0 ) ? 2 : 0 );
				break;
			case 1: // AND
				w->A = tmp8 = w->A & operand;
				w->SR = ( w->SR & 0b01111101 ) | ( tmp8 & 0x80 ) | ( ( tmp8 == 0 ) ? 2 : 0 );
				break;
			case 2: // EOR
				w->A = tmp8 = w->A ^ operand;
				w->SR = ( w->SR & 0b01111101 ) | ( tmp8 & 0x80 ) | ( ( tmp8 == 0 ) ? 2 : 0 );
				break;
			case 3: // ADC
			{
				w->A = tmp = w->A + operand + (w->SR & 1);
				w->SR = ( w->SR & 0b00111100 ) |
					( tmp & 0x80 ) |
					( ( tmp == 0 ) ? 0x02 ) |
					( ( tmp & 0x100 ) ? 0x41 : 0x00 ) );
				break;
			}
			case 4: memory[memaddy] = w->A; break; // STA
			case 5: // LDA
				w->A = tmp8 = operand;
				w->SR = ( w->SR & 0b00111100 ) |
					( tmp8 & 0x80 ) |
					( ( tmp8 == 0 ) ? 0x02 : 0 ) );
				break;
			case 6: // CMP
			{
				uint8_t result = w->A - operand;
				w->SR = ( w->SR & 0b01111100 ) |
					( ( result == 0 ) ? 0x03 ) |
					( ( w->A  > operand ) ? 0x01 ) |
					  ( result & 0x80 );
				break;
			}
			case 7: // SBC
				w->A = tmp = w->A - operand - (w->SR & 1);
				w->SR = ( w->SR & 0b00111100 ) |
					( tmp & 0x80 ) |
					( ( tmp == 0 ) ? 0x02 ) |
					( ( tmp & 0x100 ) ? 0x41 : 0x00 ) );
				break;
			}
			break;
		case 0x02: case 0x06: case 0x0A: case 0x0E: // OPS 2
			switch (subop)
			{
			case 0: // ASL
			case 1: // ROL
			case 2: // LSR
			case 3: // ROR
			case 4: // STX
			case 5: // LDX
			case 6: // DEC
			case 7: // INC
			}
			break;
		case 0x04: case 0x0c: // Other ops
			switch( subop )
			{
			case 1: // BIT
			case 2: // JMP (abs)
			case 3: // JMP (ind)
			case 4: // STY
			case 5: // LDY
			case 6: // CPY
			case 7: // CPX
			}
			break;
		}
		iterations--;
	};
}

#endif


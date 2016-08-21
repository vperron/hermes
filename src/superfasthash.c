/**
 * =====================================================================================
 *
 *   @file superfasthash.c
 *
 *   
 *        Version:  1.0
 *        Created:  01/03/2013 06:12:29 PM
 *
 *
 *   @section DESCRIPTION
 *
 * SuperFastHash : courtesy of Paul Hsieh,
 * http://www.azillionmonkeys.com/qed/hash.html
 *       
 *   @section LICENSE
 * 
 *  LGPL v2.1
 *       
 *
 * =====================================================================================
 */


#include "main.h"
#include "superfasthash.h"

#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#define get32bits(d) (*((const uint32_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#define get32bits(d) ((((uint32_t)(((const uint8_t *)(d))[3])) << 24)\
											+(((uint32_t)(((const uint8_t *)(d))[2])) << 16)\
											+(((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                      +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif

uint32_t SuperFastHash (const uint8_t* data, int len, uint32_t seed) 
{
	uint32_t tmp;
	int rem;

	if (len <= 0 || data == NULL) return 0;

	rem = len & 3;
	len >>= 2;

	/* Main loop */
	for (;len > 0; len--) {
		seed  += get16bits (data);
		tmp    = (get16bits (data+2) << 11) ^ seed;
		seed   = (seed << 16) ^ tmp;
		data  += 2*sizeof (uint16_t);
		seed  += seed >> 11;
	}

	/* Handle end cases */
	switch (rem) {
		case 3: seed += get16bits (data);
						seed ^= seed << 16;
						seed ^= ((signed char)data[sizeof (uint16_t)]) << 18;
						seed += seed >> 11;
						break;
		case 2: seed += get16bits (data);
						seed ^= seed << 11;
						seed += seed >> 17;
						break;
		case 1: seed += (signed char)*data;
						seed ^= seed << 10;
						seed += seed >> 1;
	}

	/* Force "avalanching" of final 127 bits */
	seed ^= seed << 3;
	seed += seed >> 5;
	seed ^= seed << 4;
	seed += seed >> 17;
	seed ^= seed << 25;
	seed += seed >> 6;

	return seed;
}

char *hex2string (const unsigned char *hex, int len)
{
	char hex_char [] = "0123456789abcdef";
	char *string = malloc (len * 2 + 1);
	memset(string,0,len*2+1);
	int byte_nbr;
	for (byte_nbr = 0; byte_nbr < len; byte_nbr++) {
		string [byte_nbr * 2 + 0] = hex_char [hex [byte_nbr] >> 4];
		string [byte_nbr * 2 + 1] = hex_char [hex [byte_nbr] & 15];
	}
	return string;
}

char* StrSuperFastHash(const uint8_t* data, int len, uint32_t seed)
{
  uint32_t hash = SuperFastHash(data, len, seed);
  return hex2string((unsigned char*)&hash, sizeof(uint32_t));
}

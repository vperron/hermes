/**
 * =====================================================================================
 *
 *   @file superfasthash.h
 *
 *   
 *        Version:  1.0
 *        Created:  01/03/2013 06:14:32 PM
 *
 *
 *   @section DESCRIPTION
 *
 *       
 *       
 *   @section LICENSE
 *
 *       
 *
 * =====================================================================================
 */

#include <stdint.h>

#ifndef _SATAN_SUPERFASTHASH_H_
#define _SATAN_SUPERFASTHASH_H_ 

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

uint32_t SuperFastHash (const uint8_t* data, int len, uint32_t seed);

char *hex2string (const unsigned char *hex, int len);

char* StrSuperFastHash (const uint8_t* data, int len, uint32_t seed);

#endif // _SATAN_SUPERFASTHASH_H_

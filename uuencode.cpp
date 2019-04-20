/****************************************************************
*                                                               * 
* Sample UUENCODE and UUDECODE routines.                        * 
* Copyright 1995, Socket Communications.                        * 
*                                                               * 
* These routines may be freely copied and distributed.          *
*                                                               * 
****************************************************************/
/*slightly modified by chitchat, 3/20/2000 */
#include <config.h>
#include "tintin.h"

#if defined(HAVE_STRING_H)
#include <string.h>
#elif defined(HAVE_STRINGS_H)
#include <strings.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#define SPACE 0x20   /* An ASCII space character */ 
#define LOWMASK 0x3f /* Mask for low bits of each byte */
#define HIMASK 0xc0  /* Mask for high bits of each byte */
#define SUCCESS 0    /* Successful encode/decode */ 
#define FAILURE -1   /* Failure encodeing/decoding */
#define CRYPT_KEY "yTin1.80"

#include "utils.proto"
#include "uuencode.proto"
/****************************************************************
 *                                                              * 
 * UUDECODE takes a string of encoded ASCII characters and      * 
 * performs the decoding function to recover the original input.*
 *                                                              * 
 * bin points to a buffer to hold the decoded output.           *
 * str points to the UUENCODED input (usually null terminated). * 
 * size is the number of characters in the input string.        *
 *                                                              * 
 * The size of the buffer needed to hold the decoded input can  * 
 * be calculated as:                                            * 
 * (size/4)*3 + (((size%4)) > 1?(size%4)-1:0) + 1            *
 *                                                              * 
 * NOTE: This UUDECODE routine places a terminating null at the * 
 * end of the decoded output, in case the original input        *
 * was a null terminated string.                                * 
 *                                                              * 
 ****************************************************************/
int uudecode (char *bin, int bsize, char *str, int ssize)
  { 
  int triplets, remainder; 
  unsigned char composite; 
  int ntriplet, j; 
  int required; 
  triplets = ssize / 4; 
  remainder = ssize % 4; 
  /* 
   * Calculate the number of bytes needed in the binary
   * decoded buffer. 
   */ 
  required = 3 * triplets; 
  if (remainder) 
      required += remainder - 1; 
  required++; 
  if (required > bsize) 
      return (FAILURE); 
  /* 
   * Decode each of the triplets. 
   */ 
  for (ntriplet = 0; ntriplet < triplets; 
                ntriplet++, str++) 
      { 
      for (j = 2, composite = *(str + 3) - SPACE; 
           j < 7; j += 2, bin++, str++) 
          *bin = (*str - SPACE) | 
                 ((composite & (HIMASK >> j)) << j); 
      } 
  /* 
   * Decode any dangling bytes (0,1, or 2 bytes which 
   * did not form a complete triplet). 
   */ 
  if (remainder > 1) 
      { 
      remainder--; 
      for (j = 2, composite = *(str + remainder) - SPACE; 
           j <  (2 * remainder + 1); 
           j += 2, bin++, str++) 
        *bin = (*str - SPACE) | 
               ((composite & (HIMASK>>j)) << j); 
      } 
  *bin = 0; 
  return (SUCCESS); 
  }
 
/****************************************************************
 *                                                              * 
 * UUENCODE takes a pointer to any binary data and encodes it   * 
 * into printable ASCII characters.                             * 
 *                                                              * 
 * bin points to the data to be encoded.                        * 
 * str points to a buffer to hold the encoded output.           *
 * size is the number of bytes from the input to be encoded.    * 
 *                                                              * 
 * The size of the buffer needed to hold the encoded data can   * 
 * be calculated as:                                            * 
 * (size/3)*4 + ((size%3) ? (size%3)+1:0) + 1                   * 
 *                                                              * 
 * NOTE: This UUENCODE routine places a terminating null at the * 
 * end of the encoded output.                                   * 
 *                                                              * 
 ****************************************************************/
int uuencode (char *bin, int bsize, char *str, int ssize)
  { 
  int triplets, remainder; 
  unsigned char composite; 
  int ntriplet, j; 
  int required; 
  triplets = bsize / 3; 
  remainder = bsize % 3; 
  /* 
   * Calculate the number of bytes required in the 
   * ascii string buffer. 
   */ 
  required = triplets * 4; 
  if (remainder) 
      required += remainder + 1; 
  required++; 
  if (required > ssize) 
      return (FAILURE); 
  /* 
   * Encode each of the triplets. 
   */ 
  for (ntriplet = 0; ntriplet < triplets; 
        ntriplet++, str++) 
      { 
      for (j = 2, composite = 0; j < 7; j += 2, bin++, str++)
          { 
          *str = (*bin & LOWMASK) + SPACE; 
          composite |= (*bin & HIMASK) >> j; 
          } 
      *str = composite + SPACE; 
      } 
  /* 
   * Encode any dangling bytes (0,1, or 2 bytes which 
   * do not form a complete triplet). 
   */ 
  if (remainder) 
      { 
      for (j = 2, composite = 0; j < 2 * remainder + 1; 
                 j += 2, bin++, str++) 
          { 
          *str = (*bin & LOWMASK) + SPACE; 
          composite |= (*bin & HIMASK) >> j; 
          } 
      *str++ = composite + SPACE; 
      } 
  *str = 0; 
  return (SUCCESS); 
  }

/** chitchat, 3/20/2000
 * please, do not try to hack this, it is a real simple encryption
 * and I did not even hide the KEY and the code
 * it is just a double guard for those poor mudders on the public terminals
 */
/* buffer of encrypt and decrypt need to be free'ed after use */
char *encrypt(char *bin)
{
	char *str;
	unsigned ch;
	int i, ssize, l = strlen(CRYPT_KEY), size = strlen(bin);

	if(size==0) return NULL;
	ssize = (size/3)*4 + ((size%3) ? (size%3)+1:0) + 1;
	if(!(str=(char *)malloc(ssize)) ) return NULL;
	for(i=0; i<size; i++){
		ch = (unsigned char)bin[i] + (unsigned char)CRYPT_KEY[i%l];
		if(ch>=256) ch -= 255;
		bin[i] = (char)ch;
	}
	uuencode(bin, size, str, ssize);
	return str;
}

char *decrypt(char* str)
{
	char* bin;
	unsigned ch;
	int i, bsize, l = strlen(CRYPT_KEY), size = strlen(str);

	bsize = (size/4)*3 + (((size%4)) > 1?(size%4)-1:0) + 1;
	if(!(bin=(char *)malloc(bsize)) ) return NULL;
	uudecode(bin, bsize, str, size);
	bsize = strlen(bin);
	for(i=0; i<bsize; i++){
		if((unsigned char)bin[i]>(unsigned char)CRYPT_KEY[i%l])
			ch = (unsigned char)bin[i]-(unsigned char)CRYPT_KEY[i%l];
		else ch = (unsigned char)bin[i] + 255 - (unsigned char)CRYPT_KEY[i%l];
		bin[i] = (char)ch;
	}
	return bin;
}

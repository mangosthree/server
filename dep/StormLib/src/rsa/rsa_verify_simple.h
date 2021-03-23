/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */
#ifndef __RSA_VERIFY_SIMPLE_H
#define __RSA_VERIFY_SIMPLE_H

#include "tomcrypt.h"

/**
  @file rsa_verify_simple.c
  Created by Ladislav Zezula (zezula@volny.cz) as modification
  for Blizzard strong signature verification
*/

#ifdef LTC_MRSA

/**
  Simple RSA decryption
  @param sig              The signature data
  @param siglen           The length of the signature data (octets)
  @param hash             The hash of the message that was signed
  @param hashlen          The length of the hash of the message that was signed (octets)
  @param stat             [out] The result of the signature comparison, 1==valid, 0==invalid
  @param key              The public RSA key corresponding
  @return Error code
*/
int rsa_verify_simple(const unsigned char *sig,  unsigned long siglen,
                      const unsigned char *hash, unsigned long hashlen,
                            int           *stat,
                            rsa_key       *key);
#endif /* LTC_MRSA */

#endif
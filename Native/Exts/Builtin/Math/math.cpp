/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/EDK/EDK.h>

using namespace Fabric::EDK;
IMPLEMENT_FABRIC_EDK_ENTRIES

// [pzion 20110714] Order of 300772 (mod 2^32) is 1073741824
static const uint32_t randomBase = 300773;

static inline uint32_t randomPower( size_t exp )
{
  uint32_t result = 1;
  uint32_t mult = randomBase;
  for ( size_t mask=1; mask; mask<<=1 )
  {
    if ( exp & mask )
      result *= mult;
    mult *= mult;
  }
  return result;
}

static inline uint32_t randomNext( uint32_t prev )
{
  return prev * randomBase;
}

static uint32_t randomUInt32( size_t id, size_t offset )
{
  return randomPower( 1048576*id + offset );
}

FABRIC_EXT_EXPORT KL::Integer mathRandomInteger( KL::Size id, KL::Size offset )
{
  return KL::Integer( randomUInt32( id, offset ) );    
}

FABRIC_EXT_EXPORT KL::Scalar mathRandomScalar( KL::Size id, KL::Size offset )
{
  return KL::Scalar( randomUInt32( id, offset ) ) / 4294967296.0;
}

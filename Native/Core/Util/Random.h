/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_RANDOM_H
#define _FABRIC_UTIL_RANDOM_H

#include <stdint.h>
#include <stdlib.h> // For size_t

namespace Fabric
{
  namespace Util
  {
    void generateSecureRandomBytes( size_t count, uint8_t *bytes );
  };
};

#endif //_FABRIC_UTIL_RANDOM_H

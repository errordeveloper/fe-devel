/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_MATH_H
#define _FABRIC_UTIL_MATH_H

#include <Fabric/Base/Config.h>

#if defined (FABRIC_WIN32)
# include <math.h>
#else
# include <cmath>
#endif

namespace Fabric
{
  namespace Util
  {
    template<typename T> T nanValue();
    
    template<> inline float nanValue<float>()
    {
      static uint32_t uint32Value = ~uint32_t(0);
      return *reinterpret_cast<float const *>( &uint32Value );
    }

    template<> inline double nanValue<double>()
    {
      static uint64_t uint64Value = ~uint64_t(0);
      return *reinterpret_cast<double const *>( &uint64Value );
    }

    inline bool isinf( float value )
    {
      uint32_t uint32Rep = *reinterpret_cast<uint32_t const *>( &value );
      return (uint32Rep & UINT32_C(0x7FFFFFFF)) == UINT32_C(0x7F800000);
    }

    inline bool isinf( double value )
    {
      uint64_t uint64Rep = *reinterpret_cast<uint64_t const *>( &value );
      return (uint64Rep & UINT64_C(0x7FFFFFFFFFFFFFFF)) == UINT64_C(0x7FF0000000000000);
    }

    inline bool isnan( float value )
    {
#if defined (FABRIC_WIN32)
      return value != value;
#else
      return std::isnan( value );
#endif
    }

    inline bool isnan( double value )
    {
#if defined (FABRIC_WIN32)
      return value != value;
#else
      return std::isnan( value );
#endif
    }
  };
};

#endif //_FABRIC_UTIL_MATH_H

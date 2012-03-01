/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_COMPARABLE_IMPL_H
#define _FABRIC_RT_COMPARABLE_IMPL_H

#include <Fabric/Core/RT/Impl.h>

namespace Fabric
{
  namespace RT
  {
    class ComparableImpl : public Impl
    {
    public:
      REPORT_RC_LEAKS
    
      // Impl
      
      virtual bool equalsData( void const *lhs, void const *rhs ) const;
    
      // ComparableImpl
    
      virtual size_t hash( void const *data ) const = 0;
      virtual int compare( void const *lhsData, void const *rhsData ) const = 0;
    
    protected:
    
      ComparableImpl( std::string const &codeName, ImplType type );
    };
  };
};

#endif //_FABRIC_RT_COMPARABLE_IMPL_H

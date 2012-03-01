/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_SLICED_ARRAY_DESC_H
#define _FABRIC_RT_SLICED_ARRAY_DESC_H

#include <Fabric/Core/RT/ArrayDesc.h>

namespace Fabric
{
  namespace RT
  {
    class SlicedArrayImpl;
    
    class SlicedArrayDesc : public ArrayDesc
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
    
      RC::ConstHandle<RT::SlicedArrayImpl> getImpl() const;

      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;

      void setNumMembers( void *data, size_t numMembers, void const *defaultMemberData ) const;
      
    protected:
    
      SlicedArrayDesc(
        std::string const &userNameBase,
        std::string const &userNameArraySuffix,
        RC::ConstHandle<SlicedArrayImpl> const &slicedArrayImpl,
        RC::ConstHandle<Desc> const &memberDesc
        );
      
    private:
    
      RC::ConstHandle<SlicedArrayImpl> m_slicedArrayImpl;
    };
  }
}

#endif //_FABRIC_RT_SLICED_ARRAY_DESC_H

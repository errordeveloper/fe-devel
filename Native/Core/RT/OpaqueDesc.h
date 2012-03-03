/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_OPAQUE_DESC_H
#define _FABRIC_RT_OPAQUE_DESC_H

#include <Fabric/Core/RT/Desc.h>

namespace Fabric
{
  namespace RT
  {
    class OpaqueImpl;
    
    class OpaqueDesc : public Desc
    {
      friend class Manager;
    
    public:
      REPORT_RC_LEAKS
      
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
      
    protected:
    
      OpaqueDesc(
        std::string const &userNameBase,
        std::string const &userNameArraySuffix,
        RC::ConstHandle<OpaqueImpl> const &opaqueImpl
        );
      
    private:
    
      RC::ConstHandle<OpaqueImpl> m_opaqueImpl;
    };
  }
}

#endif //_FABRIC_RT_OPAQUE_DESC_H

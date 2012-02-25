/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_PRODUCER_IMPL_H
#define _FABRIC_RT_PRODUCER_IMPL_H

#include <Fabric/Core/RT/Impl.h>

namespace Fabric
{
  namespace RT
  {
    class ProducerImpl : public Impl
    {
    public:
      REPORT_RC_LEAKS
    
      // Impl
      
      virtual bool isExportable() const;

    protected:
      
      ProducerImpl( std::string const &codeNamee, ImplType type );
    };
  }
}

#endif //_FABRIC_RT_PRODUCER_IMPL_H

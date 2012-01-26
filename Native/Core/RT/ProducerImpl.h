/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
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
    
      // Impl
      
      virtual bool isExportable() const;

    protected:
      
      ProducerImpl( std::string const &codeNamee, ImplType type );
    };
  }
}

#endif //_FABRIC_RT_PRODUCER_IMPL_H

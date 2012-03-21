/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_SIMPLE_IMPL_H
#define _FABRIC_RT_SIMPLE_IMPL_H

#include <Fabric/Core/RT/ComparableImpl.h>

namespace Fabric
{
  namespace RT
  {
    class SimpleImpl : public ComparableImpl
    {
    public:
      REPORT_RC_LEAKS
    
      // Impl
      
      virtual bool isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const;
    
    protected:
    
      SimpleImpl()
      {
      }

      void initialize(
        std::string const &codeName,
        ImplType type,
        size_t allocSize
        )
      {
        ComparableImpl::initialize(
          codeName,
          type,
          allocSize,
          FlagShallow | FlagExportable
          );
      }
    
      virtual void disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const;
    };
  }
}

#endif //_FABRIC_RT_SIMPLE_IMPL_H

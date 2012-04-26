/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_ARRAY_IMPL_H
#define _FABRIC_RT_ARRAY_IMPL_H

#include <Fabric/Core/RT/Impl.h>

namespace Fabric
{
  namespace RT
  {
    class ArrayImpl : public Impl
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
    
      // Impl
    
      virtual std::string descData( void const *data ) const;
      virtual bool equalsData( void const *lhs, void const *rhs ) const;

      // ArrayImpl

      RC::ConstHandle<Impl> getMemberImpl() const
      {
        return m_memberImpl;
      }
      
      virtual size_t getNumMembers( void const *data ) const = 0;     
      virtual void const *getImmutableMemberData( void const *data, size_t index ) const = 0;
      virtual void *getMutableMemberData( void *data, size_t index ) const = 0;
            
    protected:
    
      ArrayImpl( RC::ConstHandle<Impl> const &memberImpl )
        : m_memberImpl( memberImpl )
      {
      }
      
    private:
    
      RC::ConstHandle<Impl> m_memberImpl;
    };
  }
}

#endif //_FABRIC_RT_ARRAY_IMPL_H

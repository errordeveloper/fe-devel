/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_FIXED_ARRAY_IMPL_H
#define _FABRIC_RT_FIXED_ARRAY_IMPL_H

#include <Fabric/Core/RT/ArrayImpl.h>

namespace Fabric
{
  namespace RT
  {
    class FixedArrayImpl : public ArrayImpl
    {
      friend class Manager;
      friend class Impl;
      
    public:
      REPORT_RC_LEAKS
          
      // Impl
    
      virtual void setData( void const *src, void *dst ) const;
      virtual void disposeDatasImpl( void *data, size_t count, size_t stride ) const;
      virtual void const *getDefaultData() const;
      virtual size_t getIndirectMemoryUsage( void const *data ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;

      virtual bool isEquivalentTo( RC::ConstHandle< RT::Impl > const &desc ) const;
      virtual bool isShallow() const;
      virtual bool isNoAliasSafe() const;
      virtual bool isExportable() const;

      // ArrayImpl
      
      virtual bool isFixedArrayImpl() const { return true; }

      virtual size_t getNumMembers( void const *data ) const;     
      virtual void const *getImmutableMemberData( void const *data, size_t index ) const;
      virtual void *getMutableMemberData( void *data, size_t index ) const;
      
      // FixedArrayImpl
      
      virtual size_t getNumMembers() const;
            
    protected:
    
      FixedArrayImpl( std::string const &codeName, RC::ConstHandle<RT::Impl> const &memberImpl, size_t length );
      ~FixedArrayImpl();
            
      void const *getImmutableMemberData_NoCheck( void const *data, size_t index ) const
      { 
        uint8_t const *members = static_cast<uint8_t const *>( data );
        return &members[m_memberSize * index];
      }
      
      void *getMutableMemberData_NoCheck( void *data, size_t index ) const
      { 
        uint8_t *members = static_cast<uint8_t *>( data );
        return &members[m_memberSize * index];
      }    
      
    private:
    
      RC::ConstHandle<Impl> m_memberImpl;
      size_t m_memberSize;
      size_t m_length;
      void *m_defaultData;
   };
  };
};

#endif //_FABRIC_RT_FIXED_ARRAY_IMPL_H

/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_RT_CONST_STRING_IMPL_H
#define _FABRIC_RT_CONST_STRING_IMPL_H

#include <Fabric/Core/RT/Impl.h>

namespace Fabric
{
  namespace RT
  {
    class ConstStringImpl : public Impl
    {
      friend class Manager;
      friend class Impl;
      
      struct bits_t
      {
        char const *data;
        size_t length;
      };
      
    public:
          
      // Impl
    
      virtual void setData( void const *src, void *dst ) const;
      virtual void disposeDatasImpl( void *data, size_t count, size_t stride ) const;
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      virtual bool equalsData( void const *lhs, void const *rhs ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;

      virtual bool isEquivalentTo( RC::ConstHandle<RT::Impl> const &impl ) const;
      virtual bool isShallow() const;
      virtual bool isNoAliasSafe() const;
      virtual bool isExportable() const;
      
      // ConstStringImpl

      char const *getValueData( void const *src ) const
      {
        bits_t const *bits = static_cast<bits_t const *>( src );
        return bits->data;
      }
      
      size_t getValueLength( void const *src ) const
      {
        bits_t const *bits = static_cast<bits_t const *>( src );
        return bits->length;
      }
      
      std::string toString( void const *data ) const;
            
    protected:
    
      ConstStringImpl( std::string const &codeName );
      ~ConstStringImpl();
   };
  };
};

#endif //_FABRIC_RT_CONST_STRING_IMPL_H

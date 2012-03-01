/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_INTEGER_IMPL_H
#define _FABRIC_RT_INTEGER_IMPL_H

#include <Fabric/Core/RT/SimpleImpl.h>
#include <Fabric/Core/Util/Math.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace RT
  {
    class IntegerImpl : public NumericImpl
    {
    public:
      REPORT_RC_LEAKS
    
      // NumericImpl

      virtual bool isInteger() const { return true; }
      
      // IntegerImpl
      
      virtual bool isSigned() const { return false; }
    
    protected:
    
      IntegerImpl( std::string const &codeName, size_t size )
        : NumericImpl( codeName, DT_INTEGER, size )
      {
      }
    };
    
    template<typename T> class IntegerImplT : public IntegerImpl
    {
    public:
      REPORT_RC_LEAKS

      // Impl
      
      void const *getDefaultData() const
      {
        static T const defaultData = 0;
        return &defaultData;
      }
      
      void setData( void const *src, void *dst ) const
      {
        setValue( getValue( src ), dst );
      }
        
      void encodeJSON( void const *data, JSON::Encoder &encoder ) const
      {
        T value = getValue( data );
        int32_t int32Value = int32_t( value );
        if ( T( int32Value ) != value )
          throw Exception( "value is too large for JSON representation" );
        encoder.makeInteger( int32Value );
      }
      
      void decodeJSON( JSON::Entity const &entity, void *data ) const
      {
        if ( entity.isInteger() )
        {
          int32_t int32Value = entity.integerValue();
          T tValue = T( int32Value );
          if ( int32_t( tValue ) != int32Value )
            throw Exception( "value is out of range" );
          setValue( tValue, data );
        }
        else throw Exception("JSON entity must be integer");
      }
      
      std::string descData( void const *data ) const
      {
        return _( getValue( data ) );
      }
    
      // ComparableImpl
    
      virtual size_t hash( void const *data ) const
      {
        return size_t( getValue( data ) );
      }
      
      virtual int compare( void const *lhsData, void const *rhsData ) const
      {
        T lhsValue = getValue( lhsData );
        T rhsValue = getValue( rhsData );
        if ( lhsValue < rhsValue )
          return -1;
        else if ( lhsValue == rhsValue )
          return 0;
        else return 1;
      }
      
      // IntegerImpl

      virtual bool isSigned() const
      {
        return T(-1) < 0;
      }
      
      // IntegerImplT
      
      T getValue( void const *data ) const
      {
        return *static_cast<T const *>( data );
      }
      
      void setValue( T t, void *data ) const
      {
        *static_cast<T *>( data ) = t;
      }
    
    protected:

      IntegerImplT( std::string const &codeName )
        : IntegerImpl( codeName, sizeof(T) )
      {
      }
    };
    
    class ByteImpl : public IntegerImplT<uint8_t>
    {
      friend class Manager;
      
    protected:
    
      ByteImpl( std::string const &codeName )
        : IntegerImplT<uint8_t>( codeName )
      {
      }
    };
    
    class SI32Impl : public IntegerImplT<int32_t>
    {
      friend class Manager;
      
    protected:
    
      SI32Impl( std::string const &codeName )
        : IntegerImplT<int32_t>( codeName )
      {
      }
    };
    
    class SizeImpl : public IntegerImplT<size_t>
    {
      friend class Manager;
      
    protected:
    
      SizeImpl( std::string const &codeName )
        : IntegerImplT<size_t>( codeName )
      {
      }
    };
  };
};

#endif //_FABRIC_RT_INTEGER_IMPL_H

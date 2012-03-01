/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_JSON_DECODER_H
#define _FABRIC_JSON_DECODER_H

#include <Fabric/Base/Util/SimpleString.h>
#include <Fabric/Base/Util/Assert.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace JSON
  {
    static const size_t DecoderShortStringMaxLength = 16;
    
    struct Entity
    {
      enum Type
      {
        ET_UNDEFINED,
        ET_NULL,
        ET_BOOLEAN,
        ET_INTEGER,
        ET_SCALAR,
        ET_STRING,
        ET_OBJECT,
        ET_ARRAY
      };
      
      Type type;
      char const *data;
      size_t length;
      union
      {
        bool boolean;
        int32_t integer;
        double scalar;
        struct
        {
          size_t length;
          char shortData[DecoderShortStringMaxLength];
        } string;
        struct
        {
          size_t size;
        } object;
        struct
        {
          size_t size;
        } array;
      } value;
      
      Entity()
        : type( ET_UNDEFINED )
      {
      }
      
      void copyFrom( Entity const &that )
      {
        data = that.data;
        length = that.length;
        switch ( (type = that.type) )
        {
        case ET_BOOLEAN:
          value.boolean = that.value.boolean;
          break;
        case ET_INTEGER:
          value.integer = that.value.integer;
          break;
        case ET_SCALAR:
          value.scalar = that.value.scalar;
          break;
        case ET_STRING:
          value.string.length = that.value.string.length;
          memcpy( value.string.shortData, that.value.string.shortData, std::min( value.string.length, DecoderShortStringMaxLength ) );
          break;
        case ET_OBJECT:
          value.object.size = that.value.object.size;
          break;
        case ET_ARRAY:
          value.array.size = that.value.array.size;
          break;
        default:
          break;
        }
      }
      
      Entity( Entity const &that )
      {
        copyFrom( that );
      }
      
      Entity &operator =( Entity const &that )
      {
        copyFrom( that );
        return *this;
      }
      
      // Undefined
      
      bool isUndefined() const
      {
        return type == ET_UNDEFINED;
      }
      
      operator bool () const
      {
        return !isUndefined();
      }
      
      bool operator !() const
      {
        return isUndefined();
      }
      
      // Null
      
      bool isNull() const
      {
        return type == ET_NULL;
      }
      
      void requireNull() const
      {
        if ( !isNull() )
          throw Exception("must be a JSON null");
      }
      
      // Boolean
      
      bool isBoolean() const
      {
        return type == ET_BOOLEAN;
      }
      
      void requireBoolean() const
      {
        if ( !isBoolean() )
          throw Exception("must be a JSON boolean");
      }
      
      bool booleanValue() const
      {
        FABRIC_ASSERT( isBoolean() );
        return value.boolean;
      }
      
      // Integer
      
      bool isInteger() const
      {
        return type == ET_INTEGER;
      }
      
      void requireInteger() const
      {
        if ( !isInteger() )
          throw Exception("must be a JSON integer");
      }
      
      int32_t integerValue() const
      {
        FABRIC_ASSERT( isInteger() );
        return value.integer;
      }
      
      // Scalar
      
      bool isScalar() const
      {
        return type == ET_SCALAR;
      }
      
      void requireScalar() const
      {
        if ( !isScalar() )
          throw Exception("must be a JSON scalar");
      }
      
      double scalarValue() const
      {
        FABRIC_ASSERT( isScalar() );
        return value.scalar;
      }
      
      // String
      
      bool isString() const
      {
        return type == ET_STRING;
      }
      
      void requireString() const
      {
        if ( !isString() )
          throw Exception("must be a JSON string");
      }
      
      size_t stringLength() const
      {
        return value.string.length;
      }
      
      bool stringIsShort() const
      {
        return stringLength() <= DecoderShortStringMaxLength;
      }
      char const *stringShortData() const
      {
        FABRIC_ASSERT( stringIsShort() );
        return value.string.shortData;
      }
      
      void stringGetData( char *data ) const
      {
        if ( stringIsShort() )
        {
          memcpy( data, stringShortData(), stringLength() );
          return;
        }
        
        stringGetData_Long( data );
      }
      
      bool stringIs( char const *data, size_t length ) const
      {
        FABRIC_ASSERT( type == ET_STRING );
        FABRIC_ASSERT( strlen( data ) == length );
        if ( length != value.string.length )
          return false;
        if ( memcmp( data, value.string.shortData, std::min( length, DecoderShortStringMaxLength ) ) != 0 )
          return false;
        if ( value.string.length > DecoderShortStringMaxLength )
          return stringIs_Long( data, length );
        else return true;
      }
      bool stringIs( char const *cString ) const
      {
        return stringIs( cString, strlen( cString ) );
      }
      bool stringIs( std::string const &stdString ) const
      {
        return stringIs( stdString.data(), stdString.length() );
      }
      
      std::string stringToStdString() const;
      
      // Object
      
      bool isObject() const
      {
        return type == ET_OBJECT;
      }
      
      void requireObject() const
      {
        if ( !isObject() )
          throw Exception("must be a JSON object");
      }
      
      size_t objectSize() const
      {
        return value.object.size;
      }
      
      // Array
      
      bool isArray() const
      {
        return type == ET_ARRAY;
      }
      
      void requireArray() const
      {
        if ( !isArray() )
          throw Exception("must be a JSON array");
      }
      
      size_t arraySize() const
      {
        return value.array.size;
      }
    
    protected:
    
      void stringGetData_Long( char *data ) const;
      bool stringIs_Long( char const *data, size_t length ) const;
    };
      
    bool GetEntity( char const *&data, size_t &length, Entity &entity );
    
    class Decoder
    {
    public:
    
      Decoder( char const *data, size_t length )
        : m_data( data )
        , m_length( length )
      {
      }
      
      bool getNext( Entity &entity )
      {
        return GetEntity( m_data, m_length, entity );
      }
      
    private:
    
      char const *m_data;
      size_t m_length;
    };
    
    class ObjectDecoder
    {
    public:
    
      ObjectDecoder( Entity const &entity )
        : m_data( entity.data )
        , m_length( entity.length )
      {
        FABRIC_ASSERT( entity.isObject() );
        FABRIC_ASSERT( m_length > 0 );
        FABRIC_ASSERT( m_data[0] == '{' );
        ++m_data, --m_length;
      }
      
      bool getNext( Entity &memberKeyEntityInfo, Entity &memberValueEntityInfo );

      void rethrow( Exception const &e ) const;
    
    private:
    
      char const *m_data;
      size_t m_length;
      
      char const *m_lastKeyShortData;
      size_t m_lastKeyLength;
    };
    
    class ArrayDecoder
    {
    public:
    
      ArrayDecoder( Entity const &entity )
        : m_data( entity.data )
        , m_length( entity.length )
        , m_count( 0 )
      {
        FABRIC_ASSERT( entity.isArray() );
        FABRIC_ASSERT( m_length > 0 );
        FABRIC_ASSERT( m_data[0] == '[' );
        ++m_data, --m_length;
      }
      
      bool getNext( Entity &elementEntityInfo );

      void rethrow( Exception const &e ) const;
    
    private:
    
      char const *m_data;
      size_t m_length;
      size_t m_count;
      size_t m_lastIndex;
    };
  }
}

#endif //_FABRIC_JSON_DECODER_H

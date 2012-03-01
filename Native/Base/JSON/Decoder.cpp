/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/Util/Log.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Exception.h>

#include <math.h>

namespace Fabric
{
  namespace JSON
  {
    static const Exception malformedJSONException( "malformed JSON" );

    static inline void jsonSkipWhitespace( char const *&data, size_t &length )
    {
      while ( length > 0
        && ( data[0] == ' '
         || data[0] == '\n'
         || data[0] == '\r'
         || data[0] == '\t'
         || data[0] == '\v'
         || data[0] == '\f' )
        )
      {
        ++data, --length;
      }
    }

    static inline char jsonConsumeToken( char const *&data, size_t &length )
    {
      jsonSkipWhitespace( data, length );
      if ( length == 0 )
        throw malformedJSONException;
      char result = data[0];
      ++data, --length;
      return result;
    }
    
    static void jsonSkipString( char const *&data, size_t &length )
    {
      if ( length == 0 || data[0] != '"' )
        throw malformedJSONException;
      ++data, --length;
      
      bool done = false;
      while ( !done )
      {
        if ( length < 1 )
          throw malformedJSONException;
          
        switch ( data[0] )
        {
          case '"':
            done = true;
            break;
            
          case '\\':
            if ( length < 2 )
              throw malformedJSONException;
            switch ( data[1] )
            {
              case '"':
              case '/':
              case '\\':
              case 'b':
              case 'f':
              case 'n':
              case 'r':
              case 't':
                ++data, --length;
                break;
                
              case 'u':
                if ( length < 5 )
                  throw malformedJSONException;
                data += 4;
                length -= 4;
                break;
                
              default:
                throw malformedJSONException;
            }
            break;
          
          default:
            break;
        }
        
        ++data, --length;
      }
    }
    
    static inline uint8_t jsonHexToUInt8( char hex )
    {
      switch ( hex )
      {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a': case 'A': return 10;
        case 'b': case 'B': return 11;
        case 'c': case 'C': return 12;
        case 'd': case 'D': return 13;
        case 'e': case 'E': return 14;
        case 'f': case 'F': return 15;
        default: throw malformedJSONException;
      }
    }
    
    static inline uint16_t jsonConsumeUCS2( char const *&data, size_t &length )
    {
      if ( length < 4 )
        throw malformedJSONException;
      uint16_t result =
        (uint16_t(jsonHexToUInt8( data[0] )) << 12)
        | (uint16_t(jsonHexToUInt8( data[1] )) << 8)
        | (uint16_t(jsonHexToUInt8( data[2] )) << 4)
        | uint16_t(jsonHexToUInt8( data[3] ));
      data += 4;
      length -= 4;
      return result;
    }
    
    static inline void jsonStringAppendASCII( char ch, Entity &entity )
    {
      if ( entity.value.string.length < DecoderShortStringMaxLength )
        entity.value.string.shortData[entity.value.string.length] = ch;
      ++entity.value.string.length;
    }
    
    static inline size_t jsonUCS2ToUTF8( uint16_t ucs2, char utf8[3] )
    {
      if ( ucs2 < 0x80 )
      {
        utf8[0] = char(ucs2);
        return 1;
      }
      else if ( ucs2 < 0x800)
      {
        utf8[0] = char(ucs2 >> 6) | char(0xC0);
        utf8[1] = char(ucs2& 0x3F) | char(0x80);
        return 2;
      }
      else
      {
        utf8[0] = char(ucs2 >> 12) | char(0xE0);
        utf8[1] = (char(ucs2 >> 6) & char(0x3F)) | char(0xE0);
        utf8[2] = (char(ucs2) & char(0x3F)) | char(0x80);
        return 3;
      }
    }
    
    static inline void jsonStringAppendUCS2( uint16_t ucs2, Entity &entity )
    {
      char utf8[3];
      size_t utf8Length = jsonUCS2ToUTF8( ucs2, utf8 );
      jsonStringAppendASCII( utf8[0], entity );
      if ( utf8Length > 1 )
      {
        jsonStringAppendASCII( utf8[1], entity );
        if ( utf8Length > 2 )
          jsonStringAppendASCII( utf8[2], entity );
      }
    }
      
    bool GetEntity( char const *&data, size_t &length, Entity &entity )
    {
      jsonSkipWhitespace( data, length );
      if ( length == 0 )
        return false;
      
      entity.data = data;
      switch ( data[0] )
      {
        case 'n':
        {
          if ( length < 4
            || data[1] != 'u'
            || data[2] != 'l'
            || data[3] != 'l'
            )
            throw malformedJSONException;
          data += 4;
          length -= 4;
          entity.type = Entity::ET_NULL;
        }
        break;
        
        case 't':
        {
          if ( length < 4
            || data[1] != 'r'
            || data[2] != 'u'
            || data[3] != 'e'
            )
            throw malformedJSONException;
          data += 4;
          length -= 4;
          entity.type = Entity::ET_BOOLEAN;
          entity.value.boolean = true;
        }
        break;
        
        case 'f':
        {
          if ( length < 5
            || data[1] != 'a'
            || data[2] != 'l'
            || data[3] != 's'
            || data[4] != 'e'
            )
            throw malformedJSONException;
          data += 5;
          length -= 5;
          entity.type = Entity::ET_BOOLEAN;
          entity.value.boolean = false;
        }
        break;
        
        case '-':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        {
          if ( length > 0 && data[0] == '-' )
            ++data, --length;

          if ( length == 0 )
            throw malformedJSONException;
            
          switch ( data[0] )
          {
            case '0':
              ++data, --length;
              break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
              ++data, --length;
              while ( length > 0 && data[0] >= '0' && data[0] <= '9' )
                ++data, --length;
              break;
            default:
              throw malformedJSONException;
          }

          if ( length == 0 || (data[0] != '.' && data[0] != 'e' && data[0] != 'E') )
          {
            entity.type = Entity::ET_INTEGER;
            
            static const size_t maxIntegerLength = 15;
            size_t length = data - entity.data;
            if ( length > maxIntegerLength )
              throw malformedJSONException;
            char buf[maxIntegerLength+1];
            memcpy( buf, entity.data, length );
            buf[length] = '\0';
            entity.value.integer = atoi( buf );
          }
          else
          {
            entity.type = Entity::ET_SCALAR;
            
            if ( data[0] == '.' )
            {
              ++data, --length;
              
              if ( length == 0 || data[0] < '0' || data[0] > '9' )
                throw malformedJSONException;
              while ( length > 0 && data[0] >= '0' && data[0] <= '9' )
                ++data, --length;
            }
              
            if ( length > 0 && (data[0] == 'e' || data[0] == 'E') )
            {
              ++data, --length;

              if ( length > 0 && (data[0] == '-' || data[0] == '+') )
                ++data, --length;
              
              if ( length == 0 || data[0] < '0' || data[0] > '9' )
                throw malformedJSONException;
              ++data, --length;
              while ( length > 0 && data[0] >= '0' && data[0] <= '9' )
                ++data, --length;
            }
            
            static const size_t maxScalarLength = 31;
            size_t length = data - entity.data;
            if ( length > maxScalarLength )
              throw malformedJSONException;
            char buf[maxScalarLength+1];
            memcpy( buf, entity.data, length );
            buf[length] = '\0';
            entity.value.scalar = atof( buf );
          }
        }
        break;
        
        case '"':
        {
          entity.type = Entity::ET_STRING;
          entity.value.string.length = 0;
          ++data, --length;
          
          bool done = false;
          while ( !done )
          {
            if ( length == 0 )
              throw malformedJSONException;
            switch ( data[0] )
            {
              case '"':
                ++data, --length;
                done = true;
                break;
                
              case '\\':
              {
                ++data, --length;
                if ( length == 0 )
                  throw malformedJSONException;
                switch ( data[0] )
                {
                  case '"':
                  case '/':
                  case '\\':
                    jsonStringAppendASCII( data[0], entity );
                    ++data, --length;
                    break;
                  
                  case 'b':
                    jsonStringAppendASCII( '\b', entity );
                    ++data, --length;
                    break;
                  case 'f':
                    jsonStringAppendASCII( '\f', entity );
                    ++data, --length;
                    break;
                  case 'n':
                    jsonStringAppendASCII( '\n', entity );
                    ++data, --length;
                    break;
                  case 'r':
                    jsonStringAppendASCII( '\r', entity );
                    ++data, --length;
                    break;
                  case 't':
                    jsonStringAppendASCII( '\t', entity );
                    ++data, --length;
                    break;
                  
                  case 'u':
                    ++data, --length;
                    jsonStringAppendUCS2( jsonConsumeUCS2( data, length ), entity );
                    break;
                  
                  default:
                    throw malformedJSONException;
                }
              }
              break;
              
              default:
                jsonStringAppendASCII( data[0], entity );
                ++data, --length;
                break;
            }
          }
        }
        break;
        
        case '{':
        {
          entity.type = Entity::ET_OBJECT;
          entity.value.object.size = 0;
          ++data, --length;

          jsonSkipWhitespace( data, length );
          if ( length == 0 )
            throw malformedJSONException;
          if ( data[0] != '}' )
            ++entity.value.array.size;
          
          size_t nestCount = 1;
          while ( nestCount > 0 )
          {
            if ( length == 0 )
              throw malformedJSONException;
            switch ( data[0] )
            {
              case '[':
              case '{':
                ++nestCount;
                ++data, --length;
                break;
              
              case ',':
                if ( nestCount == 1 )
                  ++entity.value.array.size;
                ++data, --length;
                break;
              
              case ']':
              case '}':
                --nestCount;
                ++data, --length;
                break;
              
              case '"':
                jsonSkipString( data, length );
                break;
            
              default:
                ++data, --length;
                break;
            }
          }
        }
        break;
        
        case '[':
        {
          entity.type = Entity::ET_ARRAY;
          entity.value.array.size = 0;
          ++data, --length;

          jsonSkipWhitespace( data, length );
          if ( length == 0 )
            throw malformedJSONException;
          if ( data[0] != ']' )
            ++entity.value.array.size;
          
          size_t nestCount = 1;
          while ( nestCount > 0 )
          {
            if ( length == 0 )
              throw malformedJSONException;
            switch ( data[0] )
            {
              case '[':
              case '{':
                ++nestCount;
                ++data, --length;
                break;
              
              case ',':
                if ( nestCount == 1 )
                  ++entity.value.array.size;
                ++data, --length;
                break;
              
              case ']':
              case '}':
                --nestCount;
                ++data, --length;
                break;
              
              case '"':
                jsonSkipString( data, length );
                break;
            
              default:
                ++data, --length;
                break;
            }
          }
        }
        break;
        
        default:
          throw malformedJSONException;
      }
      
      entity.length = data - entity.data;
      return true;
    }

    static inline void jsonConsumeEntity( char const *&data, size_t &length, Entity &entity )
    {
      if ( !GetEntity( data, length, entity ) )
        throw malformedJSONException;
    }
    
    bool ObjectDecoder::getNext( Entity &keyString, Entity &valueEntity )
    {
      if ( m_length == 0 || m_data[0] == '}' )
        return false;
        
      jsonConsumeEntity( m_data, m_length, keyString );
      if ( !keyString.isString() )
        throw malformedJSONException;
      
      switch ( jsonConsumeToken( m_data, m_length ) )
      {
        case ':':
          break;
          
        default:
          throw malformedJSONException;
      }
      
      jsonConsumeEntity( m_data, m_length, valueEntity );
      
      switch ( jsonConsumeToken( m_data, m_length ) )
      {
        case ',':
          if ( m_length == 0 )
            throw malformedJSONException;
          break;
        case '}':
          if ( m_length > 0 )
            throw malformedJSONException;
          break;
        default:
          throw malformedJSONException;
      }
      
      m_lastKeyShortData = keyString.value.string.shortData;
      m_lastKeyLength = keyString.value.string.length;

      return true;
    }

    void ObjectDecoder::rethrow( Exception const &e ) const
    {
      throw _( m_lastKeyShortData, m_lastKeyLength, DecoderShortStringMaxLength ) + ": " + e;
    }
    
    bool ArrayDecoder::getNext( Entity &elementEntityInfo )
    {
      if ( m_length == 0 || m_data[0] == ']' )
        return false;

      jsonConsumeEntity( m_data, m_length, elementEntityInfo );
      
      switch ( jsonConsumeToken( m_data, m_length ) )
      {
        case ',':
          if ( m_length == 0 )
            throw malformedJSONException;
          break;
        case ']':
          if ( m_length > 0 )
            throw malformedJSONException;
          break;
        default:
          throw malformedJSONException;
      }
      
      m_lastIndex = ++m_count;
      return true;
    }

    void ArrayDecoder::rethrow( Exception const &e ) const
    {
      throw "index " + _(m_lastIndex) + ": " + e;
    }

    bool Entity::stringIs_Long( char const *thatData, size_t thatLength ) const
    {
      char const *entityData = data;
      size_t entityLength = length;
      FABRIC_ASSERT( entityLength > 0 );
      FABRIC_ASSERT( entityData[0] == '"' );
      ++entityData, --entityLength;
      while ( thatLength > 0 )
      {
        FABRIC_ASSERT( entityLength > 0 );
        switch ( entityData[0] )
        {
          case '"':
            FABRIC_ASSERT( false );
            return false;
            
          case '\\':
            ++entityData, --entityLength;
            FABRIC_ASSERT( entityLength > 0 );
            switch ( entityData[0] )
            {
            case '"':
            case '/':
            case '\\':
              if ( thatData[0] != entityData[0] )
                return false;
              ++entityData, --entityLength;
              ++thatData, --thatLength;
              break;
            
            case 'b':
              if ( thatData[0] != '\b' )
                return false;
              ++entityData, --entityLength;
              ++thatData, --thatLength;
              break;
            case 'f':
              if ( thatData[0] != '\f' )
                return false;
              ++entityData, --entityLength;
              ++thatData, --thatLength;
              break;
            case 'n':
              if ( thatData[0] != '\n' )
                return false;
              ++entityData, --entityLength;
              ++thatData, --thatLength;
              break;
            case 'r':
              if ( thatData[0] != '\r' )
                return false;
              ++entityData, --entityLength;
              ++thatData, --thatLength;
              break;
            case 't':
              if ( thatData[0] != '\t' )
                return false;
              ++entityData, --entityLength;
              ++thatData, --thatLength;
              break;
            
            case 'u':
            {
              ++entityData, --entityLength;
              FABRIC_ASSERT( entityLength >= 4 );
              uint16_t ucs2 = jsonConsumeUCS2( entityData, entityLength );
              
              char utf8[3];
              size_t utf8Length = jsonUCS2ToUTF8( ucs2, utf8 );
              if ( thatLength < utf8Length
                || memcmp( utf8, thatData, utf8Length ) != 0 )
                return false;
              thatData += utf8Length, thatLength -= utf8Length;
            }
            break;
            
            default:
              FABRIC_ASSERT( false );
              return false;
          }
          
          default:
            if ( entityData[0] != thatData[0] )
              return false;
            ++entityData, --entityLength;
            ++thatData, --thatLength;
            break;
        }
      }
      FABRIC_ASSERT( entityLength == 1 );
      FABRIC_ASSERT( entityData[0] == '"' );

      return true;
    }
    
    std::string Entity::stringToStdString() const
    {
      size_t length = stringLength();
      
      std::string result;
      result.resize( length );
      stringGetData( &result[0] );
      return result;
    }

    void Entity::stringGetData_Long( char *dstData ) const
    {
      FABRIC_ASSERT( type == ET_STRING );
      
      char const *entityData = data;
      size_t entityLength = length;
      
      FABRIC_ASSERT( entityLength > 0 );
      FABRIC_ASSERT( entityData[0] == '"' );
      ++entityData, --entityLength;
      
      bool done = false;
      while ( !done )
      {
        FABRIC_ASSERT( entityLength > 0 );
        
        switch ( entityData[0] )
        {
          case '"':
            done = true;
            break;
          
          case '\\':
            ++entityData, --entityLength;
            FABRIC_ASSERT( entityLength > 0 );
            switch ( entityData[0] )
            {
              case '"':
              case '/':
              case '\\':
                *dstData++ = entityData[0];
                ++entityData, --entityLength;
                break;
              
              case 'b':
                *dstData++ = '\b';
                ++entityData, --entityLength;
                break;
              case 'f':
                *dstData++ = '\f';
                ++entityData, --entityLength;
                break;
              case 'n':
                *dstData++ = '\n';
                ++entityData, --entityLength;
                break;
              case 'r':
                *dstData++ = '\r';
                ++entityData, --entityLength;
                break;
              case 't':
                *dstData++ = '\t';
                ++entityData, --entityLength;
                break;
              
              case 'u':
              {
                ++entityData, --entityLength;
                uint16_t ucs2 = jsonConsumeUCS2( entityData, entityLength );
                dstData += jsonUCS2ToUTF8( ucs2, dstData );
              }
              break;
              
              default:
                FABRIC_ASSERT( false );
                ++entityData, --entityLength;
                break;
            }
            break;
          
          default:
            *dstData++ = entityData[0];
            ++entityData, --entityLength;
            break;
        }
      }
      
      FABRIC_ASSERT( entityLength == 1 );
      FABRIC_ASSERT( entityData[0] == '"' );
    }
  }
}

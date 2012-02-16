/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Core/Util/Parse.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Util/Assert.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

namespace Fabric
{
  namespace Util
  {
    uint64_t parseUInt64( char const *data, size_t length )
    {
      uint64_t result = 0;
      if ( length >= 2 && data[0] == '0' && tolower( data[1] ) == 'x' )
      {
        for ( size_t i=2; i<length; ++i )
        {
          uint64_t oldResult = result;
          char ch = tolower( data[i] );
          if ( ch >= '0' && ch <= '9' )
            result = 16 * result + ch - '0';
          else if ( ch >= 'a' && ch <= 'f' )
            result = 16 * result + ch - 'a' + 10;
          else throw Exception( "malformed hexadecimal integer constant" );
          if ( result < oldResult )
            throw Exception( "hexadecimal integer constant out of range" );
        }
      }
      else
      {
        for ( size_t i=0; i<length; ++i )
        {
          uint64_t oldResult = result;
          char ch = data[i];
          if ( ch >= '0' && ch <= '9' )
            result = 10 * result + ch - '0';
          else throw Exception( "malformed decimal size constant" );
          if ( result < oldResult )
            throw Exception( "decimal integer constant out of range" );
        }
      }
      return result;
    }

    double parseDouble( char const *data, size_t length )
    {
      char *cString = static_cast<char *>( alloca( length + 1 ) );
      memcpy( cString, data, length );
      cString[length] = '\0';
      return parseDouble( cString );
    }

    static inline uint8_t parseHex( char const *data, size_t length )
    {
      uint8_t result = 0;
      for ( size_t i=0; i<length; ++i )
      {
        try
        {
          char ch = data[i];
          switch ( ch )
          {
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
              result = 16*result + ch - '0';
              break;
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
              result = 16*result + 10 + ch - 'a';
              break;
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
              result = 16*result + 10 + ch - 'A';
              break;
            default:
              throw Exception("invalid hex character");
          }
        }
        catch ( Exception e )
        {
          throw "index "+_(i)+": "+e;
        }
      }
      return result;
    }
    
    std::string parseQuotedString( char const *data, size_t length )
    {
      static Exception exception("malformed string constant");
      std::string result;
      
      char const *dataEnd = data + length;
      
      char quote = *data++;
      if ( quote != '\'' && quote != '"' )
        throw exception;
              
      while ( data != dataEnd )
      {
        char ch = *data++;
        if ( ch == quote )
        {
          if ( data != dataEnd )
            throw exception;
          break;
        }
        else if ( ch == '\\' )
        {
          if ( data == dataEnd )
            throw exception;
          char ch = *data++;
          switch ( ch )
          {
            case 'n':
              result.push_back( '\n' );
              break;
            case 'f':
              result.push_back( '\f' );
              break;
            case 'r':
              result.push_back( '\r' );
              break;
            case 't':
              result.push_back( '\t' );
              break;
            case 'v':
              result.push_back( '\v' );
              break;
            case 'a':
              result.push_back( '\a' );
              break;
            case 'b':
              result.push_back( '\b' );
              break;
            case '0':
              result.push_back( '\0' );
              break;
            case '"':
              result.push_back( '"' );
              break;
            case '\'':
              result.push_back( '\'' );
              break;
            case '\\':
              result.push_back( '\\' );
              break;
            case 'x':
            {
              if ( data == dataEnd )
                throw exception;
              char const *hexData = data;
              if ( data++ == dataEnd || data++ == dataEnd )
                throw exception;
              result.push_back( parseHex( hexData, 2 ) );
            }
            break;
            default:
              throw exception;
          }
        }
        else result.push_back( ch );
      }
      
      return result;
    }
  }
}

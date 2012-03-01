/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_JSON_ENCODER_H
#define _FABRIC_JSON_ENCODER_H

#include <Fabric/Base/Util/SimpleString.h>
#include <Fabric/Base/Util/Assert.h>
#include <Fabric/Base/RC/ConstHandle.h>
#include <Fabric/Base/RC/String.h>

namespace Fabric
{
  namespace JSON
  {
    class ObjectEncoder;
    class ArrayEncoder;
    
    class Encoder
    {
    public:
    
      Encoder( Util::SimpleString *ss )
        : m_ss( ss )
      {
        FABRIC_ASSERT( m_ss );
      }
      
      ~Encoder()
      {
        // [pzion 20111023] We would like to assert here but
        // we can't because an exception might be thrown
        //FABRIC_ASSERT( !m_ss );
      }
      
      void makeNull() const
      {
        FABRIC_ASSERT( m_ss );
        m_ss->append( "null", 4 );
        FABRIC_ASSERT( !(m_ss = 0) );
      }
      
      void makeBoolean( bool value ) const
      {
        FABRIC_ASSERT( m_ss );
        if ( value )
          m_ss->append( "true", 4 );
        else
          m_ss->append( "false", 5 );
        FABRIC_ASSERT( !(m_ss = 0) );
      }
      
      void makeInteger( int32_t value ) const
      {
        FABRIC_ASSERT( m_ss );
        m_ss->increaseLength( snprintf( m_ss->expand( 11 ), 11, "%d", (int)value ) );
        FABRIC_ASSERT( !(m_ss = 0) );
      }
      
      void makeScalar( float value ) const
      {
        FABRIC_ASSERT( m_ss );
        m_ss->increaseLength( snprintf( m_ss->expand( 40 ), 40, "%.16g", value ) );
        FABRIC_ASSERT( !(m_ss = 0) );
      }
      
      void makeString( char const *data, size_t length ) const
      {
        FABRIC_ASSERT( m_ss );
        char *tail = m_ss->expand( 2*length+2 );
        char *p = tail;
        *p++ = '"';
        char const *dataEnd = data + length;
        for ( ; data != dataEnd; ++data )
        {
          switch ( *data )
          {
            case '"':
              *p++ = '\\';
              *p++ = '"';
              break;
            case '\\':
              *p++ = '\\';
              *p++ = '\\';
              break;
            case '\b':
              *p++ = '\\';
              *p++ = 'b';
              break;
            case '\f':
              *p++ = '\\';
              *p++ = 'f';
              break;
            case '\n':
              *p++ = '\\';
              *p++ = 'n';
              break;
            case '\r':
              *p++ = '\\';
              *p++ = 'r';
              break;
            case '\t':
              *p++ = '\\';
              *p++ = 't';
              break;
            default:
              *p++ = *data;
              break;
          }
        }
        *p++ = '"';
        m_ss->increaseLength( p - tail );
        FABRIC_ASSERT( !(m_ss = 0) );
      }
      
      void makeString( char const *cStr ) const
      {
        makeString( cStr, strlen( cStr ) );
      }
      
      void makeString( std::string const &string ) const
      {
        makeString( string.data(), string.length() );
      }
      
      void makeString( RC::ConstHandle<RC::String> const &string ) const
      {
        makeString( string->data(), string->length() );
      }
      
      ObjectEncoder makeObject() const;
      
      ArrayEncoder makeArray() const;
      ArrayEncoder *newArray() const;
      
      void appendJSON( char const *data, size_t length ) const
      {  
        FABRIC_ASSERT( m_ss );
        m_ss->append( data, length );
        FABRIC_ASSERT( !(m_ss = 0) );
      }
      
      void appendJSON( Util::SimpleString const &ss ) const
      {  
        FABRIC_ASSERT( m_ss );
        m_ss->append( ss );
        FABRIC_ASSERT( !(m_ss = 0) );
      }
      
      void flush()
      {
        FABRIC_ASSERT( !m_ss );
      }
      
    private:
    
      mutable Util::SimpleString *m_ss;
    };
    
    class ObjectEncoder
    {
    public:
    
      ObjectEncoder( Util::SimpleString *ss )
        : m_ss( ss )
        , m_count( 0 )
      {
        m_ss->append( '{' );
      }

#if defined(FABRIC_OS_WINDOWS)
      ObjectEncoder(ObjectEncoder& other) {
        *this = other;
      }

      ObjectEncoder& operator=(ObjectEncoder& other) {
        m_ss = other.m_ss;
        m_count = other.m_count;
        other.m_ss = 0;
        return *this;
      }
		
#endif

      ~ObjectEncoder()
      {
        flush();
      }
      
      void flush()
      {
        if ( m_ss )
        {
          m_ss->append( '}' );
          m_ss = 0;
        }
      }
      
      Encoder makeMember( char const *data, size_t length ) const
      {
        FABRIC_ASSERT( m_ss );
        if ( m_count++ > 0 )
          m_ss->append( ',' );
        Encoder( m_ss ).makeString( data, length );
        m_ss->append( ':' );
        return Encoder( m_ss );
      }
      
      Encoder makeMember( char const *name ) const
      {
        return makeMember( name, strlen( name ) );
      }
      
      Encoder makeMember( std::string const &string ) const
      {
        return makeMember( string.data(), string.length() );
      }
      
      Encoder makeMember( Util::SimpleString const &ss ) const
      {
        return makeMember( ss.getData(), ss.getLength() );
      }
      
    private:
    
      Util::SimpleString *m_ss;
      mutable size_t m_count;
    };
    
    inline ObjectEncoder Encoder::makeObject() const
    {
      FABRIC_ASSERT( m_ss );
      ObjectEncoder result = ObjectEncoder( m_ss );
      FABRIC_ASSERT( !(m_ss = 0) );
      return result;
    }
    
    class ArrayEncoder
    {
    public:
    
      ArrayEncoder( Util::SimpleString *ss )
        : m_ss( ss )
        , m_count( 0 )
      {
        m_ss->append( '[' );
      }

#if defined(FABRIC_OS_WINDOWS)
      ArrayEncoder( ArrayEncoder &other )
      {
        *this = other;
      }

      ArrayEncoder& operator =( ArrayEncoder &other )
      {
        m_ss = other.m_ss;
        m_count = other.m_count;
        other.m_ss = 0;
        return *this;
      }
#endif
		
      ~ArrayEncoder()
      {
        flush();
      }
      
      void flush()
      {
        if ( m_ss )
        {
          m_ss->append( ']' );
          m_ss = 0;
        }
      }
      
      Encoder makeElement() const
      {
        FABRIC_ASSERT( m_ss );
        if ( m_count++ > 0 )
          m_ss->append( ',' );
        return Encoder( m_ss );
      }
      
      size_t getCount() const
      {
        return m_count;
      }
      
    private:
    
      Util::SimpleString *m_ss;
      mutable size_t m_count;
    };
    
    inline ArrayEncoder Encoder::makeArray() const
    {
      FABRIC_ASSERT( m_ss );
      ArrayEncoder result = ArrayEncoder( m_ss );
      FABRIC_ASSERT( !(m_ss = 0) );
      return result;
    }
    
    inline ArrayEncoder *Encoder::newArray() const
    {
      FABRIC_ASSERT( m_ss );
      ArrayEncoder *result = new ArrayEncoder( m_ss );
      FABRIC_ASSERT( !(m_ss = 0) );
      return result;
    }
  };
};

#endif //_FABRIC_JSON_ENCODER_H

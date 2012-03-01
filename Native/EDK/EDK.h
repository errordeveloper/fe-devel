/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_EDK_H
#define _FABRIC_EDK_H

#include <Fabric/Base/Config.h>
#include <Fabric/Base/Util/Bits.h>
#include <Fabric/Base/Util/AtomicSize.h>
#include <Fabric/Base/Exception.h>
#include <Fabric/EDK/Common.h>

#if defined(FABRIC_OS_WINDOWS)
# define FABRIC_EXT_EXPORT extern "C" __declspec(dllexport)
# define FABRIC_EXT_DECL_BEGIN __pragma( pack(push, 1) )
# define FABRIC_EXT_DECL_END ;__pragma( pack(pop) )
# define FABRIC_EXT_KL_STRUCT( name, content ) \
 FABRIC_EXT_DECL_BEGIN \
 struct name content \
 FABRIC_EXT_DECL_END
# define FABRIC_EXT_KL_CLASS( name, content ) \
 FABRIC_EXT_DECL_BEGIN \
 class name content \
 FABRIC_EXT_DECL_END
#else
# define FABRIC_EXT_EXPORT extern "C" __attribute__ ((visibility("default")))
# define FABRIC_EXT_DECL_BEGIN
# define FABRIC_EXT_DECL_END __attribute__((__packed__))
# define FABRIC_EXT_KL_STRUCT( name, content ) \
 struct name content FABRIC_EXT_DECL_END
# define FABRIC_EXT_KL_CLASS( name, content ) \
 class name content FABRIC_EXT_DECL_END
#endif

#if defined(FABRIC_OS_LINUX)
  #include <stdlib.h>
  #include <string.h>
  #include <math.h>
#endif

namespace Fabric
{
  namespace EDK
  {
    static Callbacks s_callbacks;
    
    inline void throwException( size_t length, char const *data )
    {
      printf("Fabric::EDK::Exception: %s\n",data);
      s_callbacks.m_throwException( length, data );
    }
    
    inline void throwException( char const *cStrFormat, ... )
    {
      char cStr[4096];
      va_list argList;
      va_start( argList, cStrFormat );
      int vsnprintfResult = vsnprintf( cStr, 4096, cStrFormat, argList );
      va_end( argList );
      if ( vsnprintfResult < 0 )
      {
        static const char *malformattedException = "internal error: malformed exception";
        throwException( strlen( malformattedException ), malformattedException );
      }
      else throwException( vsnprintfResult, cStr );
    }
    
    inline void throwException( std::string const &string )
    {
      throwException( string.length(), string.data() );
    }

    namespace KL
    {
      typedef bool Boolean;
      typedef uint8_t Byte;
      typedef int32_t Integer;
      typedef size_t Size;
      typedef float Scalar;
      typedef void *Data;
    
      FABRIC_EXT_KL_CLASS( StringBase, {
        struct bits_t
        {
          Util::AtomicSize refCount;
          size_t allocSize;
          size_t length;
          char cStr[0]; // [pzion 20101225] Will be extended to length of string plus null terminator
        };

      public:
    
        StringBase &assign( StringBase const &that )
        {
          if ( m_bits != that.m_bits )
          {
            if ( m_bits && m_bits->refCount.decrementAndGetValue() == 0 )
              ( *s_callbacks.m_free )( m_bits );
            m_bits = that.m_bits;
            if ( m_bits )
              m_bits->refCount.increment();
          }
          return *this;
        }
      
        StringBase &operator =( StringBase const &that )
        {
          return assign( that );
        }
      
        StringBase &operator =( char const *cString )
        {
          setData( cString, strlen(cString) );
          return *this;
        }
      
        char const *data() const
        {
          return m_bits? m_bits->cStr: 0;
        }
      
        size_t length() const
        {
          return m_bits? m_bits->length: 0;
        }
      
        StringBase &append( char const *data, size_t length )
        {
          memcpy( expand( length ), data, length );
          return *this;
        }
      
        StringBase &append( char ch )
        {
          return append( &ch, 1 );
        }
      
        StringBase &operator +=( char ch )
        {
          return append( ch );
        }
      
        StringBase &append( char const *cString )
        {
          return append( cString, strlen(cString) );
        }
      
        StringBase &operator +=( char const *cString )
        {
          return append( cString );
        }
      
        StringBase &append( StringBase const &that )
        {
          if ( m_bits )
          {
            size_t thatLength = that.length();
            memcpy( expand( thatLength ), that.data(), thatLength );
            return *this;
          }
          else return assign( that );
        }

        StringBase &operator +=( StringBase const &that )
        {
          return append( that );
        }
    
        void reserve( size_t capacity )
        {
          if ( (!m_bits && capacity > 0)
            || (m_bits && m_bits->refCount.getValue() > 1)
            || (m_bits && capacity == 0)
            || (m_bits && m_bits->allocSize < capacity + 1)
            )
          {
            bits_t *newBits;
            if ( capacity )
            {
              size_t newAllocSize = AllocSizeForLength( capacity );
              newBits = reinterpret_cast<bits_t *>( ( *s_callbacks.m_malloc )( sizeof(bits_t) + newAllocSize ) );
              newBits->refCount.setValue( 1 );
              newBits->allocSize = newAllocSize;
              if ( m_bits )
                memcpy( newBits->cStr, m_bits->cStr, m_bits->length );
              newBits->length = capacity;
            }
            else newBits = 0;
          
            if ( m_bits && m_bits->refCount.decrementAndGetValue() == 0 )
              ( *s_callbacks.m_free )( m_bits );
            m_bits = newBits;
          }
        }
      
      protected:
    
        static size_t AllocSizeForLength( size_t length )
        {
          return std::max( size_t(31), Util::nextPowerOfTwoMinusOne( length+1 ) );
        }
    
        void setData( char const *data, size_t length )
        {
          reserve( length );
          if ( m_bits )
          {
            memcpy( m_bits->cStr, data, length );
            m_bits->cStr[length] = '\0';
          }
        }
      
        char *expand( size_t delta )
        {
          size_t oldLength = length();
          size_t newLength = oldLength + delta;
        
          reserve( newLength );
          if ( m_bits )
          {
            m_bits->cStr[newLength] = '\0';
            return &m_bits->cStr[oldLength];
          }
          else return 0;
        }
      
        bits_t *m_bits;
      } );
    
      FABRIC_EXT_KL_CLASS( String : public StringBase, {
      public:
    
        String()
        {
          m_bits = 0;
        }
      
        String( char const *cString )
        {
          m_bits = 0;
          size_t length = strlen( cString );
          memcpy( expand( length ), cString, length );
        }

        String( String const &that )
        {
          m_bits = that.m_bits;
          if ( m_bits )
            m_bits->refCount.increment();
        }
        
        ~String()
        {
          if ( m_bits && m_bits->refCount.decrementAndGetValue() == 0 )
            ( *s_callbacks.m_free )( m_bits );
        }
        
        typedef StringBase const &IN;
        typedef StringBase &IO;
      });

      FABRIC_EXT_KL_CLASS( FileHandleWrapper, {

      public:
        FileHandleWrapper(){}
        FileHandleWrapper( String const &handleString )
        {
          wrap( handleString );
        }

        void wrap( String const &handleString )
        {
          m_handle = handleString;
        }

        void createFromFile( char const *filePathCString, bool readOnly )
        {
          ( *s_callbacks.m_fileHandleCreateFromPath)( &m_handle, filePathCString, false, readOnly );
        }

        void createFromFolder( char const *folderCString, bool readOnly )
        {
          ( *s_callbacks.m_fileHandleCreateFromPath)( &m_handle, folderCString, true, readOnly );
        }

        String get() const
        {
          return m_handle;
        }

        operator String() const
        {
          return m_handle;
        }

        String getPath() const
        {
          String path;
          ( *s_callbacks.m_fileGetPath )( &m_handle, &path );
          return path;
        }

        bool isValid() const
        {
          return ( *s_callbacks.m_fileHandleIsValid )( &m_handle );
        }

        bool isReadOnly() const
        {
          return ( *s_callbacks.m_fileHandleIsReadOnly )( &m_handle );
        }

        bool isFolder() const //else: file
        {
          return ( *s_callbacks.m_fileHandleIsFolder )( &m_handle );
        }

        bool targetExists() const
        {
          return ( *s_callbacks.m_fileHandleTargetExists )( &m_handle );
        }

        void ensureTargetExists() const
        {
          ( *s_callbacks.m_fileHandleEnsureTargetExists )( &m_handle );
        }

        void ensureIsValidFile() const
        {
          if( !isValid() )
            throwException( "FileHandle '%s' is not a valid fileHandle.", m_handle.data() );
          if( isFolder() )
            throwException( "FileHandle '%s' is a folder.", m_handle.data() );
        }

      private:

        String m_handle;
      } );

      FABRIC_EXT_KL_STRUCT( RGBA, {
        Byte r;
        Byte g;
        Byte b;
        Byte a;
      } );
    
      FABRIC_EXT_KL_STRUCT( RGB, {
        Byte r;
        Byte g;
        Byte b;
      } );
    
      FABRIC_EXT_KL_STRUCT( Color, {
        Scalar r;
        Scalar g;
        Scalar b;
        Scalar a;
      } );
      
      FABRIC_EXT_KL_STRUCT( Vec2, {
        Scalar x;
        Scalar y;
      } );
    
      FABRIC_EXT_KL_STRUCT( Vec3, {
        Scalar x;
        Scalar y;
        Scalar z;
      } );
    
      FABRIC_EXT_KL_STRUCT( Vec4, {
        Scalar x;
        Scalar y;
        Scalar z;
        Scalar t;
      } );

      FABRIC_EXT_KL_STRUCT( Quat, {
        Vec3 v;
        Scalar w;
      } );
      
      FABRIC_EXT_KL_STRUCT( Xfo, {
        Quat ori;
        Vec3 tr;
        Vec3 sc;
      } );

      FABRIC_EXT_KL_STRUCT( Mat22, {
        Vec2 row0;
        Vec2 row1;
      } );
    
      FABRIC_EXT_KL_STRUCT( Mat33, {
        Vec3 row0;
        Vec3 row1;
        Vec3 row2;
      } );
    
      FABRIC_EXT_KL_STRUCT( Mat44, {
        Vec4 row0;
        Vec4 row1;
        Vec4 row2;
        Vec4 row3;
      } );

      FABRIC_EXT_DECL_BEGIN //Note: FABRIC_EXT_KL_CLASS macro can't be used on templated classes
      template< class Member, bool copyOnWrite = true > class VariableArray
      {
      public:
        
        typedef VariableArray const &IN;
        typedef VariableArray &IO;
        
        void init( size_t size )
        {
          m_allocSize = size;
          m_size = size;
          m_memberDatas = static_cast<Member *>( ( *s_callbacks.m_malloc )( m_allocSize * sizeof(Member) ) );
          memset( &m_memberDatas[0], 0, m_size * sizeof(Member) );
        }
    
        VariableArray()
          : m_allocSize( 0 )
          , m_size( 0 )
          , m_memberDatas( 0 )
        {
        }
        
        VariableArray( size_t size )
        {
          init( size );
        }
      
        void assign( VariableArray const &that )
        {
          if ( m_memberDatas )
            ( *s_callbacks.m_free )( m_memberDatas );
          m_allocSize = that.m_size;
          m_size = that.m_size;
          m_memberDatas = static_cast<Member *>( ( *s_callbacks.m_malloc )( m_allocSize * sizeof(Member) ) );
          memset( &m_memberDatas[0], 0, m_size * sizeof(Member) );
          for ( size_t i=0; i<m_size; ++i )
            m_memberDatas[i] = that.m_memberDatas[i];
        }
        
        VariableArray( VariableArray const &that )
          : m_memberDatas( 0 )
        {
          assign( that );
        }
        
        void dispose()
        {
          if ( m_memberDatas )
            ( *s_callbacks.m_free )( m_memberDatas );
        }
        
        ~VariableArray()
        {
          dispose();
        }
      
        VariableArray &operator =( VariableArray const &that )
        {
          assign( that );
          *this;
        }
      
        Member const &member( size_t index ) const
        {
          return m_memberDatas[index];
        }
      
        Member const &operator[]( size_t index ) const        {
          return member( index );
        }
      
        Member &member( size_t index )
        {
          return m_memberDatas[index];
        }
      
        Member &operator[]( size_t index )
        {
          return member( index );
        }
      
        size_t size() const
        {
          return m_size;
        }
      
        void resize( size_t newSize )
        {
          size_t oldSize = m_size;
          if ( oldSize != newSize )
          {
            size_t oldAllocSize = m_allocSize;
            if ( newSize < oldSize )
            {
              //getMemberImpl()->disposeDatas( bits->memberDatas + m_memberSize * newNumMembers, oldNumMembers - newNumMembers, m_memberSize );
            }
              
            if ( newSize == 0 )
            {
              ( *s_callbacks.m_free )( m_memberDatas );
              m_allocSize = 0;
              m_memberDatas = 0;
            }
            else
            {
              if ( newSize > oldAllocSize )
              {
                size_t newAllocSize = ComputeAllocatedSize( oldAllocSize, newSize );
                size_t size = sizeof(Member) * newAllocSize;
                if ( oldSize )
                {
                  m_memberDatas = static_cast<Member *>( ( *s_callbacks.m_realloc )( m_memberDatas, size ) );
                }
                else
                {
                  m_memberDatas = static_cast<Member *>( ( *s_callbacks.m_malloc )( size ) );
                }
                m_allocSize = newAllocSize;
              }
              if( newSize > oldSize )
                  memset( m_memberDatas + oldSize, 0, (newSize - oldSize) * sizeof(Member) );
            }
            m_size = newSize;
          }
        }
      
      protected:
    
        static size_t ComputeAllocatedSize( size_t prevNbAllocated, size_t nbRequested )
        {
          if( nbRequested > prevNbAllocated )
          {
            size_t inflatedNbAllocated;
            if( prevNbAllocated < 16 ) 
              inflatedNbAllocated = (prevNbAllocated>>1) + 1 + prevNbAllocated;
            else
              inflatedNbAllocated = (prevNbAllocated>>3) + 4 + prevNbAllocated;
            return std::max( nbRequested, inflatedNbAllocated );
          }
          else if( nbRequested < prevNbAllocated )
          {
            if( nbRequested == 0 )
              return 0;
            if( prevNbAllocated < 16 )
              return prevNbAllocated;
            size_t deflateThreshold = prevNbAllocated - (prevNbAllocated>>2);
            return nbRequested <= deflateThreshold ? nbRequested : prevNbAllocated;
          }
          else
            return nbRequested;
        }
      
      private:
    
        size_t m_allocSize;
        size_t m_size;
        Member *m_memberDatas;
      }
      FABRIC_EXT_DECL_END;

      FABRIC_EXT_DECL_BEGIN //Note: FABRIC_EXT_KL_CLASS macro can't be used on templated classes
      template< class Member > class SlicedArray
      {
        struct RCVA
        {
          size_t refCount;
          VariableArray<Member> varArray;
        };
      
      public:
        
        typedef SlicedArray const &IN;
        typedef SlicedArray &IO;
    
        SlicedArray( size_t size )
          : m_offset( 0 )
          , m_size( size )
        {
          m_rcva = static_cast<RCVA *>( ( *s_callbacks.m_malloc )( sizeof( RCVA ) ) );
          m_rcva->refCount = 1;
          m_rcva->varArray.init( size );
        }
      
        SlicedArray( SlicedArray const &that )
          : m_offset( that.m_offset )
          , m_size( that.size )
          , m_rcva( that.m_rcva )
        {
          if ( m_rcva )
            ++m_rcva->refCount;
        }
      
        SlicedArray( SlicedArray const &that, size_t offset, size_t size )
          : m_offset( that.m_offset )
          , m_size( that.size )
          , m_rcva( that.m_variableArray )
        {
          if ( m_rcva && offset + size > m_rcva->varArray.size() )
            throw Exception( "SlicedArray: offset and/or size out of range" );
          
          if ( m_rcva )
            ++m_rcva->refCount;
        }
      
        SlicedArray &operator =( SlicedArray const &that )
        {
          if ( m_rcva && --m_rcva->refCount == 0 )
          {
            m_rcva->varArray.dispose();
            ( *s_callbacks.m_free )( m_rcva );
          }

          m_offset = that.m_offset;
          m_size = that.m_size;
          m_rcva = that.m_rcva;
          
          if ( m_rcva )
            ++m_rcva->refCount;
        }
        
        ~SlicedArray()
        {
          if ( m_rcva && --m_rcva->refCount == 0 )
          {
            m_rcva->varArray.dispose();
            ( *s_callbacks.m_free )( m_rcva );
          }
        }
      
        Member const &operator[]( size_t index ) const
        {
          return m_rcva->varArray[m_offset + index];
        }
      
        Member &operator[]( size_t index )
        {
          return m_rcva->varArray[m_offset + index];
        }
      
        size_t size() const
        {
          return m_size;
        }
      
      private:
    
        size_t m_offset;
        size_t m_size;
        RCVA *m_rcva;
      }
      FABRIC_EXT_DECL_END;
    };
  };
};

#define IMPLEMENT_FABRIC_EDK_ENTRIES_BASE \
  FABRIC_EXT_EXPORT void FabricEDKInit( const Fabric::EDK::Callbacks& callbacks ) \
  { \
    Fabric::EDK::s_callbacks = callbacks; \
  }

#if defined( FABRIC_RC_LEAK_REPORT ) 
# define IMPLEMENT_FABRIC_EDK_ENTRIES IMPLEMENT_FABRIC_EDK_ENTRIES_BASE \
  namespace Fabric \
  { \
    namespace RC \
    { \
      int _LeakReportRegisterClass(const char* file, int line){return 0;} \
      void _LeakReportClassRetain( int ID ){} \
      void _LeakReportClassRelease( int ID ){} \
      void _ReportLeaks(){} \
    } \
  }
#else
# define IMPLEMENT_FABRIC_EDK_ENTRIES IMPLEMENT_FABRIC_EDK_ENTRIES_BASE
#endif

#endif //_FABRIC_EDK_H

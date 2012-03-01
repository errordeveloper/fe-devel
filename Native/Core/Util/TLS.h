/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_TLS_H
#define _FABRIC_UTIL_TLS_H

#if defined( FABRIC_OS_WINDOWS )
#include <windows.h>
#else
#include <pthread.h>
#endif
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace Util
  {
    template < typename T > class TLSVar
    {
    public:
    
      TLSVar( T const &defaultValue = 0 )
        : m_defaultValue( defaultValue )
      {
#if defined( FABRIC_OS_WINDOWS )
        if( ( m_tlsIndex = ::TlsAlloc( ) ) == TLS_OUT_OF_INDEXES )
          throw Exception("Failed to create TLS slot");
        ::TlsSetValue( m_tlsIndex, NULL );
#else
        if ( pthread_key_create( &m_pthreadKey, &TLSVar::Destructor ) )
          throw Exception("Failed to create TLS slot");
#endif
      }
      
      ~TLSVar()
      {
#if defined( FABRIC_OS_WINDOWS )
        ::TlsFree( m_tlsIndex );
#else
        pthread_key_delete( m_pthreadKey );
#endif
      }
      
      T const &get() const
      {
        return *valuePtrForThread();
      }
      
      operator T const &() const
      {
        return get();
      }
      
      operator T &()
      {
        return *valuePtrForThread();
      }
      
      TLSVar &operator =( T const &t )
      {
        *valuePtrForThread() = t;
        return *this;
      }
      
      class Setter
      {
      public:
      
        Setter( TLSVar &tlsVar, T const &newValue )
          : m_tlsVar( tlsVar )
        {
          m_oldValue = m_tlsVar;
          m_tlsVar = newValue;
        }
        
        ~Setter()
        {
          m_tlsVar = m_oldValue;
        }
      
      private:
      
        TLSVar &m_tlsVar;
        T m_oldValue;
      };
      
    protected:
    
      T *valuePtrForThread() const
      {
#if defined( FABRIC_OS_WINDOWS )
        T *val = (T *)::TlsGetValue( m_tlsIndex );
#else
        T *val = (T *)pthread_getspecific( m_pthreadKey );
#endif
        if ( !val )
        {
          val = new T( m_defaultValue );
#if defined( FABRIC_OS_WINDOWS )
          ::TlsSetValue( m_tlsIndex, val );
#else
          pthread_setspecific( m_pthreadKey, val );
#endif
        }
        return val;
      }
      
      static void Destructor( void *value )
      {
        delete (T *)value;
      }
        
    private:
    
#if defined( FABRIC_OS_WINDOWS )
      DWORD   m_tlsIndex;
#else
      pthread_key_t m_pthreadKey;
#endif
      T m_defaultValue;
    };
  };
};

#endif //_FABRIC_UTIL_TLS_H

/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_AUTO_PTR_H
#define _FABRIC_UTIL_AUTO_PTR_H

namespace Fabric
{
  namespace Util
  {
    template< class T > class AutoPtr
    {
    public:
    
      AutoPtr()
        : m_ptr( 0 )
      {
      }
      
      AutoPtr( T *ptr )
        : m_ptr( ptr )
      {
      }
      
      AutoPtr &operator =( T *ptr )
      {
        delete m_ptr;
        m_ptr = ptr;
        return *this;
      }
      
      T *ptr()
      {
        return m_ptr;
      }
      
      T *operator ->()
      {
        return m_ptr;
      }

      T &operator *()
      {
        return *m_ptr;
      }

      operator bool() const
      {
        return m_ptr != 0;
      }
      
      bool operator !() const
      {
        return !m_ptr;
      }
      
      ~AutoPtr()
      {
        delete m_ptr;
      }
      
    private:
    
      T *m_ptr;
    };
  };
};

#endif //_FABRIC_UTIL_AUTO_PTR_H

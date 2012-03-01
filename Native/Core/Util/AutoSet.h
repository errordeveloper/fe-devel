/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_AUTO_SET_H
#define _FABRIC_UTIL_AUTO_SET_H

namespace Fabric
{
  namespace Util
  {
    template< class T > class AutoSet
    {
    public:
    
      AutoSet( T &lValue, T newRValue )
        : m_lValue( lValue )
        , m_oldRValue( lValue )
      {
        m_lValue = newRValue;
      }
      
      AutoSet()
      {
        m_lValue = m_oldRValue;
      }
      
    private:
    
      T &m_lValue;
      T m_oldRValue;
    };
  };
};

#endif //_FABRIC_UTIL_AUTO_SET_H

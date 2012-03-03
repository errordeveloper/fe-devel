/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/WeakContainer.h>

namespace Fabric
{
  namespace RC
  {
    void Object::retain() const
    {
      m_refCount.increment();
#if defined( FABRIC_RC_LEAK_REPORT )
      if(m_leakClassID == 0)
        _LeakReportClassRetain( 0 );
#endif
    }
    
    void Object::release() const
    {
#if defined( FABRIC_RC_LEAK_REPORT )
      if(m_leakClassID == 0)
        _LeakReportClassRelease( 0 );
#endif
      if ( m_refCount.decrementAndGetValue() == 0 )
      {
        if ( m_weakContainer )//[JeromeCG 20120224] Invalidate weak containers before destructors to avoid double release from "makeStrong"
          m_weakContainer->invalidate();

        delete this;
      }
    }
    
    Object::Object()
      : m_refCount( 0 )
      , m_weakContainer( 0 )
#if defined( FABRIC_RC_LEAK_REPORT )
      , m_leakClassID( 0 )
#endif
    {
    }

    Object::~Object()
    {
      if ( m_weakContainer )
        m_weakContainer->release();
    }

#if defined( FABRIC_RC_LEAK_REPORT )
    void Object::_setLeakClassID( int ID ) const
    {
      //[JeromeCG 20120222] This is required because a "retain" might happen in a parent class constructor, so it would be assigned to a different class ID...
      int prevID = m_leakClassID;
      size_t prevRefCount = m_refCount.getValue();
      m_leakClassID = ID;//Ideally it would be an atomic swap..
      if(prevID != ID)
      {
        while(prevRefCount--)
        {
          _LeakReportClassRelease( prevID );
          _LeakReportClassRetain( ID );
        }
      }
    }
#endif

    WeakContainer *Object::weakContainer() const
    {
      if ( !m_weakContainer )
      {
        m_weakContainer = new WeakContainer( const_cast<Object *>(this) );
        m_weakContainer->retain();
      }
      return m_weakContainer;
    }
  };
};

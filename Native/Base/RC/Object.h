/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RC_OBJECT_H
#define _FABRIC_RC_OBJECT_H

#include <Fabric/Base/Util/AtomicSize.h>

namespace Fabric
{
  namespace RC
  {
    class WeakContainer;
    template < class RCO > class WeakHandle;
    template < class RCO > class WeakConstHandle;
    
    class Object
    {
      template < class RCO > friend class WeakHandle;
      template < class RCO > friend class WeakConstHandle;

    public:
    
#if defined( FABRIC_RC_LEAK_REPORT ) 
      virtual void retain() const;
      virtual void release() const;
      void _setLeakClassID( int ID ) const;
      mutable volatile int m_leakClassID;
#else
      void retain() const;
      void release() const;
#endif      
    protected:
    
      Object();
      virtual ~Object();
    
      WeakContainer *weakContainer() const;

    private:
      
      mutable Util::AtomicSize m_refCount;

      mutable WeakContainer *m_weakContainer;
    };

#if defined( FABRIC_RC_LEAK_REPORT )
    int _LeakReportRegisterClass(const char* file, int line);
    void _LeakReportClassRetain( int ID );
    void _LeakReportClassRelease( int ID );
    void _ReportLeaks();
#endif
  };
};

#if defined( FABRIC_RC_LEAK_REPORT )
# define REPORT_RC_LEAKS \
    virtual void retain() const \
    { \
      static int ID = -1; \
      if( ID == -1 ) \
        ID = RC::_LeakReportRegisterClass(__FILE__, __LINE__); \
      RC::Object::_setLeakClassID( ID ); \
      RC::_LeakReportClassRetain( ID ); \
      RC::Object::retain(); \
    } \
    virtual void release() const \
    { \
      static int ID = -1; \
      if( ID == -1 ) \
        ID = RC::_LeakReportRegisterClass(__FILE__, __LINE__); \
      RC::Object::_setLeakClassID( ID ); \
      RC::_LeakReportClassRelease( ID ); \
      RC::Object::release(); \
    }
#else
# define REPORT_RC_LEAKS
#endif

#endif //_FABRIC_RC_OBJECT_H

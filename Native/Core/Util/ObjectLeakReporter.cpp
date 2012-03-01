/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */

#include <Fabric/Base/RC/Object.h>

#if defined( FABRIC_RC_LEAK_REPORT )

#include <Fabric/Core/Util/Mutex.h>
#include <Fabric/Base/Util/Assert.h>
#include <map>
#include <string>
#include <sstream>

//[JeromeCG 20120222] This should be in Base/RC, but it depends on FABRIC_LOG which is in DG/Util..
namespace Fabric
{
  namespace RC
  {
    const size_t kMaxNbClasses = 2000;

    class ObjectLeakReporter
    {
      std::map<std::string, int> m_classToIDMap;
      Util::Mutex m_classToIDMapMutex;

      Util::AtomicSize m_classRefCounts[ kMaxNbClasses ];//Static max size, but avoids mutex requirements

    public:
      ObjectLeakReporter()
        : m_classToIDMapMutex("ObjectLeakReporter::m_classToIDMapMutex")
      {
        for( size_t i = 0; i < kMaxNbClasses; ++i )
          m_classRefCounts[i] = 0;
        m_classToIDMap["RC::Object"] = 0;
      }

      int registerClass(const char* file, const int line)
      {
        Util::Mutex::Lock lock( m_classToIDMapMutex );
        std::stringstream s;
        s << file << "#" << line;
        if( m_classToIDMap.find( s.str() ) == m_classToIDMap.end() )
        {
          int ID = (int)m_classToIDMap.size();
          m_classToIDMap[s.str()] = ID;
          FABRIC_ASSERT( (size_t)ID < kMaxNbClasses );
        }
        return m_classToIDMap[s.str()];
      }

      void classRetain( int ID )
      {
        FABRIC_ASSERT( ID >= 0 && size_t( ID ) < kMaxNbClasses );
        m_classRefCounts[ ID ].increment();
      }

      void classRelease( int ID )
      {
        FABRIC_ASSERT( ID >= 0 && size_t( ID ) < kMaxNbClasses );
        m_classRefCounts[ ID ].decrement();
        FABRIC_ASSERT(int(m_classRefCounts[ ID ].getValue()) >= 0);
      }

      void reportLeaks()
      {
        //[JeromeCG 20120222] Note: leaks cannot be reported in the destructor, since global objects are destructed in arbitrary order, 
        //                    and we depend on FABRIC_LOG which itself uses a global object in Windows.
        size_t nbLeakingClasses = 0;
        for( std::map<std::string, int>::const_iterator it = m_classToIDMap.begin(); it != m_classToIDMap.end(); ++it )
        {
          if( m_classRefCounts[ it->second ].getValue() )
          {
            FABRIC_LOG("Leak detected: %d unreleased references to %s", m_classRefCounts[ it->second ].getValue(),  it->first.c_str() );
            ++nbLeakingClasses;
          }
        }
        if( nbLeakingClasses )
        {
          FABRIC_LOG("Nb of leaking RC classes: %d", nbLeakingClasses );
        }
        else
        {
          FABRIC_LOG("No RC classes are leaking" );
        }
      }
    };

    ObjectLeakReporter g_objectLeakReporter;

    int _LeakReportRegisterClass(const char* file, const int line)
    {
      return g_objectLeakReporter.registerClass(file, line);
    }

    void _LeakReportClassRetain( int ID )
    {
      g_objectLeakReporter.classRetain( ID );
    }

    void _LeakReportClassRelease( int ID )
    {
      g_objectLeakReporter.classRelease( ID );
    }

    void _ReportLeaks()
    {
      g_objectLeakReporter.reportLeaks();
    }
  };
};

#endif

/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MT/Util.h>

#if defined(FABRIC_POSIX)
# include <stdlib.h>
# include <sys/types.h>
# include <sys/sysctl.h>
#elif defined(FABRIC_WIN32)
# include <windows.h>
# include <process.h>
#endif
#if defined(FABRIC_OS_LINUX)
# include <sys/prctl.h>
#endif

namespace Fabric
{
  namespace MT
  {
    size_t getNumCores()
    {
      static size_t numCores = 0;
      if ( !numCores )
      {
#if defined(FABRIC_OS_MACOSX)
        int32_t numCoresInt32;
        
        int mib[4];
        size_t len = sizeof(numCoresInt32); 

        mib[0] = CTL_HW;
        mib[1] = HW_AVAILCPU;

        /* get the number of CPUs from the system */
        if ( sysctl( mib, 2, &numCoresInt32, &len, NULL, 0 ) != 0 || numCores < 1 )
        {
          mib[1] = HW_NCPU;
          if ( sysctl( mib, 2, &numCoresInt32, &len, NULL, 0 ) != 0 || numCores < 1 )
            numCores = 1;
        }
          
        numCores = (size_t)numCoresInt32;
#elif defined(FABRIC_OS_LINUX)
        numCores = std::max<size_t>( 1, sysconf( _SC_NPROCESSORS_ONLN ) );
#elif defined(FABRIC_OS_WINDOWS)
        SYSTEM_INFO sysInfo;
        ::ZeroMemory( &sysInfo, sizeof( sysInfo ) );
        ::GetSystemInfo( &sysInfo );
        numCores = std::max<size_t>( 1, sysInfo.dwNumberOfProcessors );
#else
# error "Missing implementation for getNumCores()"
#endif

        char const *envMaxThreads = getenv( "FABRIC_MAX_THREADS" );
        if ( envMaxThreads )
          numCores = std::min<size_t>( numCores, std::max<size_t>( 1, atol(envMaxThreads) ) );
      }
      return numCores;
    }
  };
};

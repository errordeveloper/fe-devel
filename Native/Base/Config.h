/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_CONFIG_H
#define _FABRIC_UTIL_CONFIG_H

#if defined( __linux__ )

# define FABRIC_OS_LINUX         
# define FABRIC_POSIX

#elif defined( __APPLE__ )

# define FABRIC_OS_MACOSX
# define FABRIC_POSIX

#elif defined( _MSC_VER )

# define FABRIC_OS_WINDOWS
# define FABRIC_WIN32

# if !defined( FABRIC_INCLUDE_WINDOWS_UI )
#  define WIN32_LEAN_AND_MEAN   // Avoid unnecessary cruft
# endif
# define NOMINMAX              // Don't define min/max as macros, stupid
# if !defined( WIN32 )
#  define WIN32 1
# endif

// For some reason, this warning cannot be disabled
// from the command line.
// 'Unsupported zero-sized arrays in structs blah...'
#pragma warning( disable: 4200 )


// Some missing POSIX-y functions
#include <math.h>
static inline double round( double x ) { return floor(x+0.5); }
static inline float roundf( float x ) { return floorf(x+0.5f); }
#define snprintf _snprintf

// And some odd warnings
#pragma warning( disable: 4355 )

#else
# error Unknown platform.
#endif

// Build architecture
#if defined(FABRIC_POSIX)
# if defined(__x86_64)
#  define FABRIC_ARCH_64BIT
# else
#  define FABRIC_ARCH_32BIT
# endif
#else
// [pzion 20111014] FIXME: we should properly detect Windows bit architecture
# define FABRIC_ARCH_32BIT
#endif

// Build settings
#if defined(NDEBUG)
# define FABRIC_BUILD_RELEASE
#else
# define FABRIC_BUILD_DEBUG
#endif

// NPAPI export
#if defined(FABRIC_OS_WINDOWS)
// NB. NPAPI exports on Windows are handled through the FabricPlugin.def file.
// Otherwise the linker barfs.
# define FABRIC_NPAPI_EXPORT
#else
# define FABRIC_NPAPI_EXPORT __attribute__ ((visibility("default")))
#endif

// SDK export
#if defined(FABRIC_OS_WINDOWS)
# define FABRIC_SDK_EXPORT __declspec(dllexport)
#else
# define FABRIC_SDK_EXPORT __attribute__ ((visibility("default")))
#endif

#define FABRIC_CLI_EXPORT FABRIC_SDK_EXPORT

#include <string>

namespace Fabric
{
  namespace Util
  {
    std::string const &getHostTriple();
    
    bool matchHostTriple( std::string const &hostTriple, std::string const &pattern );
  };
};

#endif // _FABRIC_UTIL_CONFIG_H

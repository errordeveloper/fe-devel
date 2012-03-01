/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_PLUG_HELPERS_H
#define _FABRIC_PLUG_HELPERS_H
 
#include <Fabric/Base/Config.h>
#include <Fabric/Base/RC/ConstHandle.h>

#include <map>
#include <vector>
#include <string>

#if defined(FABRIC_OS_WINDOWS)
# include <windows.h>
#endif


namespace Fabric
{
  namespace Plug
  {
#if defined(FABRIC_OS_MACOSX) || defined(FABRIC_OS_LINUX)
    typedef void *SOLibHandle;
#elif defined(FABRIC_OS_WINDOWS)
    typedef HMODULE SOLibHandle;
#else
# error "Unsupported platform"
#endif
    extern SOLibHandle const invalidSOLibHandle;

    SOLibHandle SOLibOpen( std::string const &name, std::string &resolvedName, bool global, std::vector<std::string> const &pluginDirs );
    void *SOLibResolve( SOLibHandle soLibHandle, std::string const &functionName );
    void SOLibClose( SOLibHandle soLibHandle, std::string const &resolvedName );
  };
};

#endif //_FABRIC_PLUG_HELPERS_H

/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Base/Config.h>
#include <Fabric/Base/Util/Log.h>
#include <Fabric/Core/Util/Debug.h>
#include <Fabric/Core/Build.h>
#include <Fabric/Core/MT/Impl.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Core/DG/IRCache.h>

#include <npapi/npapi.h>
#include <npapi/npfunctions.h>
#include <llvm/Support/Threading.h>
#include <stdlib.h>

#if defined(FABRIC_OS_LINUX)
namespace Fabric
{
  namespace NPAPI
  {
    extern NPError NPP_GetValue(NPP instance, NPPVariable variable, void* value);
  };
};
#endif

extern "C"
{
FABRIC_NPAPI_EXPORT NPError OSCALL NP_GetEntryPoints(NPPluginFuncs* plugin_funcs)
{
  extern NPError InitializePluginFunctions(NPPluginFuncs* plugin_funcs);
  return InitializePluginFunctions(plugin_funcs);
}

static void llvmInitialize()
{
  llvm::llvm_start_multithreaded();
  if ( !llvm::llvm_is_multithreaded() )
  {
    FABRIC_LOG( "LLVM not compiled with multithreading enabled; aborting" );
    abort();
  }
}

static void displayHeader()
{
  FABRIC_LOG( "%s version %s", Fabric::buildName, Fabric::buildFullVersion );
  FABRIC_LOG( "Plugin loaded." );
}

#if defined(FABRIC_OS_LINUX)
FABRIC_NPAPI_EXPORT NPError OSCALL NP_Initialize(NPNetscapeFuncs* browser_functions,
                      NPPluginFuncs* plugin_functions)
{
  displayHeader();
  FABRIC_DEBUG_LOG( "Debug with: gdb --pid=%d", getpid() );
  llvmInitialize();
  extern void InitializeBrowserFunctions(NPNetscapeFuncs* browser_functions);
  InitializeBrowserFunctions(browser_functions);
  NPError np_err = NP_GetEntryPoints(plugin_functions);
  return np_err;
}
#elif defined(FABRIC_OS_MACOSX)
FABRIC_NPAPI_EXPORT NPError OSCALL NP_Initialize(NPNetscapeFuncs* browser_functions)
{
  displayHeader();
  FABRIC_DEBUG_LOG( "Debug with: gdb --pid=%d", getpid() );
  llvmInitialize();
  extern void InitializeBrowserFunctions(NPNetscapeFuncs* browser_functions);
  InitializeBrowserFunctions(browser_functions);
  return NPERR_NO_ERROR;
}
#elif defined(FABRIC_OS_WINDOWS)
FABRIC_NPAPI_EXPORT NPError OSCALL NP_Initialize(NPNetscapeFuncs* browser_functions)
{
  displayHeader();
  llvmInitialize();
  extern void InitializeBrowserFunctions(NPNetscapeFuncs* browser_functions);
  InitializeBrowserFunctions(browser_functions);
  return NPERR_NO_ERROR;
}
#else
# error "Unrecognized FABRIC_OS_..."
#endif

FABRIC_NPAPI_EXPORT NPError OSCALL NP_Shutdown()
{
  Fabric::MT::ThreadPool::Instance()->terminate();
  llvm::llvm_stop_multithreaded();

  Fabric::Plug::Manager::Terminate();
  Fabric::DG::IRCache::Terminate();

#if defined( FABRIC_RC_LEAK_REPORT )
  Fabric::RC::_ReportLeaks();
#endif
  
  return NPERR_NO_ERROR;
}

#if defined(FABRIC_OS_LINUX)
FABRIC_NPAPI_EXPORT NPError OSCALL NP_GetValue(NPP instance, NPPVariable variable, void* value)
{
  return Fabric::NPAPI::NPP_GetValue(instance, variable, value);
}

FABRIC_NPAPI_EXPORT const char * OSCALL NP_GetPluginVersion(void)
{
  return Fabric::buildFullVersion;
}

FABRIC_NPAPI_EXPORT const char* OSCALL NP_GetMIMEDescription(void)
{
  return "application/fabric::Fabric application";
}
#endif

}  // extern "C"

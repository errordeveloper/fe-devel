/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_OCL_H
#define _FABRIC_OCL_H

#include <Fabric/Base/Config.h>

#if defined(FABRIC_MODULE_OPENCL)
# include <Fabric/Base/RC/Handle.h>

# include <string>

namespace Fabric
{
  namespace RT
  {
    class Manager;
  };
  
  namespace CG
  {
    class ModuleBuilder;
  };
  
  namespace OCL
  {
    void registerTypes( RC::Handle<RT::Manager> const &rtManager );
    void llvmPrepareModule( CG::ModuleBuilder &moduleBuilder, RC::Handle<RT::Manager> const &rtManager );
    void *llvmResolveExternalFunction( std::string const &name );
  };
};
#endif

#endif //_FABRIC_OCL_H

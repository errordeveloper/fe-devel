/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_USE_GLOBAL_H
#define _FABRIC_AST_USE_GLOBAL_H

#include <Fabric/Core/AST/Global.h>

namespace Fabric
{
  namespace CG
  {
    class Adapter;
    class Manager;
  };
  
  namespace AST
  {
    class UseVector;
    class UseInfoSet;
    
    class UseGlobal : public Global
    {
      FABRIC_AST_NODE_DECL( UseGlobal );
      
    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<UseGlobal> Create(
        CG::Location const &location,
        RC::ConstHandle<UseVector> const &useVector
        );
      
      virtual void collectUses( UseNameToLocationMap &uses ) const;
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      virtual void llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const;
      
    protected:
    
      UseGlobal(
        CG::Location const &location,
        RC::ConstHandle<UseVector> const &useVector
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      RC::ConstHandle<UseVector> m_useVector;
    };
  };
};

#endif //_FABRIC_AST_USE_GLOBAL_H

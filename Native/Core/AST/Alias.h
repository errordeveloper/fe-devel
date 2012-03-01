/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_ALIAS_H
#define _FABRIC_AST_ALIAS_H

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
    class Alias : public Global
    {
      FABRIC_AST_NODE_DECL( Alias );
      
    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<Alias> Create(
        CG::Location const &location,
        std::string const &name,
        std::string const &adapterName
        );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      virtual void llvmCompileToModule( CG::ModuleBuilder &moduleBuilder, CG::Diagnostics &diagnostics, bool buildFunctionBodies ) const;
      
    protected:
    
      Alias(
        CG::Location const &location,
        std::string const &name,
        std::string const &adapterName
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      std::string m_name;
      std::string m_adapterName;
    };
  };
};

#endif //_FABRIC_AST_ALIAS_H

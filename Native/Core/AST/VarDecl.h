/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_VAR_DECL_H
#define _FABRIC_AST_VAR_DECL_H

#include <Fabric/Core/AST/Node.h>
#include <Fabric/Core/CG/ExprValue.h>

namespace Fabric
{
  namespace CG
  {
    class BasicBlockBuilder;
    class Diagnostics;
    class Manager;
  };
  
  namespace AST
  {
    class VarDecl : public Node
    {
    public:
      REPORT_RC_LEAKS

      virtual void registerTypes( std::string const &baseType, RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;

      virtual void llvmCompileToBuilder( std::string const &baseType, CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const = 0;
     
    protected:
    
      VarDecl(
        CG::Location const &location,
        std::string const &name,
        std::string const &arrayModifier
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
      CG::ExprValue llvmAllocateVariable( std::string const &baseType, CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;

      RC::ConstHandle<CG::Adapter> getAdapter( std::string const &baseType, RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
    private:
    
      std::string m_name;
      std::string m_arrayModifier;
    };
  }
}

#endif //_FABRIC_AST_VAR_DECL_H

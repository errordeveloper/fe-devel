/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_CONST_DECL_STATEMENT_H
#define _FABRIC_AST_CONST_DECL_STATEMENT_H

#include <Fabric/Core/AST/Statement.h>

namespace Fabric
{
  namespace CG
  {
    class Adapter;
  };
  
  namespace AST
  {
    class ConstDecl;
    
    class ConstDeclStatement : public Statement
    {
      FABRIC_AST_NODE_DECL( ConstDeclStatement );

    public:
      REPORT_RC_LEAKS

      static RC::ConstHandle<ConstDeclStatement> Create(
        CG::Location const &location,
        RC::ConstHandle<ConstDecl> const &constDecl
        );
      
      virtual void registerTypes( RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const;
      
      virtual void llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;
      
    protected:
    
      ConstDeclStatement(
        CG::Location const &location,
        RC::ConstHandle<ConstDecl> const &constDecl
        );
      
      virtual void appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const;
    
    private:
    
      RC::ConstHandle<ConstDecl> m_constDecl;
    };
  };
};

#endif //_FABRIC_AST_CONST_DECL_STATEMENT_H

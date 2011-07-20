/*
 *
 *  Created by Peter Zion on 10-12-02.
 *  Copyright 2010 Fabric Technologies Inc. All rights reserved.
 *
 */

#ifndef _FABRIC_AST_VAR_DECL_H
#define _FABRIC_AST_VAR_DECL_H

#include <Fabric/Core/AST/Statement.h>
#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace AST
  {
    class VarDecl: public Statement
    {
      FABRIC_AST_NODE_DECL( VarDecl );
      
    public:

      static RC::Handle<VarDecl> Create(
        CG::Location const &location,
        std::string const &name,
        std::string const &type
        );

      RC::Handle<JSON::Object> toJSON() const;
      
      std::string const &getType() const
      {
        return m_type;
      }
      
      virtual void llvmCompileToBuilder( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;
     
    protected:
    
      VarDecl(
        CG::Location const &location,
        std::string const &name,
        std::string const &type
        );
    
      CG::ExprValue llvmAllocateVariable( CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;

    private:
    
      std::string m_name;
      std::string m_type;
    };
  };
};

#endif //_FABRIC_AST_VAR_DECL_H

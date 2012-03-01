/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_AST_NAKED_VAR_DECL_H
#define _FABRIC_AST_NAKED_VAR_DECL_H

#include <Fabric/Core/AST/VarDecl.h>
#include <Fabric/Core/AST/Expr.h>

namespace Fabric
{
  namespace AST
  {
    class ExprVector;
    
    class NakedVarDecl: public VarDecl
    {
      FABRIC_AST_NODE_DECL( NakedVarDecl );

    public:

      static RC::ConstHandle<NakedVarDecl> Create(
        CG::Location const &location,
        std::string const &name,
        std::string const &arrayModifier
        );

      virtual void llvmCompileToBuilder( std::string const &baseType, CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const;
     
    protected:
    
      NakedVarDecl(
        CG::Location const &location,
        std::string const &name,
        std::string const &arrayModifier
        );
    };
  }
}

#endif //_FABRIC_AST_NAKED_VAR_DECL_H

/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/AST/NakedVarDecl.h>
#include <Fabric/Core/CG/Function.h>
#include <Fabric/Core/CG/Mangling.h>
#include <Fabric/Core/CG/ModuleBuilder.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( NakedVarDecl );
    
    RC::ConstHandle<NakedVarDecl> NakedVarDecl::Create(
      CG::Location const &location,
      std::string const &name,
      std::string const &arrayModifier
      )
    {
      return new NakedVarDecl( location, name, arrayModifier );
    }
    
    NakedVarDecl::NakedVarDecl(
      CG::Location const &location,
      std::string const &name,
      std::string const &arrayModifier
      )
      : VarDecl( location, name, arrayModifier )
    {
    }

    void NakedVarDecl::llvmCompileToBuilder( std::string const &baseType, CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      CG::ExprValue result = VarDecl::llvmAllocateVariable( baseType, basicBlockBuilder, diagnostics );
      FABRIC_ASSERT( result.getUsage() == CG::USAGE_LVALUE );
      
      CG::Function const *function = basicBlockBuilder.getModuleBuilder().maybeGetFunction(
        getLocation(),
        CG::ConstructorPencilKey( result.getAdapter() ),
        CG::ExprTypeVector(
          result.getExprType()
          ),
        CG::ConstructorQueryDesc(
          result.getAdapter(),
          CG::AdapterVector()
          )
        );
      if ( function )
      {
        std::vector<CG::ExprValue> exprValues;
        exprValues.push_back( result );
        function->llvmCreateCall( basicBlockBuilder, exprValues );
      }
    }
  }
}

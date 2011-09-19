/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "VarDecl.h"
#include <Fabric/Core/CG/Adapter.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/CG/ModuleBuilder.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace AST
  {
    FABRIC_AST_NODE_IMPL( VarDecl );
    
    RC::ConstHandle<VarDecl> VarDecl::Create(
      CG::Location const &location,
      std::string const &name,
      std::string const &arrayModifier
      )
    {
      return new VarDecl( location, name, arrayModifier );
    }
    
    VarDecl::VarDecl(
      CG::Location const &location,
      std::string const &name,
      std::string const &arrayModifier
      )
      : Node( location )
      , m_name( name )
      , m_arrayModifier( arrayModifier )
    {
    }
    
    void VarDecl::appendJSONMembers( Util::JSONObjectGenerator const &jsonObjectGenerator, bool includeLocation ) const
    {
      Node::appendJSONMembers( jsonObjectGenerator, includeLocation );
      jsonObjectGenerator.makeMember( "name" ).makeString( m_name );
      jsonObjectGenerator.makeMember( "arrayModifier" ).makeString( m_arrayModifier );
    }
    
    RC::ConstHandle<CG::Adapter> VarDecl::getAdapter( std::string const &baseType, RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      std::string type = baseType + m_arrayModifier;
      RC::ConstHandle<CG::Adapter> result;
      try
      {
        result = cgManager->getAdapter( type );
      }
      catch ( Exception e )
      {
        addError( diagnostics, e );
      }
      return result;
    }
    
    void VarDecl::registerTypes( std::string const &baseType, RC::Handle<CG::Manager> const &cgManager, CG::Diagnostics &diagnostics ) const
    {
      getAdapter( baseType, cgManager, diagnostics );
    }

    void VarDecl::llvmCompileToBuilder( std::string const &baseType, CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      llvmAllocateVariable( baseType, basicBlockBuilder, diagnostics );
    }

    CG::ExprValue VarDecl::llvmAllocateVariable( std::string const &baseType, CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = getAdapter( baseType, basicBlockBuilder.getManager(), diagnostics );
      adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      
      llvm::Value *result = adapter->llvmAlloca( basicBlockBuilder, m_name );
      adapter->llvmInit( basicBlockBuilder, result );
      
      CG::Scope &scope = basicBlockBuilder.getScope();
      if ( scope.has( m_name ) )
        addError( diagnostics, ("variable '" + m_name + "' already exists").c_str() );
      else scope.put( m_name, CG::VariableSymbol::Create( CG::ExprValue( adapter, CG::USAGE_LVALUE, basicBlockBuilder.getContext(), result ) ) );
        
      return CG::ExprValue( adapter, CG::USAGE_LVALUE, basicBlockBuilder.getContext(), result );
    }
  };
};

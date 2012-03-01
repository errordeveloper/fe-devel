/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
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
    
    void VarDecl::appendJSONMembers( JSON::ObjectEncoder const &jsonObjectEncoder, bool includeLocation ) const
    {
      Node::appendJSONMembers( jsonObjectEncoder, includeLocation );
      jsonObjectEncoder.makeMember( "name" ).makeString( m_name );
      jsonObjectEncoder.makeMember( "arrayModifier" ).makeString( m_arrayModifier );
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

    CG::ExprValue VarDecl::llvmAllocateVariable( std::string const &baseType, CG::BasicBlockBuilder &basicBlockBuilder, CG::Diagnostics &diagnostics ) const
    {
      RC::ConstHandle<CG::Adapter> adapter = getAdapter( baseType, basicBlockBuilder.getManager(), diagnostics );
      adapter->llvmCompileToModule( basicBlockBuilder.getModuleBuilder() );
      
      llvm::Value *result = adapter->llvmAlloca( basicBlockBuilder, m_name );
      adapter->llvmInit( basicBlockBuilder, result );
      
      CG::Scope &scope = basicBlockBuilder.getScope();
      if ( scope.hasLocal( m_name ) )
        addError( diagnostics, ("variable " + _(m_name) + " already exists").c_str() );
      else scope.put( m_name, CG::VariableSymbol::Create( CG::ExprValue( adapter, CG::USAGE_LVALUE, basicBlockBuilder.getContext(), result ) ) );
        
      return CG::ExprValue( adapter, CG::USAGE_LVALUE, basicBlockBuilder.getContext(), result );
    }
  };
};

/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "ModuleBuilder.h"
#include "Manager.h"
#include "Error.h"
#include "Location.h"

#include <llvm/Function.h>

namespace Fabric
{
  namespace CG
  {
    ModuleBuilder::ModuleBuilder(
      RC::Handle<Manager> const &manager,
      RC::Handle<Context> const &context,
      llvm::Module *module,
      CompileOptions const *compileOptions
      )
      : m_manager( manager )
      , m_context( context )
      , m_module( module )
      , m_compileOptions( compileOptions )
    {
    }
    
    RC::Handle<Manager> ModuleBuilder::getManager()
    {
      return m_manager;
    }
    
    RC::ConstHandle<Adapter> ModuleBuilder::maybeGetAdapter( std::string const &userName ) const
    {
      return m_manager->maybeGetAdapter( userName );
    }
    
    RC::ConstHandle<Adapter> ModuleBuilder::getAdapter( std::string const &userName, CG::Location const &location )
    {
      RC::ConstHandle<Adapter> result = maybeGetAdapter( userName );
      if ( !result )
        throw CG::Error( location, _(userName) + ": type not registered" );
      return result;
    }
    
    RC::Handle<Context> ModuleBuilder::getContext()
    {
      return m_context;
    }
    
    llvm::Module *ModuleBuilder::getModule()
    {
      return m_module;
    }
    
    ModuleBuilder::operator llvm::Module *()
    {
      return getModule();
    }
    
    llvm::Module *ModuleBuilder::operator ->()
    {
      return getModule();
    }
      
    ModuleScope &ModuleBuilder::getScope()
    {
      return m_moduleScope;
    }
    
    bool ModuleBuilder::haveCompiledToModule( std::string const &codeName )
    {
      bool insertResult = m_haveCompiledToModule.insert( codeName ).second;
      return !insertResult;
    }

    RC::ConstHandle<PencilSymbol> ModuleBuilder::addFunction(
      std::string const &pencilName,
      CG::Function const &function
      )
    {
      FABRIC_ASSERT( !pencilName.empty() );
      
      RC::Handle<PencilSymbol> &pencil = m_pencils[pencilName];
      if ( !pencil )
        pencil = PencilSymbol::Create( pencilName );
      CG::Function const *existingFunction = pencil->maybeGetPreciseFunction( function.getParamTypes() );
      if ( existingFunction )
      {
        if ( existingFunction->getLLVMFunction() != function.getLLVMFunction() )
          throw Exception( pencilName + "(" + existingFunction->getParamTypes().desc() + ") already exists" );
      }
      else pencil->add( function );
      
      return pencil;
    }
    
    CG::Function const *ModuleBuilder::maybeGetPreciseFunction( std::string const &pencilName, ExprTypeVector const &argTypes ) const
    {
      FABRIC_ASSERT( !pencilName.empty() );
      
      Pencils::const_iterator it = m_pencils.find( pencilName );
      if ( it == m_pencils.end() )
        return 0;
      RC::Handle<PencilSymbol> const &pencil = it->second;
      FABRIC_ASSERT( pencil );
      
      return pencil->maybeGetPreciseFunction( argTypes );
    }
    
    CG::Function const *ModuleBuilder::getPreciseFunction( CG::Location const &location, std::string const &pencilName, ExprTypeVector const &argTypes ) const
    {
      CG::Function const *result = maybeGetPreciseFunction( pencilName, argTypes );
      if ( !result )
        throw CG::Error( location, "no such " + pencilName + "(" + argTypes.desc() + ")" );
      return result;
    }
    
    CG::Function const *ModuleBuilder::maybeGetFunction( CG::Location const &location, std::string const &pencilName, ExprTypeVector const &argTypes ) const
    {
      FABRIC_ASSERT( !pencilName.empty() );
      
      Pencils::const_iterator it = m_pencils.find( pencilName );
      if ( it == m_pencils.end() )
        return 0;
      RC::Handle<PencilSymbol> const &pencil = it->second;
      FABRIC_ASSERT( pencil );
      
      return pencil->maybeGetFunction( *this, location, argTypes );
    }
    
    CG::Function const *ModuleBuilder::getFunction( CG::Location const &location, std::string const &pencilName, ExprTypeVector const &argTypes ) const
    {
      FABRIC_ASSERT( !pencilName.empty() );
      
      Pencils::const_iterator it = m_pencils.find( pencilName );
      if ( it == m_pencils.end() )
        throw CG::Error( location, "no such " + pencilName + "(" + argTypes.desc() + ")" );
      RC::Handle<PencilSymbol> const &pencil = it->second;
      FABRIC_ASSERT( pencil );
      
      return pencil->getFunction( *this, location, argTypes );
    }
    
    CG::Function const *ModuleBuilder::getUniqueFunction( CG::Location const &location, std::string const &pencilName ) const
    {
      FABRIC_ASSERT( !pencilName.empty() );
      
      Pencils::const_iterator it = m_pencils.find( pencilName );
      if ( it == m_pencils.end() )
        return 0;
      RC::Handle<PencilSymbol> const &pencil = it->second;
      FABRIC_ASSERT( pencil );
      
      return pencil->getUniqueFunction( location );
    }
  }
}

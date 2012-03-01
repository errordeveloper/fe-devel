/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
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
      std::string const &pencilKey,
      CG::Function const &function
      )
    {
      FABRIC_ASSERT( !pencilKey.empty() );
      
      RC::Handle<PencilSymbol> &pencil = m_pencils[pencilKey];
      if ( !pencil )
        pencil = PencilSymbol::Create();
      CG::Function const *existingFunction = pencil->maybeGetPreciseFunction( function.getParamTypes() );
      if ( existingFunction )
      {
        if ( existingFunction->getLLVMFunction() != function.getLLVMFunction() )
        {
          std::string message = function.getDesc() + " already exists";
          if ( existingFunction->getDesc() != function.getDesc() )
            message += " as " + existingFunction->getDesc();
          throw Exception( message );
        }
      }
      else pencil->add( function );
      
      return pencil;
    }
    
    CG::Function const *ModuleBuilder::maybeGetPreciseFunction(
      std::string const &pencilKey,
      ExprTypeVector const &argTypes
      ) const
    {
      FABRIC_ASSERT( !pencilKey.empty() );
      
      Pencils::const_iterator it = m_pencils.find( pencilKey );
      if ( it == m_pencils.end() )
        return 0;
      RC::Handle<PencilSymbol> const &pencil = it->second;
      FABRIC_ASSERT( pencil );
      
      return pencil->maybeGetPreciseFunction( argTypes );
    }
    
    CG::Function const *ModuleBuilder::getPreciseFunction(
      CG::Location const &location,
      std::string const &pencilKey,
      ExprTypeVector const &argTypes,
      std::string const &queryDesc
      ) const
    {
      CG::Function const *result = maybeGetPreciseFunction( pencilKey, argTypes );
      if ( !result )
        throw CG::Error( location, "no such " + queryDesc );
      return result;
    }
    
    CG::Function const *ModuleBuilder::maybeGetFunction(
      CG::Location const &location,
      std::string const &pencilKey,
      ExprTypeVector const &argTypes,
      std::string const &desc
      ) const
    {
      FABRIC_ASSERT( !pencilKey.empty() );
      
      Pencils::const_iterator it = m_pencils.find( pencilKey );
      if ( it == m_pencils.end() )
        return 0;
      RC::Handle<PencilSymbol> const &pencil = it->second;
      FABRIC_ASSERT( pencil );
      
      return pencil->maybeGetFunction( location, *this, argTypes, desc );
    }
    
    CG::Function const *ModuleBuilder::getFunction(
      CG::Location const &location,
      std::string const &pencilKey,
      ExprTypeVector const &argTypes,
      std::string const &desc
      ) const
    {
      FABRIC_ASSERT( !pencilKey.empty() );
      
      Pencils::const_iterator it = m_pencils.find( pencilKey );
      if ( it == m_pencils.end() )
        throw CG::Error( location, "no such " + desc );
      RC::Handle<PencilSymbol> const &pencil = it->second;
      FABRIC_ASSERT( pencil );
      
      return pencil->getFunction( location, *this, argTypes, desc );
    }
    
    CG::Function const *ModuleBuilder::getUniqueFunction(
      CG::Location const &location,
      std::string const &pencilKey,
      std::string const &desc
      ) const
    {
      FABRIC_ASSERT( !pencilKey.empty() );
      
      Pencils::const_iterator it = m_pencils.find( pencilKey );
      if ( it == m_pencils.end() )
        throw CG::Error( location, "no such " + desc );;
      RC::Handle<PencilSymbol> const &pencil = it->second;
      FABRIC_ASSERT( pencil );
      
      return pencil->getUniqueFunction( location, desc );
    }
  }
}

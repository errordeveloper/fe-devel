/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_CG_MODULE_BUILDER_H
#define _FABRIC_CG_MODULE_BUILDER_H

#include <Fabric/Core/CG/PencilSymbol.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Base/RC/Handle.h>

#include <set>
#include <map>

namespace llvm
{
  class LLVMContext;
  class Module;
};

namespace Fabric
{
  namespace CG
  {
    class CompileOptions;
    class Context;
    class Location;
    class Manager;
    
    class ModuleBuilder
    {
    public:
    
      ModuleBuilder(
        RC::Handle<Manager> const &manager,
        RC::Handle<Context> const &context,
        llvm::Module *module,
        CompileOptions const *compileOptions
        );
      
      llvm::Module *getModule();
      operator llvm::Module *();
      llvm::Module *operator ->();
      
      RC::Handle<Manager> getManager();
      
      RC::Handle<Context> getContext();
      
      ModuleScope &getScope();
      
      bool haveCompiledToModule( std::string const &codeName );
      
      RC::ConstHandle<PencilSymbol> addFunction(
        std::string const &pencilKey,
        CG::Function const &function
        );
        
      CG::Function const *maybeGetPreciseFunction(
        std::string const &pencilKey,
        ExprTypeVector const &argTypes
        ) const;
        
      CG::Function const *getPreciseFunction(
        CG::Location const &location,
        std::string const &pencilKey,
        ExprTypeVector const &argTypes,
        std::string const &desc
        ) const;

      CG::Function const *maybeGetFunction(
        CG::Location const &location,
        std::string const &pencilKey,
        ExprTypeVector const &argTypes,
        std::string const &desc
        ) const;
        
      CG::Function const *getFunction(
        CG::Location const &location,
        std::string const &pencilKey,
        ExprTypeVector const &argTypes,
        std::string const &desc
        ) const;
      
      CG::Function const *getUniqueFunction(
        CG::Location const &location,
        std::string const &pencilKey,
        std::string const &desc
        ) const;

      RC::ConstHandle<Adapter> maybeGetAdapter( std::string const &userName ) const;
      RC::ConstHandle<Adapter> getAdapter( std::string const &userName, CG::Location const &location );
      
      std::string getDestructorName( std::string const &typeName )
      {
        return m_destructorNames[typeName];
      }
      void setDestructorName( std::string const &typeName, std::string const &destructorName )
      {
        m_destructorNames[typeName] = destructorName;
      }
      
      CompileOptions const *getCompileOptions() const
      {
        return m_compileOptions;
      }
    
    private:
    
      typedef std::map< std::string, RC::Handle<PencilSymbol> > Pencils;
    
      RC::Handle<Manager> m_manager;     
      RC::Handle<Context> m_context; 
      llvm::Module *m_module;
      ModuleScope m_moduleScope;
      std::set<std::string> m_havePreparedModule;
      std::set<std::string> m_haveCompiledToModule;
      Pencils m_pencils;
      std::map< std::string, std::string > m_destructorNames;
      CompileOptions const *m_compileOptions;
    };
  };
};

#endif //_FABRIC_CG_MODULE_BUILDER_H

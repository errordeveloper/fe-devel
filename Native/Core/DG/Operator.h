/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_OPERATOR_H
#define _FABRIC_DG_OPERATOR_H

#include <Fabric/Core/DG/NamedObject.h>
#include <Fabric/Core/DG/Prototype.h>
#include <Fabric/Core/CG/Diagnostics.h>
#include <Fabric/Core/MT/ParallelCall.h>

#include <set>
#include <llvm/ADT/OwningPtr.h>

namespace llvm
{
  class LLVMContext;
  class Module;
  class Function;
  class Value;
  class ExecutionEngine;
};

namespace Fabric
{
  namespace RT
  {
    class Manager;
  };
  
  namespace CG
  {
    class Manager;
  };
  
  namespace AST
  {
    class GlobalList;
    class Operator;
  };
  
  namespace DG
  {
    class Context;
    class CompiledObject;
    class BindingList;
    class Binding;
    class Scope;
    class Code;
    class Function;
    
    class Operator : public NamedObject
    {
      friend class BindingList;
      
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<Operator> Create( std::string const &name, RC::Handle<Context> const &context );
      
      void addBinding( Binding *binding );
      void removeBinding( Binding *binding );

      virtual bool isOperator() const { return true; }

      std::string const &getEntryFunctionName() const;
      void setEntryFunctionName( std::string const &entryFunctionName );
      
      std::string const &getFilename() const;
      std::string const &getSourceCode() const;
      void setFilenameAndSourceCode( std::string const &filename, std::string const &sourceCode );
      
#if defined(FABRIC_BUILD_DEBUG)
      std::string const &getByteCode() const;
#endif

      bool getMainThreadOnly() const
      {
        return m_mainThreadOnly;
      }
      void setMainThreadOnly( bool mainThreadOnly )
      {
        m_mainThreadOnly = mainThreadOnly;
      }
      
      RC::ConstHandle<AST::GlobalList> getAST() const;
      void setAST( RC::ConstHandle<AST::GlobalList> const &ast );
      
      CG::Diagnostics const &getDiagnostics() const;

      RC::Handle<MT::ParallelCall> bind(
        std::vector<std::string> &errors,
        Prototype *prototype,
        Scope const &scope,
        unsigned prefixCount,
        void * const *prefixes
        ) const;

      virtual void jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      static void jsonExecCreate( JSON::Entity const &arg, RC::Handle<Context> const &context, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecSetSourceCode( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecSetEntryFunctionName( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecSetMainThreadOnly( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonDesc( JSON::Encoder &resultEncoder ) const;
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
      virtual void jsonDescType( JSON::Encoder &resultEncoder ) const;
      void jsonDescFilename( JSON::Encoder &resultEncoder ) const;
      void jsonDescSourceCode( JSON::Encoder &resultEncoder ) const;
      void jsonDescEntryFunctionName( JSON::Encoder &resultEncoder ) const;
      void jsonDescDiagnostics( JSON::Encoder &resultEncoder ) const;
      void jsonDescMainThreadOnly( JSON::Encoder &resultEncoder ) const;
      
    protected:
    
      Operator( std::string const &name, RC::Handle<Context> const &context );
      ~Operator();
      
      void compile();
      void resolveEntryFunction();
      void resolveASTOperator();
    
      virtual void propagateMarkForRecompileImpl( unsigned generation );    
      virtual void propagateMarkForRefreshImpl( unsigned generation );    
      virtual void collectErrors();
      virtual void invalidateRunState();
      virtual void refreshRunState();
      virtual void collectTasksImpl( unsigned generation, MT::TaskGroupStream &taskGroupStream ) const;
      virtual bool canExecute() const;

    private:
    
      Context *m_context;
    
      std::string m_entryFunctionName;
      std::string m_filename;
      std::string m_sourceCode;
      RC::ConstHandle<Code> m_code;
      RC::ConstHandle<Function> m_function;
      bool m_mainThreadOnly;
      
      RC::ConstHandle<AST::Operator> m_astOperator;
    
      typedef std::multiset<Binding *> Bindings; 
      Bindings m_bindings;
    };
  };
};

#endif //_FABRIC_DG_OPERATOR_H

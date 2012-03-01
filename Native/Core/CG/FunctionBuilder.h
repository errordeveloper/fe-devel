/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_FUNCTION_BUILDER_H
#define _FABRIC_CG_FUNCTION_BUILDER_H

#include <Fabric/Core/CG/Function.h>
#include <Fabric/Core/CG/Scope.h>
#include <Fabric/Base/RC/Handle.h>

#include <llvm/Argument.h>

namespace llvm
{
  class LLVMContext;
  class Module;
  class Function;
};

namespace Fabric
{
  namespace CG
  {
    class Manager;
    class ModuleBuilder;
    class PencilSymbol;
    
    class FunctionBuilder
    {
    public:
    
      static const size_t ExportSymbol = 0x01;
      static const size_t ImportSymbol = 0x01;
      static const size_t DirectlyReturnLValue = 0x02;
      static const size_t DirectlyReturnRValue = 0x04;
    
      FunctionBuilder( 
        ModuleBuilder &moduleBuilder,
        std::string const &pencilKey,
        std::string const &symbolName,
        std::string const &desc,
        RC::ConstHandle<Adapter> const &returnAdapter, 
        ParamVector const &params,
        size_t flags,
        PolymorphismParameters const &polymorphismParameters = PolymorphismParameters()
        );
      
      FunctionBuilder( 
        ModuleBuilder &moduleBuilder, 
        std::string const &pencilKey,
        std::string const &symbolName,
        std::string const &desc,
        std::string const &paramLayout,
        size_t flags
        );
        
      FunctionBuilder(
        ModuleBuilder &moduleBuilder,
        llvm::FunctionType const *llvmFunctionType,
        llvm::Function *llvmFunction
        );
      
      FunctionBuilder(
        ModuleBuilder &moduleBuilder, 
        RC::ConstHandle<CG::Adapter> const &returnAdapter,
        std::string const &functionName,
        std::string const &param1Name,
        RC::ConstHandle<CG::Adapter> const &param1Adapter,
        Usage param1Usage,
        size_t flags = 0
        );
      
      FunctionBuilder(
        ModuleBuilder &moduleBuilder, 
        RC::ConstHandle<CG::Adapter> const &returnAdapter,
        std::string const &functionName,
        std::string const &param1Name,
        RC::ConstHandle<CG::Adapter> const &param1Adapter,
        Usage param1Usage,
        std::string const &param2Name,
        RC::ConstHandle<CG::Adapter> const &param2Adapter,
        Usage param2Usage,
        size_t flags = 0
        );
      
      ~FunctionBuilder();
      
      llvm::Function *getLLVMFunction();
      llvm::Function *operator ->();
      
      ModuleBuilder &getModuleBuilder();
      llvm::BasicBlock *createBasicBlock( std::string const &name );
      
      RC::Handle<Manager> getManager();

      RC::Handle<Context> getContext();
      
      llvm::Argument *operator[]( size_t index );
      
      FunctionScope &getScope();

      RC::ConstHandle<Adapter> maybeGetAdapter( std::string const &userName ) const;
      RC::ConstHandle<Adapter> getAdapter( std::string const &userName, CG::Location const &location ) const;
      
      RC::ConstHandle<CG::PencilSymbol> getPencil() const;
    
    protected:

      static inline FunctionParam FriendlyFunctionParam(
        std::string const &paramName,
        RC::ConstHandle<Adapter> const &paramAdapter,
        Usage paramUsage
        )
      {
        std::string friendlyParamName;
        switch ( paramUsage )
        {
          case USAGE_LVALUE:
            friendlyParamName = paramName + "LValue";
            break;
          case USAGE_RVALUE:
            friendlyParamName = paramName + "RValue";
            break;
          default:
            FABRIC_ASSERT( false );
            friendlyParamName = paramName;
            break;
        }
        return FunctionParam( friendlyParamName, paramAdapter, paramUsage );
      }
      
    private:
    
      void build( 
        std::string const &pencilKey, 
        std::string const &symbolName, 
        std::string const &desc,
        RC::ConstHandle<Adapter> const &returnAdapter, 
        ParamVector const &params, 
        size_t flags,
        PolymorphismParameters const &polymorphismParameters = PolymorphismParameters()
        );
                
      ModuleBuilder &m_moduleBuilder;
      llvm::FunctionType const *m_llvmFunctionType;
      llvm::Function *m_llvmFunction;
      FunctionScope *m_functionScope;
      RC::ConstHandle<CG::PencilSymbol> m_pencil;
      
      bool m_haveHiddenReturnLValue;
    };
  }
}

#endif //_FABRIC_CG_FUNCTION_BUILDER_H

/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_FLOAT_ADAPTER_H
#define _FABRIC_CG_FLOAT_ADAPTER_H

#include <Fabric/Core/CG/SimpleAdapter.h>

namespace llvm
{
  class Constant;
};

namespace Fabric
{
  namespace RT
  {
    class FloatDesc;
  }
  namespace CG
  {
    class FloatAdapter : public SimpleAdapter
    {
      friend class Manager;
    
    public:
      REPORT_RC_LEAKS
      
      virtual std::string toString( void const *data ) const;
    
      virtual llvm::Constant *llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const;

      llvm::Constant *llvmConst( RC::Handle<Context> const &context, double value ) const;
      
      virtual void llvmCompileToModule( ModuleBuilder &moduleBuilder ) const;
      
    protected:
      
      FloatAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::FloatDesc> const &floatDesc );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;
      
      std::string internalFunctionNameForType( std::string const &externalFunctionName ) const;
      std::string externalFunctionNameForType( std::string const &externalFunctionName ) const;
      
    private:
    
      RC::ConstHandle<RT::FloatDesc> m_floatDesc;
    };
  };
};

#endif //_FABRIC_CG_FLOAT_ADAPTER_H

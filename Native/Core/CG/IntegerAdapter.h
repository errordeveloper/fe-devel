/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_INTEGER_ADAPTER_H
#define _FABRIC_CG_INTEGER_ADAPTER_H

#include <Fabric/Core/CG/SimpleAdapter.h>
#include <Fabric/Core/RT/NumericDesc.h>

namespace llvm
{
  class Constant;
};

namespace Fabric
{
  namespace CG
  {
    class IntegerAdapter : public SimpleAdapter
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
      
      // Adapter
      
      virtual std::string toString( void const *data ) const;
      virtual llvm::Constant *llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const;
      virtual void llvmCompileToModule( ModuleBuilder &moduleBuilder ) const;
    
      // IntegerAdapter
      
      llvm::Constant *llvmConst( RC::Handle<Context> const &context, int32_t value ) const;
          
    protected:

      IntegerAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::NumericDesc> const &integerDesc );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;
      
    private:
    
      RC::ConstHandle<RT::NumericDesc> m_integerDesc;
    };
  };
};

#endif //_FABRIC_CG_INTEGER_ADAPTER_H

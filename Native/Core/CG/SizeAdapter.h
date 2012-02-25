/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_SIZE_ADAPTER_H
#define _FABRIC_CG_SIZE_ADAPTER_H

#include <Fabric/Core/CG/SimpleAdapter.h>

namespace llvm
{
  class Constant;
};

namespace Fabric
{
  namespace RT
  {
    class NumericDesc;
  };
  
  namespace CG
  {
    class SizeAdapter : public SimpleAdapter
    {
      friend class Manager;
    
    public:
      REPORT_RC_LEAKS

      // Adapter
      
      virtual std::string toString( void const *data ) const;
      virtual llvm::Constant *llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const;
      virtual void llvmCompileToModule( ModuleBuilder &moduleBuilder ) const;
      
      // SizeAdapter
      
      llvm::Constant *llvmConst( RC::Handle<Context> const &context, size_t value ) const;

    protected:

      SizeAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::NumericDesc> const &sizeDesc );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;
      
    private:
    
      RC::ConstHandle<RT::NumericDesc> m_sizeDesc;
    };
  };
};

#endif //_FABRIC_CG_SIZE_ADAPTER_H

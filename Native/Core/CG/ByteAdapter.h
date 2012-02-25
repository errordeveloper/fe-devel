/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_BYTE_ADAPTER_H
#define _FABRIC_CG_BYTE_ADAPTER_H

#include <Fabric/Core/CG/SimpleAdapter.h>

namespace Fabric
{
  namespace RT
  {
    class NumericDesc;
  };
  
  namespace CG
  {
    class ByteAdapter : public SimpleAdapter
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
      
      // Adapter
      
      virtual std::string toString( void const *data ) const;
      virtual llvm::Constant *llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const;
      virtual void llvmCompileToModule( ModuleBuilder &moduleBuilder ) const;

      // ByteAdapter
      
      llvm::Constant *llvmConst( RC::Handle<Context> const &context, uint8_t value ) const;
      
    protected:
    
      ByteAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::NumericDesc> const &byteDesc );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;
      
    private:
    
      RC::ConstHandle<RT::NumericDesc> m_byteDesc;
    };
  };
};

#endif //_FABRIC_CG_BYTE_ADAPTER_H

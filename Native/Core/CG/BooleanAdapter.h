/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_BOOLEAN_ADAPTER_H
#define _FABRIC_CG_BOOLEAN_ADAPTER_H

#include <Fabric/Core/CG/SimpleAdapter.h>

namespace llvm
{
  class Constant;
}

namespace Fabric
{
  namespace RT
  {
    class BooleanDesc;
  };
  
  namespace CG
  {
    class Manager;
    
    class BooleanAdapter : public SimpleAdapter
    {
      friend class Manager;
      
    public:
      REPORT_RC_LEAKS
      
      // Adapter
      
      virtual std::string toString( void const *data ) const;
      virtual llvm::Constant *llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const;
      virtual void llvmCompileToModule( ModuleBuilder &moduleBuilder ) const;
    
      // BooleanAdapter
    
      llvm::Constant *llvmConst( RC::Handle<Context> const &context, bool value ) const;

    protected:

      BooleanAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::BooleanDesc> const &booleanDesc );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;
     
    private:
    
      RC::ConstHandle<RT::BooleanDesc> m_booleanDesc;
    };
  };
};

#endif //_FABRIC_CG_BOOLEAN_ADAPTER_H

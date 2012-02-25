/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_OPAQUE_ADAPTER_H
#define _FABRIC_CG_OPAQUE_ADAPTER_H

#include <Fabric/Core/CG/SimpleAdapter.h>

namespace Fabric
{
  namespace RT
  {
    class OpaqueDesc;
  };
  
  namespace CG
  {
    class OpaqueAdapter : public Adapter
    {
      friend class Manager;

    public:
      REPORT_RC_LEAKS

      // Adapter
    
      virtual void llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const;

      virtual void llvmCompileToModule( ModuleBuilder &moduleBuilder ) const;
      
    protected:

      OpaqueAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::OpaqueDesc> const &opaqueDesc );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;
      
      virtual llvm::Constant *llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const;
     
      virtual std::string toString( void const *data ) const;
      
    private:
    
      RC::ConstHandle<RT::OpaqueDesc> m_opaqueDesc;
    };
  };
};

#endif //_FABRIC_CG_OPAQUE_ADAPTER_H

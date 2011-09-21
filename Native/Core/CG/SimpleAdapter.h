#ifndef _FABRIC_CG_SIMPLE_ADAPTER_H
#define _FABRIC_CG_SIMPLE_ADAPTER_H

#include <Fabric/Core/CG/Adapter.h>

namespace Fabric
{
  namespace CG
  {
    class SimpleAdapter : public Adapter
    {
    public:
    
      virtual void llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const;
      virtual void llvmRetain( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const;
      virtual void llvmRelease( BasicBlockBuilder &basicBlockBuilder, llvm::Value *rValue ) const;

    protected:
    
      SimpleAdapter(
        RC::ConstHandle<Manager> const &manager,
        RC::ConstHandle<RT::Desc> const &desc
        );
    };
  };
};

#endif //_FABRIC_CG_SIMPLE_ADAPTER_H

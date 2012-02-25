/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_CG_STRUCT_ADAPTER_H
#define _FABRIC_CG_STRUCT_ADAPTER_H

#include <Fabric/Core/CG/Adapter.h>

#include <vector>

namespace Fabric
{
  namespace RT
  {
    struct StructMemberInfo;
    class StructDesc;
  };
  
  namespace CG
  {
    class ModuleBuilder;
    
    class StructAdapter : public Adapter
    {
      friend class Manager;
      
      typedef std::vector< RC::ConstHandle<Adapter> > MemberAdaptorVector;
      
    public:
      REPORT_RC_LEAKS
    
      // Adapter

      virtual void llvmDefaultAssign( BasicBlockBuilder &basicBlockBuilder, llvm::Value *dstLValue, llvm::Value *srcRValue ) const;
      virtual void llvmDisposeImpl( BasicBlockBuilder &basicBlockBuilder, llvm::Value *lValue ) const;

      virtual llvm::Constant *llvmDefaultValue( BasicBlockBuilder &basicBlockBuilder ) const;
      virtual llvm::Constant *llvmDefaultRValue( BasicBlockBuilder &basicBlockBuilder ) const;
      virtual llvm::Constant *llvmDefaultLValue( BasicBlockBuilder &basicBlockBuilder ) const;
 
      virtual void llvmCompileToModule( ModuleBuilder &moduleBuilder ) const;
      
      virtual std::string toString( void const *data ) const;
    
      // StructAdapter
    
      bool hasMember( std::string const &memberName ) const;
      size_t getMemberIndex( std::string const &memberName ) const;
      
      size_t getNumMembers() const
      {
        return m_memberAdapters.size();
      }
      RT::StructMemberInfo const &getMemberInfo( size_t memberIndex ) const;
      RC::ConstHandle<Adapter> getMemberAdapter( size_t memberIndex ) const;

    protected:
    
      StructAdapter( RC::ConstHandle<Manager> const &manager, RC::ConstHandle<RT::StructDesc> const &structDesc );
      
      virtual llvm::Type const *buildLLVMRawType( RC::Handle<Context> const &context ) const;

    private:
    
      RC::ConstHandle<RT::StructDesc> m_structDesc;
      bool m_isShallow;
      MemberAdaptorVector m_memberAdapters;
    };
  };
};

#endif //_FABRIC_CG_STRUCT_ADAPTER_H

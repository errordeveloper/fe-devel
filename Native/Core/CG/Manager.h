/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#ifndef _FABRIC_CG_MANAGER_H
#define _FABRIC_CG_MANAGER_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Core/RT/StructMemberInfo.h>

#include <string>
#include <map>
#include <llvm/LLVMContext.h>

namespace llvm
{
  class Module;
  class ExecutionEngine;
};

namespace Fabric
{
  namespace RT
  {
    class Desc;
    class Manager;
  };
  
  namespace CG
  {
    class Adapter;
    class ArrayProducerAdapter;
    class BooleanAdapter;
    class ByteAdapter;
    class ConstStringAdapter;
    class ContainerAdapter;
    class IntegerAdapter;
    class SizeAdapter;
    class FloatAdapter;
    class StringAdapter;
    class StructAdapter;
    class FixedArrayAdapter;
    class VariableArrayAdapter;
    class ValueProducerAdapter;
    class SlicedArrayAdapter;
    class OpaqueAdapter;
    class ModuleBuilder;
    
    class Manager : public RC::Object
    {
      typedef std::map< RC::ConstHandle<RT::Desc>, RC::ConstHandle<Adapter> > DescToAdapterMap;
      
    public:
    
      static RC::Handle<Manager> Create( RC::Handle<RT::Manager> const &rtManager );
      
      RC::Handle<RT::Manager> getRTManager() const;
      
      RC::ConstHandle<Adapter> getAdapter( RC::ConstHandle<RT::Desc> const &desc ) const;
      RC::ConstHandle<Adapter> maybeGetAdapter( std::string const &userName ) const;
      RC::ConstHandle<Adapter> getAdapter( std::string const &userName ) const;
      std::string const &getCodeName( std::string const &userName ) const;
      
      RC::ConstHandle<BooleanAdapter> getBooleanAdapter() const;
      RC::ConstHandle<ByteAdapter> getByteAdapter() const;
      RC::ConstHandle<IntegerAdapter> getIntegerAdapter() const;
      RC::ConstHandle<SizeAdapter> getSizeAdapter() const;
      RC::ConstHandle<FloatAdapter> getFP32Adapter() const;
      RC::ConstHandle<FloatAdapter> getFP64Adapter() const;
      RC::ConstHandle<StringAdapter> getStringAdapter() const;
      RC::ConstHandle<OpaqueAdapter> getDataAdapter() const;
      RC::ConstHandle<ConstStringAdapter> getConstStringAdapter() const;
      RC::ConstHandle<ContainerAdapter> getContainerAdapter() const;
      
      RC::ConstHandle<VariableArrayAdapter> getVariableArrayOf( RC::ConstHandle<Adapter> const &adapter ) const;
      RC::ConstHandle<SlicedArrayAdapter> getSlicedArrayOf( RC::ConstHandle<Adapter> const &adapter ) const;
      RC::ConstHandle<FixedArrayAdapter> getFixedArrayOf( RC::ConstHandle<Adapter> const &adapter, size_t length ) const;
      RC::ConstHandle<ValueProducerAdapter> getValueProducerOf( RC::ConstHandle<Adapter> const &adapter ) const;
      RC::ConstHandle<ArrayProducerAdapter> getArrayProducerOf( RC::ConstHandle<Adapter> const &adapter ) const;
      
      RC::ConstHandle<StructAdapter> registerStruct( std::string const &name, RT::StructMemberInfoVector const &structMemberInfoVector );
      RC::ConstHandle<Adapter> registerAlias( std::string const &name, RC::ConstHandle<Adapter> const &adapter );
      
      void *llvmResolveExternalFunction( std::string const &functionName ) const;
      void llvmAddGlobalMappingsToExecutionEngine( llvm::ExecutionEngine *executionEngine, llvm::Module &module ) const;

      RC::ConstHandle<RT::Desc> getStrongerTypeOrNone( RC::ConstHandle<RT::Desc> const &lhsDesc, RC::ConstHandle<RT::Desc> const &rhsDesc ) const;
      
    protected:
    
      Manager( RC::Handle<RT::Manager> const &rtManager );
      
    private:
    
      typedef std::map< size_t, RC::ConstHandle<ConstStringAdapter> > ConstStringAdapters;
      
      RC::Handle<RT::Manager> m_rtManager;
      
      mutable DescToAdapterMap m_descToAdapterMap;
      
      mutable RC::ConstHandle<BooleanAdapter> m_booleanAdapter;
      mutable RC::ConstHandle<ByteAdapter> m_byteAdapter;
      mutable RC::ConstHandle<IntegerAdapter> m_integerAdapter;
      mutable RC::ConstHandle<SizeAdapter> m_sizeAdapter;
      mutable RC::ConstHandle<FloatAdapter> m_scalarAdapter;
      mutable RC::ConstHandle<FloatAdapter> m_fp64Adapter;
      mutable RC::ConstHandle<StringAdapter> m_stringAdapter;
      mutable RC::ConstHandle<OpaqueAdapter> m_dataAdapter;
      mutable RC::ConstHandle<ConstStringAdapter> m_constStringAdapter;
      mutable RC::ConstHandle<ContainerAdapter> m_containerAdapter;
    };
  };
};

#endif //_FABRIC_CG_MANAGER_H

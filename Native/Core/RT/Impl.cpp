/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/Impl.h>

#include <Fabric/Core/RT/ArrayProducerImpl.h>
#include <Fabric/Core/RT/ComparableImpl.h>
#include <Fabric/Core/RT/DictImpl.h>
#include <Fabric/Core/RT/FixedArrayImpl.h>
#include <Fabric/Core/RT/SlicedArrayImpl.h>
#include <Fabric/Core/RT/ValueProducerImpl.h>
#include <Fabric/Core/RT/VariableArrayImpl.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/Util/Bits.h>
#include <Fabric/Base/Exception.h>

namespace Fabric
{
  namespace RT
  {
    Impl::Impl()
      : m_disposeCallback( 0 )
    {
    }
    
    void Impl::initialize( std::string const &codeName, ImplType implType, size_t allocSize, size_t flags )
    {
      FABRIC_ASSERT( Util::countBits( implType ) == 1 );
      m_codeName = codeName;
      m_implType = implType;
      m_allocSize = allocSize;
      m_flags = flags;
    }

    RC::ConstHandle<DictImpl> Impl::getDictImpl( RC::ConstHandle<ComparableImpl> const &comparableImpl ) const
    {
      RC::WeakConstHandle<DictImpl> &dictImplWeakHandle = m_dictImpls[comparableImpl];
      RC::ConstHandle<DictImpl> dictImpl = dictImplWeakHandle.makeStrong();
      if ( !dictImpl )
      {
        dictImpl = new DictImpl( m_codeName + ".Dict_" + comparableImpl->getCodeName(), comparableImpl, this );
        dictImplWeakHandle = dictImpl;
      }
      return dictImpl;
    }

    RC::ConstHandle<FixedArrayImpl> Impl::getFixedArrayImpl( size_t length ) const
    {
      RC::WeakConstHandle<FixedArrayImpl> &fixedArrayImplWeakHandle = m_fixedArrayImpls[length];
      RC::ConstHandle<FixedArrayImpl> fixedArrayImpl = fixedArrayImplWeakHandle.makeStrong();
      if ( !fixedArrayImpl )
      {
        fixedArrayImpl = new FixedArrayImpl( m_codeName + ".FixArray_" + _(length), this, length );
        fixedArrayImplWeakHandle = fixedArrayImpl;
      }
      return fixedArrayImpl;
    }

    RC::ConstHandle<VariableArrayImpl> Impl::getVariableArrayImpl() const
    {
      RC::WeakConstHandle<VariableArrayImpl> &variableArrayImplWeakHandle = m_variableArrayImpl;
      RC::ConstHandle<VariableArrayImpl> variableArrayImpl = variableArrayImplWeakHandle.makeStrong();
      if ( !variableArrayImpl )
      {
        std::string name = m_codeName + ".VarArray";
        variableArrayImpl = new VariableArrayImpl( name, this );
        variableArrayImplWeakHandle = variableArrayImpl;
      }
      return variableArrayImpl;
    }

    RC::ConstHandle<SlicedArrayImpl> Impl::getSlicedArrayImpl() const
    {
      RC::ConstHandle<SlicedArrayImpl> slicedArrayImpl = m_slicedArrayImpl.makeStrong();
      if ( !slicedArrayImpl )
      {
        slicedArrayImpl = new SlicedArrayImpl( m_codeName + ".SliceArray", this );
        m_slicedArrayImpl = slicedArrayImpl;
      }
      return slicedArrayImpl;
    }

    RC::ConstHandle<ValueProducerImpl> Impl::getValueProducerImpl() const
    {
      RC::WeakConstHandle<ValueProducerImpl> &valueProducerImplWeakHandle = m_valueProducerImpl;
      RC::ConstHandle<ValueProducerImpl> valueProducerImpl = valueProducerImplWeakHandle.makeStrong();
      if ( !valueProducerImpl )
      {
        std::string name = m_codeName + ".ValueProducer";
        valueProducerImpl = new ValueProducerImpl( name, this );
        valueProducerImplWeakHandle = valueProducerImpl;
      }
      return valueProducerImpl;
    }

    RC::ConstHandle<ArrayProducerImpl> Impl::getArrayProducerImpl() const
    {
      RC::WeakConstHandle<ArrayProducerImpl> &arrayProducerImplWeakHandle = m_arrayProducerImpl;
      RC::ConstHandle<ArrayProducerImpl> arrayProducerImpl = arrayProducerImplWeakHandle.makeStrong();
      if ( !arrayProducerImpl )
      {
        std::string name = m_codeName + ".ArrayProducer";
        arrayProducerImpl = new ArrayProducerImpl( name, this );
        arrayProducerImplWeakHandle = arrayProducerImpl;
      }
      return arrayProducerImpl;
    }

    void Impl::setDisposeCallback( void (*disposeCallback)( void * ) ) const
    {
      m_disposeCallback = disposeCallback;
    }

    size_t Impl::getIndirectMemoryUsage( void const *data ) const
    {
      return 0;
    }
  }
}

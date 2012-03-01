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
    Impl::Impl( std::string const &codeName, ImplType implType )
      : m_codeName( codeName )
      , m_implType( implType )
      , m_size( 0 )
      , m_disposeCallback( 0 )
    {
      FABRIC_ASSERT( Util::countBits( implType ) == 1 );
    }
    
    void Impl::setSize( size_t size )
    {
      m_size = size;
    }

    RC::ConstHandle<DictImpl> Impl::getDictImpl( RC::ConstHandle<ComparableImpl> const &comparableImpl ) const
    {
      RC::WeakConstHandle<DictImpl> &dictImplWeakHandle = m_dictImpls[comparableImpl];
      RC::ConstHandle<DictImpl> dictImpl = dictImplWeakHandle.makeStrong();
      if ( !dictImpl )
      {
        dictImpl = new DictImpl( m_codeName + "_D_" + comparableImpl->getCodeName(), comparableImpl, this );
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
        fixedArrayImpl = new FixedArrayImpl( m_codeName + "_FA" + _(length), this, length );
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
        std::string name = m_codeName + "_VA";
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
        slicedArrayImpl = new SlicedArrayImpl( m_codeName + "_SA", this );
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
        std::string name = m_codeName + "_VP";
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
        std::string name = m_codeName + "_AP";
        arrayProducerImpl = new ArrayProducerImpl( name, this );
        arrayProducerImplWeakHandle = arrayProducerImpl;
      }
      return arrayProducerImpl;
    }
    
    void Impl::disposeData( void *lValue ) const
    {
      disposeDatas( lValue, 1, getAllocSize() );
    }
    
    void Impl::disposeDatas( void *lValue, size_t count, size_t stride ) const
    {
      if ( m_disposeCallback )
      {
        uint8_t *data = static_cast<uint8_t *>( lValue );
        uint8_t * const dataEnd = data + count * stride;
        while ( data != dataEnd )
        {
          m_disposeCallback( data );
          data += stride;
        }
      }
      disposeDatasImpl( lValue, count, stride );
    }

    void Impl::setDisposeCallback( void (*disposeCallback)( void * ) ) const
    {
      m_disposeCallback = disposeCallback;
    }

    size_t Impl::getIndirectMemoryUsage( void const *data ) const
    {
      return 0;
    }
  };
};

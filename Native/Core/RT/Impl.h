/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_IMPL_H
#define _FABRIC_RT_IMPL_H

#include <Fabric/Base/RC/Object.h>
#include <Fabric/Base/RC/WeakConstHandle.h>
#include <Fabric/Core/RT/ImplType.h>
#include <Fabric/Core/Util/UnorderedMap.h>
#include <Fabric/Base/Util/Assert.h>

#include <stdint.h>
#include <map>

namespace Fabric
{
  namespace JSON
  {
    class Decoder;
    class Encoder;
    struct Entity;
  };
  
  namespace RT
  {
    class ArrayProducerImpl;
    class VariableArrayImpl;
    class SlicedArrayImpl;
    class FixedArrayImpl;
    class FixedArrayImpl;
    class ComparableImpl;
    class DictImpl;
    class ValueProducerImpl;
    
    class Impl : public RC::Object
    {
    public:
      REPORT_RC_LEAKS
      
      std::string const &getCodeName() const { return m_codeName; }
      ImplType getType() const { return m_implType; }
      size_t getAllocSize() const { return m_allocSize; }
      bool isShallow() const { return m_flags & FlagShallow; }
      bool isNoAliasUnsafe() const { return m_flags & FlagNoAliasUnsafe; }
      bool isNoAliasSafe() const { return !isNoAliasUnsafe(); }
      bool isExportable() const { return m_flags & FlagExportable; }
      
      virtual void const *getDefaultData() const = 0;
      void setData( void const *src, void *dst ) const
      {
        setDatas( 1, src, getAllocSize(), dst, getAllocSize() );
      }
      void setDatas( size_t count, void const *src, size_t srcStride, void *dst, size_t dstStride ) const
      {
        if ( isShallow() && srcStride == getAllocSize() && dstStride == getAllocSize() )
          memcpy( dst, src, dstStride * count );
        else
          setDatasImpl(
            count,
            static_cast<uint8_t const *>( src ),
            srcStride,
            static_cast<uint8_t *>( dst ),
            dstStride
            );
      }
      void disposeData( void *data ) const
      {
        disposeDatas( 1, data, getAllocSize() );
      }
      void disposeDatas( size_t count, void *_data, size_t stride ) const
      {
        if ( m_disposeCallback )
        {
          uint8_t *data = static_cast<uint8_t *>( _data );
          uint8_t * const dataEnd = data + count * stride;
          while ( data != dataEnd )
          {
            m_disposeCallback( data );
            data += stride;
          }
        }
        if ( !isShallow() )
          disposeDatasImpl(
            count,
            static_cast<uint8_t *>( _data ),
            stride
            );
      }
      virtual std::string descData( void const *data ) const = 0;
      virtual bool equalsData( void const *lhs, void const *rhs ) const = 0;
      virtual size_t getIndirectMemoryUsage( void const *data ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const = 0;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const = 0;
      
      virtual bool isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const = 0;
      
      RC::ConstHandle<FixedArrayImpl> getFixedArrayImpl( size_t length ) const;
      RC::ConstHandle<VariableArrayImpl> getVariableArrayImpl() const;
      RC::ConstHandle<SlicedArrayImpl> getSlicedArrayImpl() const;
      RC::ConstHandle<DictImpl> getDictImpl( RC::ConstHandle<ComparableImpl> const &comparableImpl ) const;
      RC::ConstHandle<ValueProducerImpl> getValueProducerImpl() const;
      RC::ConstHandle<ArrayProducerImpl> getArrayProducerImpl() const;
      
      void setDisposeCallback( void (*disposeCallback)( void * ) ) const;
      
    protected:

      static const size_t FlagShallow = 0x01;
      static const size_t FlagNoAliasUnsafe = 0x02;
      static const size_t FlagExportable = 0x04;

      Impl();

      void initialize( std::string const &codeName, ImplType implType, size_t allocSize, size_t flags );

      virtual void setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const = 0;
      virtual void disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const = 0;
      
    private:
    
      std::string m_codeName;
      ImplType m_implType;
      size_t m_allocSize;
      size_t m_flags;
      
      mutable void (*m_disposeCallback)( void *lValue );
      
      mutable RC::WeakConstHandle<VariableArrayImpl> m_variableArrayImpl;
      mutable RC::WeakConstHandle<SlicedArrayImpl> m_slicedArrayImpl;
      mutable Util::UnorderedMap< size_t, RC::WeakConstHandle<FixedArrayImpl> > m_fixedArrayImpls;
      mutable std::map< RC::WeakConstHandle<ComparableImpl>, RC::WeakConstHandle<DictImpl> > m_dictImpls;
      mutable RC::WeakConstHandle<ValueProducerImpl> m_valueProducerImpl;
      mutable RC::WeakConstHandle<ArrayProducerImpl> m_arrayProducerImpl;
    };
  }
}

#endif // _FABRIC_RT_IMPL_H

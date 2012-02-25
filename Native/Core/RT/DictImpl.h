/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_RT_VARIABLE_DICT_IMPL_H
#define _FABRIC_RT_VARIABLE_DICT_IMPL_H

#include <Fabric/Core/RT/Impl.h>
#include <Fabric/Base/Util/AtomicSize.h>
#include <Fabric/Base/Util/Bits.h>

namespace Fabric
{
  namespace RT
  {
    class ComparableImpl;
    
#define RT_DICT_IMPL_MINIMUM_BUCKET_COUNT size_t(4)
    
    class DictImpl : public Impl
    {
      friend class Manager;
      friend class Impl;
      friend class SlicedArrayImpl;
      
      struct node_t
      {
        node_t *bitsPrevNode;
        node_t *bitsNextNode;
        node_t *bucketPrevNode;
        node_t *bucketNextNode;
        size_t keyHash;
        char keyAndValue[0];
      };
      
      struct bucket_t
      {
        node_t *firstNode;
        node_t *lastNode;
      };
        
      struct bits_t
      {
        size_t bucketCount;
        size_t nodeCount;
        node_t *firstNode;
        node_t *lastNode;
        bucket_t *buckets;
      };
    
    public:
      REPORT_RC_LEAKS
    
      // Impl
      
      virtual void setData( void const *src, void *dst ) const;
      virtual void disposeDatasImpl( void *data, size_t count, size_t stride ) const;
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      virtual bool equalsData( void const *lhs, void const *rhs ) const;
      virtual size_t getIndirectMemoryUsage( void const *data ) const;
      
      virtual void encodeJSON( void const *data, JSON::Encoder &encoder ) const;
      virtual void decodeJSON( JSON::Entity const &entity, void *data ) const;
      
      virtual bool isShallow() const;
      virtual bool isNoAliasSafe() const;
      virtual bool isEquivalentTo( RC::ConstHandle<RT::Impl> const &desc ) const;
      virtual bool isExportable() const;

      // DictImpl
      
      bool has( void const *data, void const *keyData ) const;
      void const *getImmutable( void const *data, void const *keyData ) const;
      void *getMutable( void *data, void const *keyData ) const;
      size_t getSize( void const *data ) const;
      void delete_( void *data, void const *keyData ) const;
      void clear( void *data ) const;
      
      std::string descData( void const *data, size_t limit ) const;

    protected:
    
      DictImpl(
        std::string const &codeName,
        RC::ConstHandle<RT::ComparableImpl> const &keyImpl,
        RC::ConstHandle<RT::Impl> const &valueImpl
        );
      
      void const *immutableKeyData( node_t const *node ) const
      {
        return &node->keyAndValue[0];
      }
      
      void *mutableKeyData( node_t *node ) const
      {
        return &node->keyAndValue[0];
      }
      
      void const *immutableValueData( node_t const *node ) const
      {
        return &node->keyAndValue[m_keySize];
      }
      
      void *mutableValueData( node_t *node ) const
      {
        return &node->keyAndValue[m_keySize];
      }
      
      static size_t BucketCountForNodeCount( size_t nodeCount )
      {
        return std::max( RT_DICT_IMPL_MINIMUM_BUCKET_COUNT, Util::nextPowerOfTwoMinusOne( nodeCount / 2 ) + 1 );
      }
      
      void disposeNode( node_t *node ) const;
      void disposeBits( bits_t *bits ) const;
      
      bool has( bucket_t const *bucket, void const *keyData ) const;
      void const *getImmutable( bucket_t const *bucket, void const *keyData ) const;
      void *getMutable( bits_t *bits, bucket_t *bucket, void const *keyData, size_t keyHash ) const;
      void delete_( bits_t *bits, bucket_t *bucket, void const *keyData ) const;
      
      void insertNode( bits_t *bits, bucket_t *bucket, node_t *node ) const;
      void removeNode( bits_t *bits, bucket_t *bucket, node_t *node ) const;
      
      void maybeResize( bits_t *bits ) const;

    private:

      RC::ConstHandle<ComparableImpl> m_keyImpl;
      size_t m_keySize;
      bool m_keyIsShallow, m_keyIsNoAliasSafe;
      RC::ConstHandle<Impl> m_valueImpl;
      size_t m_valueSize;
      bool m_valueIsShallow, m_valueIsNoAliasSafe;
      size_t m_nodeSize;
   };
  };
};

#endif //_FABRIC_RT_VARIABLE_DICT_IMPL_H

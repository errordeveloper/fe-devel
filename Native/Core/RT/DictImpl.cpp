/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "DictImpl.h"
#include <Fabric/Core/RT/ComparableImpl.h>
#include <Fabric/Core/RT/StringImpl.h>
#include <Fabric/Base/Util/SimpleString.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Core/Util/Timer.h>
#include <Fabric/Base/Config.h>
#include <Fabric/Base/Util/Bits.h>

#include <algorithm>

namespace Fabric
{
  namespace RT
  {
    DictImpl::DictImpl(
      std::string const &codeName,
      RC::ConstHandle<ComparableImpl> const &keyImpl,
      RC::ConstHandle<Impl> const &valueImpl
      )
      : m_keyImpl( keyImpl )
      , m_keySize( keyImpl->getAllocSize() )
      , m_valueImpl( valueImpl )
      , m_valueSize( valueImpl->getAllocSize() )
      , m_nodeSize( sizeof(node_t) + m_keySize + m_valueSize )
    {
      size_t flags = 0;
      if ( keyImpl->isNoAliasUnsafe() || valueImpl->isNoAliasUnsafe() )
        flags |= FlagNoAliasUnsafe;
      if ( m_keyImpl->isExportable() && m_valueImpl->isExportable() )
        flags |= FlagExportable;
      initialize( codeName, DT_DICT, sizeof(bits_t), flags );
    }
    
    void const *DictImpl::getDefaultData() const
    {
      static bits_t defaultData;
      static bool defaultDataInitialized = false;
      if ( !defaultDataInitialized )
      {
        memset( &defaultData, 0, sizeof(bits_t) );
        defaultDataInitialized = true;
      }
      return &defaultData;
    }

    void DictImpl::initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;

      while ( dst != dstEnd )
      {
        memset( dst, 0, sizeof(bits_t) );
        if ( src )
        {
          bits_t const *srcBits = reinterpret_cast<bits_t const *>( src );
          node_t const *node = srcBits->firstNode;
          while ( node )
          {
            void const *srcKeyData = immutableKeyData( node );
            void const *srcValueData = immutableValueData( node );
            m_valueImpl->setData( srcValueData, getMutable( dst, srcKeyData ) );
            node = node->bitsNextNode;
          }
          src += srcStride;
        }
        dst += dstStride;
      }
    }

    void DictImpl::setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( src );
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;

      while ( dst != dstEnd )
      {
        disposeData( dst );
        memset( dst, 0, sizeof(bits_t) );
        
        bits_t const *srcBits = reinterpret_cast<bits_t const *>( src );
        node_t const *node = srcBits->firstNode;
        while ( node )
        {
          void const *srcKeyData = immutableKeyData( node );
          void const *srcValueData = immutableValueData( node );
          m_valueImpl->setData( srcValueData, getMutable( dst, srcKeyData ) );
          node = node->bitsNextNode;
        }

        src += srcStride;
        dst += dstStride;
      }
    }
    
    void DictImpl::disposeNode( node_t *node ) const
    {
      m_keyImpl->disposeData( mutableKeyData( node ) );
      m_valueImpl->disposeData( mutableValueData( node ) );
      free( node );
    }
    
    void DictImpl::disposeBits( bits_t *bits ) const
    {
      node_t *node = bits->firstNode;
      while ( node )
      {
        node_t *nextNode = node->bitsNextNode;
        disposeNode( node );
        node = nextNode;
      }
      free( bits->buckets );
    }

    void DictImpl::disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const
    {
      uint8_t * const dataEnd = data + count * stride;
      while ( data != dataEnd )
      {
        bits_t *bits = reinterpret_cast<bits_t *>(data);
        disposeBits( bits );
        data += stride;
      }
    }
    
    bool DictImpl::has( bucket_t const *bucket, void const *keyData ) const
    {
      node_t const *node = bucket->firstNode;
      while ( node )
      {
        if ( m_keyImpl->compare( keyData, immutableKeyData( node ) ) == 0 )
          return true;
        node = node->bucketNextNode;
      }
      return false;
    }
    
    bool DictImpl::has( void const *data, void const *keyData ) const
    {
      bits_t const *bits = reinterpret_cast<bits_t const *>( data );
      if ( bits->bucketCount > 0 )
      {
        size_t keyHash = m_keyImpl->hash( keyData );
        size_t bucketIndex = keyHash & (bits->bucketCount - 1);
        bucket_t const *bucket = &bits->buckets[bucketIndex];
        return has( bucket, keyData );
      }
      else return false;
    }
    
    void const *DictImpl::getImmutable( bucket_t const *bucket, void const *keyData ) const
    {
      node_t const *node = bucket->firstNode;
      while ( node )
      {
        if ( m_keyImpl->compare( keyData, immutableKeyData( node ) ) == 0 )
          return immutableValueData( node );
        node = node->bucketNextNode;
      }
      return m_valueImpl->getDefaultData();
    }
    
    void const *DictImpl::getImmutable( void const *data, void const *keyData ) const
    {
      bits_t const *bits = reinterpret_cast<bits_t const *>( data );
      if ( bits->bucketCount > 0 )
      {
        size_t keyHash = m_keyImpl->hash( keyData );
        size_t bucketIndex = keyHash & (bits->bucketCount - 1);
        bucket_t const *bucket = &bits->buckets[bucketIndex];
        return getImmutable( bucket, keyData );
      }
      else return m_valueImpl->getDefaultData();
    }
    
    void DictImpl::insertNode( bits_t *bits, bucket_t *bucket, node_t *node ) const
    {
      ++bits->nodeCount;
      
      // [pzion 20111014] We insert at the back of the iteration
      // order since we preserve insert order.
      
      node->bitsNextNode = 0;
      if ( bits->lastNode )
      {
        FABRIC_ASSERT( bits->firstNode );
        node->bitsPrevNode = bits->lastNode;
        FABRIC_ASSERT( !bits->lastNode->bitsNextNode );
        bits->lastNode->bitsNextNode = node;
        bits->lastNode = node;
      }
      else
      {
        FABRIC_ASSERT( !bits->firstNode );
        node->bitsPrevNode = 0;
        bits->firstNode = bits->lastNode = node;
      }
      
      // [pzion 20111014] We insert at the *front* of the bucket
      // since there's a good chance we'll need to access this entry
      // again soon.
      
      node->bucketPrevNode = 0;
      if ( bucket->firstNode )
      {
        FABRIC_ASSERT( bucket->lastNode );
        node->bucketNextNode = bucket->firstNode;
        FABRIC_ASSERT( !bucket->firstNode->bucketPrevNode );
        bucket->firstNode->bucketPrevNode = node;
        bucket->firstNode = node;
      }
      else
      {
        FABRIC_ASSERT( !bucket->lastNode );
        node->bucketNextNode = 0;
        bucket->firstNode = bucket->lastNode = node;
      }
    }
    
    void DictImpl::maybeResize( bits_t *bits ) const
    {
      size_t oldBucketCount = bits->bucketCount;
      size_t newBucketCount = BucketCountForNodeCount( bits->nodeCount + 1 );
      if ( oldBucketCount < newBucketCount )
      {
        FABRIC_ASSERT( (newBucketCount & (newBucketCount - 1)) == 0 );
        bits->bucketCount = newBucketCount;

#if defined(FABRIC_BUILD_DEBUG)        
        size_t oldNodeCount = bits->nodeCount;
#endif
        bits->nodeCount = 0;
        
        bucket_t *oldBuckets = bits->buckets;
        size_t bucketsSize = bits->bucketCount * sizeof(bucket_t);
        bits->buckets = reinterpret_cast<bucket_t *>( malloc( bucketsSize ) );
        memset( bits->buckets, 0, bucketsSize );
        
        node_t *node = bits->firstNode;
        bits->firstNode = 0;
        bits->lastNode = 0;
        while ( node )
        {
          node_t *nextNode = node->bitsNextNode;
          size_t bucketIndex = node->keyHash & (bits->bucketCount - 1);
          bucket_t *bucket = &bits->buckets[bucketIndex];
          insertNode( bits, bucket, node );
          node = nextNode;
        }
        
        free( oldBuckets );
        
        FABRIC_ASSERT( bits->nodeCount == oldNodeCount );
      }
    }
    
    void *DictImpl::getMutable( bits_t *bits, bucket_t *bucket, void const *keyData, size_t keyHash ) const
    {
      // [pzion 20111014] Does the appropriate node already exist?
      
      node_t *node = bucket->firstNode;
      while ( node )
      {
        if ( m_keyImpl->compare( keyData, immutableKeyData( node ) ) == 0 )
          return mutableValueData( node );
        node = node->bucketNextNode;
      }
      
      // [pzion 20111014] The node doesn't exist; create a new one.
      
      node = reinterpret_cast<node_t *>( malloc( m_nodeSize ) );
      node->keyHash = keyHash;
      insertNode( bits, bucket, node );
      
      // Set the key and default value, and return the value data
      
      void *dstKeyData = mutableKeyData( node );
      m_keyImpl->initializeData( keyData, dstKeyData );
      void *valueData = mutableValueData( node );
      m_valueImpl->initializeData( m_valueImpl->getDefaultData(), valueData );
      return valueData;
    }
    
    void *DictImpl::getMutable( void *data, void const *keyData ) const
    {
      bits_t *bits = reinterpret_cast<bits_t *>( data );
      // [pzion 20111017] Only maybe resize when our node count
      // is zero or a power of two minus one
      if ( (bits->nodeCount & (bits->nodeCount - 1)) == 0 )
        maybeResize( bits );
      size_t keyHash = m_keyImpl->hash( keyData );
      size_t bucketIndex = keyHash & (bits->bucketCount - 1);
      bucket_t *bucket = &bits->buckets[bucketIndex];
      void *result = getMutable( bits, bucket, keyData, keyHash );
      return result;
    }
    
    void DictImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      RC::ConstHandle<StringImpl> keyImplAsStringImpl;
      if ( isString( m_keyImpl->getType() ) )
        keyImplAsStringImpl = RC::ConstHandle<StringImpl>::StaticCast( m_keyImpl );
      
      JSON::ObjectEncoder objectEncoder = encoder.makeObject();
      bits_t const *bits = reinterpret_cast<bits_t const *>( data );
      if ( bits )
      {
        node_t *node = bits->firstNode;
        while ( node )
        {
          void const *keyData = immutableKeyData( node );
          void const *valueData = immutableValueData( node );
          if ( keyImplAsStringImpl )
          {
            JSON::Encoder memberEncoder = objectEncoder.makeMember( keyImplAsStringImpl->getValueData( keyData ), keyImplAsStringImpl->getValueLength( keyData ) );
            m_valueImpl->encodeJSON( valueData, memberEncoder );
          }
          else
          {
            Util::SimpleString encodedKey;
            {
              JSON::Encoder encodedKeyEncoder( &encodedKey );
              m_keyImpl->encodeJSON( keyData, encodedKeyEncoder );
            }
            JSON::Encoder memberEncoder = objectEncoder.makeMember( encodedKey );
            m_valueImpl->encodeJSON( valueData, memberEncoder );
          }
          node = node->bitsNextNode;
        }
      }
    }
    
    void DictImpl::decodeJSON( JSON::Entity const &entity, void *data ) const
    {
      entity.requireObject();

      RC::ConstHandle<StringImpl> keyImplAsStringImpl;
      if ( isString( m_keyImpl->getType() ) )
        keyImplAsStringImpl = RC::ConstHandle<StringImpl>::StaticCast( m_keyImpl );
      
      disposeData( data );
      memset( data, 0, sizeof(bits_t) );
        
      void *keyData = alloca( m_keySize );
      m_keyImpl->initializeData( m_keyImpl->getDefaultData(), keyData );
      JSON::ObjectDecoder jsonObjectDecoder( entity );
      JSON::Entity keyString, valueEntity;
      while ( jsonObjectDecoder.getNext( keyString, valueEntity ) )
      {
        if ( keyImplAsStringImpl )
        {
          keyString.stringGetData( StringImpl::GetMutableValueData( keyData, keyString.stringLength() ) );
        }
        else
        {
          char *mutableData;
          char const *data;
          if ( keyString.stringIsShort() )
            data = keyString.stringShortData();
          else
          {
            mutableData = reinterpret_cast<char *>( alloca( keyString.stringLength() ) );
            keyString.stringGetData( mutableData );
            data = mutableData;
          }
          
          JSON::Decoder jsonDecoder( data, keyString.value.string.length );
          JSON::Entity jsonDecodedEntity;
          if ( !jsonDecoder.getNext( jsonDecodedEntity ) )
            throw Exception( "invalid JSON key" );
          m_keyImpl->decodeJSON( jsonDecodedEntity, keyData );
          if ( jsonDecoder.getNext( jsonDecodedEntity ) )
            throw Exception( "invalid JSON key" );
        }
        
        void *valueData = getMutable( data, keyData );
        m_valueImpl->decodeJSON( valueEntity, valueData );
      }
      m_keyImpl->disposeData( keyData );
    }
    
    void DictImpl::removeNode( bits_t *bits, bucket_t *bucket, node_t *node ) const
    {
      if ( node->bitsPrevNode )
      {
        FABRIC_ASSERT( node->bitsPrevNode->bitsNextNode == node );
        node->bitsPrevNode->bitsNextNode = node->bitsNextNode;
      }
      else
      {
        FABRIC_ASSERT( bits->firstNode == node );
        bits->firstNode = node->bitsNextNode;
      }
      
      if ( node->bitsNextNode )
      {
        FABRIC_ASSERT( node->bitsNextNode->bitsPrevNode == node );
        node->bitsNextNode->bitsPrevNode = node->bitsPrevNode;
      }
      else
      {
        FABRIC_ASSERT( bits->lastNode == node );
        bits->lastNode = node->bitsPrevNode;
      }
      
      if ( node->bucketPrevNode )
      {
        FABRIC_ASSERT( node->bucketPrevNode->bucketNextNode == node );
        node->bucketPrevNode->bucketNextNode = node->bucketNextNode;
      }
      else
      {
        FABRIC_ASSERT( bucket->firstNode == node );
        bucket->firstNode = node->bucketNextNode;
      }
      
      if ( node->bucketNextNode )
      {
        FABRIC_ASSERT( node->bucketNextNode->bucketPrevNode == node );
        node->bucketNextNode->bucketPrevNode = node->bucketPrevNode;
      }
      else
      {
        FABRIC_ASSERT( bucket->lastNode == node );
        bucket->lastNode = node->bucketPrevNode;
      }
      
      --bits->nodeCount;
      disposeNode( node );
    }
    
    void DictImpl::delete_( bits_t *bits, bucket_t *bucket, void const *keyData ) const
    {
      node_t *node = bucket->firstNode;
      while ( node )
      {
        if ( m_keyImpl->compare( keyData, immutableKeyData( node ) ) == 0 )
        {
          removeNode( bits, bucket, node );
          break;
        }
        node = node->bucketNextNode;
      }
    }
    
    void DictImpl::delete_( void *data, void const *keyData ) const
    {
      bits_t *bits = reinterpret_cast<bits_t *>( data );
      if ( bits->bucketCount > 0 )
      {
        size_t keyHash = m_keyImpl->hash( keyData );
        size_t bucketIndex = keyHash & (bits->bucketCount - 1);
        bucket_t *bucket = &bits->buckets[bucketIndex];
        delete_( bits, bucket, keyData );
      }
    }
    
    void DictImpl::clear( void *data ) const
    {
      bits_t *bits = reinterpret_cast<bits_t *>( data );
      node_t *node = bits->firstNode;
      while ( node )
      {
        node_t *nextNode = node->bitsNextNode;
        disposeNode( node );
        node = nextNode;
      }
      bits->nodeCount = 0;
      bits->firstNode = bits->lastNode = 0;
      for ( size_t i=0; i<bits->bucketCount; ++i )
      {
        bucket_t *bucket = &bits->buckets[i];
        bucket->firstNode = bucket->lastNode = 0;
      }
    }
    
    std::string DictImpl::descData( void const *data, size_t maxNumToDisplay ) const
    {
      size_t numDisplayed = 0;
      std::string result = "{";
      bits_t const *bits = reinterpret_cast<bits_t const *>( data );
      if ( bits )
      {
        node_t *node = bits->firstNode;
        while ( node )
        {
          if ( numDisplayed > 0 )
            result += ',';
          if ( numDisplayed == maxNumToDisplay )
          {
            result += "...";
            break;
          }
          result += m_keyImpl->descData( immutableKeyData( node ) );
          result += ':';
          result += m_valueImpl->descData( immutableValueData( node ) );
          ++numDisplayed;

          node = node->bitsNextNode;
        }
      }
      result += "}";
      return result;
    }
    
    std::string DictImpl::descData( void const *data ) const
    {
      return descData( data, 16 );
    }
    
    bool DictImpl::isEquivalentTo( RC::ConstHandle<Impl> const &that ) const
    {
      if ( !isDict( that->getType() ) )
        return false;
      RC::ConstHandle<DictImpl> dictImpl = RC::ConstHandle<DictImpl>::StaticCast( that );

      return m_keyImpl->isEquivalentTo( dictImpl->m_keyImpl )
        && m_valueImpl->isEquivalentTo( dictImpl->m_valueImpl );
    }

    size_t DictImpl::getSize( void const *data ) const
    {
      FABRIC_ASSERT( data );
      bits_t const *bits = static_cast<bits_t const *>(data);
      return bits->nodeCount;
    }
    
    bool DictImpl::equalsData( void const *lhs, void const *rhs ) const
    {
      FABRIC_ASSERT(false);
      return true;
    }

    size_t DictImpl::getIndirectMemoryUsage( void const *data ) const
    {
      bits_t const *bits = reinterpret_cast<bits_t const *>( data );
      size_t total = bits->bucketCount * sizeof( bucket_t );
      for ( size_t i=0; i<bits->bucketCount; ++i )
      {
        node_t const *node = bits->buckets[i].firstNode;
        while ( node )
        {
          total += sizeof( node )
            + m_keyImpl->getAllocSize() + m_keyImpl->getIndirectMemoryUsage( immutableKeyData( node ) )
            + m_valueImpl->getAllocSize() + m_valueImpl->getIndirectMemoryUsage( immutableValueData( node ) );
          node = node->bucketNextNode;
        }
      }
      return total;
    }
  }
}

/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include "DictImpl.h"
#include <Fabric/Core/RT/ComparableImpl.h>
#include <Fabric/Core/RT/StringImpl.h>
#include <Fabric/Base/JSON/Object.h>
#include <Fabric/Base/JSON/Encode.h>
#include <Fabric/Base/JSON/Decode.h>
#include <Fabric/Core/Util/Encoder.h>
#include <Fabric/Core/Util/Decoder.h>
#include <Fabric/Base/Util/SimpleString.h>
#include <Fabric/Core/Util/Format.h>
#include <Fabric/Core/Util/JSONGenerator.h>
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
      : Impl( codeName, DT_DICT )
      , m_keyImpl( keyImpl )
      , m_keySize( keyImpl->getAllocSize() )
      , m_keyIsShallow( keyImpl->isShallow() )
      , m_valueImpl( valueImpl )
      , m_valueSize( valueImpl->getAllocSize() )
      , m_valueIsShallow( valueImpl->isShallow() )
      , m_nodeSize( sizeof(node_t) + m_keySize + m_valueSize )
    {
      setSize( sizeof(bits_t) );
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

    void DictImpl::setData( void const *srcData, void *dstData ) const
    {
      disposeData( dstData );
      memset( dstData, 0, sizeof(bits_t) );
      
      bits_t const *srcBits = reinterpret_cast<bits_t const *>( srcData );
      node_t const *node = srcBits->firstNode;
      while ( node )
      {
        void const *srcKeyData = immutableKeyData( node );
        void const *srcValueData = immutableValueData( node );
        m_valueImpl->setData( srcValueData, getMutable( dstData, srcKeyData ) );
      }
    }
    
    void DictImpl::disposeNode( node_t *node ) const
    {
      if ( !m_keyIsShallow )
        m_keyImpl->disposeData( mutableKeyData( node ) );
      if ( !m_valueIsShallow )
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

    void DictImpl::disposeDatasImpl( void *_data, size_t count, size_t stride ) const
    {
      uint8_t *data = static_cast<uint8_t *>( _data );
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
        
        size_t oldNodeCount = bits->nodeCount;
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
      
      memset( &node->keyAndValue[0], 0, m_keySize + m_valueSize );
      m_keyImpl->setData( keyData, mutableKeyData( node ) );
      void *valueData = mutableValueData( node );
      m_valueImpl->setData( m_valueImpl->getDefaultData(), valueData );
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

    RC::Handle<JSON::Value> DictImpl::getJSONValue( void const *data ) const
    {
      RC::ConstHandle<StringImpl> keyImplAsStringImpl;
      if ( isString( m_keyImpl->getType() ) )
        keyImplAsStringImpl = RC::ConstHandle<StringImpl>::StaticCast( m_keyImpl );
        
      RC::Handle<JSON::Object> result = JSON::Object::Create();
      bits_t const *bits = reinterpret_cast<bits_t const *>( data );
      if ( bits )
      {
        node_t *node = bits->firstNode;
        while ( node )
        {
          void const *keyData = immutableKeyData( node );
          std::string keyString = keyImplAsStringImpl
            ? std::string( keyImplAsStringImpl->getValueData( keyData ), keyImplAsStringImpl->getValueLength( keyData ) )
            : JSON::encode( m_keyImpl->getJSONValue( keyData ) );
          result->set( keyString, m_valueImpl->getJSONValue( immutableValueData( node ) ) );
          node = node->bitsNextNode;
        }
      }
      return result;
    }
    
    void DictImpl::generateJSON( void const *data, Util::JSONGenerator &jsonGenerator ) const
    {
      RC::ConstHandle<StringImpl> keyImplAsStringImpl;
      if ( isString( m_keyImpl->getType() ) )
        keyImplAsStringImpl = RC::ConstHandle<StringImpl>::StaticCast( m_keyImpl );
      
      Util::JSONObjectGenerator jsonObjectGenerator = jsonGenerator.makeObject();
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
            Util::JSONGenerator memberJG = jsonObjectGenerator.makeMember( keyImplAsStringImpl->getValueData( keyData ), keyImplAsStringImpl->getValueLength( keyData ) );
            m_valueImpl->generateJSON( valueData, memberJG );
          }
          else
          {
            Util::SimpleString encodedKey;
            {
              Util::JSONGenerator encodedKeyJG( &encodedKey );
              m_keyImpl->generateJSON( keyData, encodedKeyJG );
            }
            Util::JSONGenerator memberJG = jsonObjectGenerator.makeMember( encodedKey );
            m_valueImpl->generateJSON( valueData, memberJG );
          }
          node = node->bitsNextNode;
        }
      }
    }
    
    void DictImpl::setDataFromJSONValue( RC::ConstHandle<JSON::Value> const &jsonValue, void *data ) const
    {
      if ( !jsonValue->isObject() )
        throw Exception( "JSON value is not object" );
      RC::ConstHandle<JSON::Object> jsonObject = RC::ConstHandle<JSON::Object>::StaticCast( jsonValue );

      RC::ConstHandle<StringImpl> keyImplAsStringImpl;
      if ( isString( m_keyImpl->getType() ) )
        keyImplAsStringImpl = RC::ConstHandle<StringImpl>::StaticCast( m_keyImpl );
      
      disposeData( data );
      memset( data, 0, sizeof(bits_t) );
        
      void *keyData = alloca( m_keySize );
      memset( keyData, 0, m_keySize );
      for ( JSON::Object::const_iterator it=jsonObject->begin(); it!=jsonObject->end(); ++it )
      {
        std::string const &keyString = it->first;
        if ( keyImplAsStringImpl )
          StringImpl::SetValue( keyString.data(), keyString.length(), keyData );
        else m_keyImpl->setDataFromJSONValue( JSON::decode( keyString ), keyData );
        
        RC::ConstHandle<JSON::Value> const &valueJSONValue = it->second;
        void *valueData = getMutable( data, keyData );
        m_valueImpl->setDataFromJSONValue( valueJSONValue, valueData );
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
    
    bool DictImpl::isShallow() const
    {
      return false;
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
    
    int DictImpl::compareData( void const *lhs, void const *rhs ) const
    {
      FABRIC_ASSERT(false);
      return 0;
    }
  };
};

/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "VariableArrayImpl.h"

#include <Fabric/Base/Util/SimpleString.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Core/Util/Timer.h>
#include <Fabric/Base/Config.h>
#include <Fabric/Base/Util/Bits.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>

#include <algorithm>

namespace Fabric
{
  namespace RT
  {
    VariableArrayImpl::VariableArrayImpl( std::string const &codeName, RC::ConstHandle<Impl> const &memberImpl )
      : ArrayImpl( memberImpl )
      , m_memberImpl( memberImpl )
      , m_memberSize( memberImpl->getAllocSize() )
    {
      size_t flags = 0;
      if ( m_memberImpl->isExportable() )
        flags |= FlagExportable;
      if ( m_memberImpl->isNoAliasUnsafe() )
        flags |= FlagNoAliasUnsafe;
      initialize( codeName, DT_VARIABLE_ARRAY, sizeof(bits_t *), flags );
    }

    void const *VariableArrayImpl::getDefaultData() const
    {
      static bits_t *defaultBits = 0;
      return &defaultBits;
    }

    size_t VariableArrayImpl::ComputeAllocatedSize( size_t prevNbAllocated, size_t nbRequested )
    {
      //[JeromeCG 20120130] Explanations
      //The main goals are both to avoid re-allocating repeatedly and memory waste.
      //Here are a few heuristics I try to take into account while trying to find the default formula. For some of these
      //I only partially succeed, because I still try to avoid too much complexity.
      //
      //  1- On the first resize, just allocate exactly what was asked. Often arrays are only allocated once. For KL, I'd extend that for the case
      //    where we go from 1 to many elements, since by default an initial slice count of 1.
      //  2- When an entire array is copied to another one, just allocate the actual size of the source (not its 'reserved' size).
      //  3- When growing an array, allocate exponentially to avoid scalability problems, but don't over-allocate too much (maybe in the 10% to 25% range)
      //  4- When shrinking an array, deallocate exponentially to avoid keeping to much unrequired memory
      //  5- Free the memory when resizing back to 0
      //  6- The smaller the array, the bigger the proportion of each over-allocated items
      //  7- Small arrays are frequent, including those which are resized dynamically (push / pop)
      //  8- Waste while shrinking is less critical than waste when growing, since memory peak has already been reached. However we still need to care about
      //  it since it can accumulate
      //
      //  Note: for 4) and 5), there are scenarios where it can be better to never shrink memory usage, particularly when an array is reused in a loop.
      //  However, I don't think that many programmers are careful enough to scope arrays outside of loops just in order to avoid reallocations. To support
      //  these scenarios we would need a 'hint' or a method in which the user can tell to not release memory...
      //
      //  Note2: there's no easy answer for both 6) and 7). With very small arrays, we can't at the same time avoid over-allocated items AND frequent reallocs...
      //  so we need to do a compromise between both.
      if( nbRequested > prevNbAllocated )
      {
        size_t inflatedNbAllocated;
        if( prevNbAllocated < 16 ) 
          inflatedNbAllocated = (prevNbAllocated>>1) + 1 + prevNbAllocated;//50% + 1 growth
        else
          inflatedNbAllocated = (prevNbAllocated>>3) + 4 + prevNbAllocated;//12.5% + 4 growth  (+4: just to maintain the 'pace' we had when < 16)
        return std::max( nbRequested, inflatedNbAllocated );
      }
      else if( nbRequested < prevNbAllocated )
      {
        if( nbRequested == 0 )
          return 0;

        //Because it's exponentially growing and shrinking, we need to be careful for oscillation problems.
        //Eg: push -> reallocate with X% more, then pop -> shrink to new size because >X% wasted, then push -> reallocate again by X%...
        //To avoid this, we simply tolerate 25% of 'wasted' memory when we shrink back, while we grow by 12.5%.

        if( prevNbAllocated < 16 )
          return prevNbAllocated; //Avoid oscillation problems with small arrays, because of their different allocation policy above

        size_t deflateThreshold = prevNbAllocated - (prevNbAllocated>>2);//25% shrink
        return nbRequested <= deflateThreshold ? nbRequested : prevNbAllocated;
      }
      else
        return nbRequested;
    }

    void VariableArrayImpl::initializeDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;
      while ( dst != dstEnd )
      {
        if ( src )
        {
          bits_t *srcBits = *reinterpret_cast<bits_t * const *>( src );
          if ( srcBits )
            srcBits->refCount.increment();
          *reinterpret_cast<bits_t **>( dst ) = srcBits;
          src += srcStride;
        }
        else *reinterpret_cast<bits_t **>( dst ) = 0;
        dst += dstStride;
      }
    }

    void VariableArrayImpl::setDatasImpl( size_t count, uint8_t const *src, size_t srcStride, uint8_t *dst, size_t dstStride ) const
    {
      FABRIC_ASSERT( src );
      FABRIC_ASSERT( dst );
      uint8_t * const dstEnd = dst + count * dstStride;
      while ( dst != dstEnd )
      {
        bits_t *srcBits = *reinterpret_cast<bits_t * const *>( src );
        bits_t *dstBits = *reinterpret_cast<bits_t **>( dst );

        if ( dstBits != srcBits )
        {
          if ( dstBits && dstBits->refCount.decrementAndGetValue() == 0 )
          {
            m_memberImpl->disposeDatas( dstBits->numMembers, dstBits->memberDatas, m_memberSize );
            free( dstBits );
          }
          dstBits = srcBits;
          if ( dstBits )
            dstBits->refCount.increment();
          *reinterpret_cast<bits_t **>( dst ) = dstBits;
        }

        src += srcStride;
        dst += dstStride;
      }
    }

    void VariableArrayImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      size_t numMembers = getNumMembers(data);
      
      JSON::ArrayEncoder jsonArrayEncoder = encoder.makeArray();
      for ( size_t i = 0; i < numMembers; ++i )
      {
        void const *memberData = getImmutableMemberData_NoCheck( data, i );
        JSON::Encoder elementEncoder = jsonArrayEncoder.makeElement();
        m_memberImpl->encodeJSON( memberData, elementEncoder );
      }
    }
    
    void VariableArrayImpl::decodeJSON( JSON::Entity const &entity, void *data ) const
    {
      entity.requireArray();
        
      setNumMembers( data, entity.value.array.size );
        
      size_t membersFound = 0;
      JSON::ArrayDecoder arrayDecoder( entity );
      JSON::Entity elementEntity;
      while ( arrayDecoder.getNext( elementEntity ) )
      {
        FABRIC_ASSERT( membersFound < entity.value.array.size );
        try
        {
          void *memberData = getMutableMemberData_NoCheck( data, membersFound );
          m_memberImpl->decodeJSON( elementEntity, memberData );
        }
        catch ( Exception e )
        {
          throw _(membersFound) + ": " + e;
        }
        ++membersFound;
      }
      
      FABRIC_ASSERT( membersFound == entity.value.array.size );
    }

    void VariableArrayImpl::disposeDatasImpl( size_t count, uint8_t *data, size_t stride ) const
    {
      FABRIC_ASSERT( data );
      uint8_t * const dataEnd = data + count * stride;
      while ( data != dataEnd )
      {
        bits_t *bits = *reinterpret_cast<bits_t **>(data);
        if ( bits && bits->refCount.decrementAndGetValue() == 0 )
        {
          m_memberImpl->disposeDatas( bits->numMembers, bits->memberDatas, m_memberSize );
          free( bits );
        }
        data += stride;
      }
    }
    
    std::string VariableArrayImpl::descData( void const *data ) const
    {
      std::string result = "[";
      size_t numMembers = getNumMembers(data);
      size_t numMembersToDisplay = numMembers;
      if ( numMembersToDisplay > 16 )
        numMembersToDisplay = 16;
      for ( size_t i=0; i<numMembersToDisplay; ++i )
      {
        if ( result.length() > 1 )
          result += ",";
        result += m_memberImpl->descData( getImmutableMemberData_NoCheck( data, i ) );
      }
      if ( numMembers > numMembersToDisplay )
        result += "...";
      result += "]";
      return result;
    }

    void VariableArrayImpl::push( void *dst, void const *src ) const
    {
      size_t oldNumMembers = getNumMembers( dst );
      size_t newNumMembers = oldNumMembers + 1;
      setNumMembers( dst, newNumMembers );
      m_memberImpl->setData( src, getMutableMemberData_NoCheck( dst, oldNumMembers ) );
    }
    
    void VariableArrayImpl::pop( void *dst, void *result ) const
    {
      size_t oldNumMembers = getNumMembers( dst );
      size_t newNumMembers = oldNumMembers - 1;
      if ( result )
        m_memberImpl->setData( getImmutableMemberData_NoCheck( (void const *)dst, newNumMembers ), result );
      setNumMembers( dst, newNumMembers );
    }
    
    void VariableArrayImpl::append( void *dst, void const *src ) const
    {
      size_t srcNumMembers = getNumMembers( src );
      if ( srcNumMembers > 0 )
      {
        size_t oldDstNumMembers = getNumMembers( dst );
        size_t newDstNumMembers = oldDstNumMembers + srcNumMembers;
        setNumMembers(
          dst,
          newDstNumMembers,
          getImmutableMemberData_NoCheck( src, 0 ),
          m_memberSize
          );
      }
    }

    bool VariableArrayImpl::isEquivalentTo( RC::ConstHandle<Impl> const &that ) const
    {
      if ( !isVariableArray( that->getType() ) )
        return false;
      RC::ConstHandle<VariableArrayImpl> variableArrayImpl = RC::ConstHandle<VariableArrayImpl>::StaticCast( that );
      return getMemberImpl()->isEquivalentTo( variableArrayImpl->getMemberImpl() );
    }

    size_t VariableArrayImpl::getNumMembers( void const *data ) const
    {
      FABRIC_ASSERT( data );
      bits_t const *bits = *static_cast<bits_t const * const*>(data);
      return bits? bits->numMembers: 0;
    }
    
    void const *VariableArrayImpl::getImmutableMemberData( void const *data, size_t index ) const
    { 
      FABRIC_ASSERT( data );
      bits_t const *bits = *static_cast<bits_t const * const*>(data);
      size_t numMembers = bits->numMembers;
      if ( index >= numMembers )
        throw Exception( "index ("+_(index)+") out of range ("+_(numMembers)+")" );
      return getImmutableMemberData_NoCheck( data, index );
    }
    
    void *VariableArrayImpl::getMutableMemberData( void *data, size_t index ) const
    { 
      FABRIC_ASSERT( data );
      bits_t const *bits = *static_cast<bits_t const **>(data);
      size_t numMembers = bits->numMembers;
      if ( index >= numMembers )
        throw Exception( "index ("+_(index)+") out of range ("+_(numMembers)+")" );
      return getMutableMemberData_NoCheck( data, index );
    }

    void VariableArrayImpl::setMembers( void *data, size_t numMembers, void const *members ) const
    {
      FABRIC_ASSERT( data );
      setNumMembers( data, numMembers );
      setMembers( data, 0, numMembers, members );
    }

    void VariableArrayImpl::setMembers( void *data, size_t dstOffset, size_t numMembers, void const *members ) const
    {
      FABRIC_ASSERT( data );
      FABRIC_ASSERT( numMembers + dstOffset <= getNumMembers( data ) );
      m_memberImpl->setDatas(
        numMembers,
        members,
        m_memberSize,
        getMutableMemberData( data, dstOffset ),
        m_memberSize
        );
    }

    void VariableArrayImpl::setNumMembers( void *data, size_t newNumMembers, void const *defaultMemberData, size_t defaultMemberStride ) const
    {
      FABRIC_ASSERT( data );

      bits_t *bits = *reinterpret_cast<bits_t **>(data);
      size_t oldNumMembers = bits? bits->numMembers: 0;
      if ( oldNumMembers != newNumMembers )
      {
        if ( bits && bits->refCount.getValue() > 1 )
        {
          size_t newAllocNumMembers = ComputeAllocatedSize( 0, newNumMembers );

          bits_t *newBits;
          if ( newAllocNumMembers )
          {
            newBits = static_cast<bits_t *>( malloc( sizeof(bits_t) + newAllocNumMembers * m_memberSize ) );
            newBits->refCount.setValue( 1 );
            newBits->allocNumMembers = newAllocNumMembers;
            newBits->numMembers = newNumMembers;
            if ( bits )
            {
              size_t copyFromOldCount = std::min( oldNumMembers, newNumMembers );
              if ( copyFromOldCount > 0 )
                m_memberImpl->initializeDatas( copyFromOldCount, bits->memberDatas, m_memberSize, newBits->memberDatas, m_memberSize );
              size_t initialzeCount = newNumMembers - copyFromOldCount;
              if ( initialzeCount > 0 )
              {
                if ( !defaultMemberData )
                {
                  defaultMemberData = m_memberImpl->getDefaultData();
                  FABRIC_ASSERT( defaultMemberStride == 0 );
                }
                m_memberImpl->initializeDatas( initialzeCount, defaultMemberData, defaultMemberStride, newBits->memberDatas + m_memberSize * copyFromOldCount, m_memberSize );
              }
            }
          }
          else newBits = 0;
          *reinterpret_cast<bits_t **>(data) = newBits;

          if ( bits && bits->refCount.decrementAndGetValue() == 0 )
          {
            m_memberImpl->disposeDatas( bits->numMembers, bits->memberDatas, m_memberSize );
            free( bits );
          }
        }
        else
        {
          if ( newNumMembers < oldNumMembers )
            m_memberImpl->disposeDatas( oldNumMembers - newNumMembers, bits->memberDatas + m_memberSize * newNumMembers, m_memberSize );

          size_t oldAllocNumMembers = bits? bits->allocNumMembers: 0;
          size_t newAllocNumMembers = ComputeAllocatedSize( oldAllocNumMembers, newNumMembers );
          if ( newAllocNumMembers != oldAllocNumMembers )
          {
            if ( !newAllocNumMembers )
            {
              free( bits );
              bits = 0;
            }
            else if ( !oldAllocNumMembers )
            {
              bits = static_cast<bits_t *>( malloc( sizeof(bits_t) + newAllocNumMembers * m_memberSize ) );
              bits->refCount.setValue( 1 );
              bits->allocNumMembers = newAllocNumMembers;
            }
            else if ( bits )
            {
              bits = static_cast<bits_t *>( realloc( bits, sizeof(bits_t) + newAllocNumMembers * m_memberSize ) );
              bits->allocNumMembers = newAllocNumMembers;
            }
            *reinterpret_cast<bits_t **>(data) = bits;
          }

          if ( newNumMembers > oldNumMembers )
          {
            if ( !defaultMemberData )
            {
              defaultMemberData = m_memberImpl->getDefaultData();
              FABRIC_ASSERT( defaultMemberStride == 0 );
            }
            m_memberImpl->initializeDatas( newNumMembers - oldNumMembers, defaultMemberData, defaultMemberStride, bits->memberDatas + m_memberSize * oldNumMembers, m_memberSize );
          }

          if ( bits )
            bits->numMembers = newNumMembers;
        }
      }
    }

    size_t VariableArrayImpl::getIndirectMemoryUsage( void const *data ) const
    {
      bits_t const *bits = *reinterpret_cast<bits_t const * const *>(data);
      size_t total = 0;
      if ( bits )
      {
        total += bits->allocNumMembers * m_memberImpl->getAllocSize();
        for ( size_t i=0; i<bits->numMembers; ++i )
          total += m_memberImpl->getIndirectMemoryUsage( getImmutableMemberData_NoCheck( data, i ) );
      }
      return total;
    }

    VariableArrayImpl::bits_t *VariableArrayImpl::duplicate( void *data ) const
    {
      bits_t *bits = *static_cast<bits_t **>( data );
      FABRIC_ASSERT ( bits->refCount.getValue() > 1 );
      bits_t *newBits = static_cast<bits_t *>( malloc( sizeof(bits_t) + bits->allocNumMembers * m_memberSize ) );
      newBits->refCount.setValue( 1 );
      newBits->allocNumMembers = bits->allocNumMembers;
      newBits->numMembers = bits->numMembers;
      m_memberImpl->initializeDatas( newBits->numMembers, bits->memberDatas, m_memberSize, newBits->memberDatas, m_memberSize );
      if ( bits->refCount.decrementAndGetValue() == 0 )
      {
        m_memberImpl->disposeDatas( bits->numMembers, bits->memberDatas, m_memberSize );
        free( bits );
      }
      *static_cast<bits_t **>( data ) = newBits;
      return newBits;
    }
  }
}

/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
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
      : ArrayImpl( codeName, DT_VARIABLE_ARRAY, memberImpl )
      , m_memberImpl( memberImpl )
      , m_memberSize( memberImpl->getAllocSize() )
    {
      setSize( sizeof(bits_t) );
    }
    
    void const *VariableArrayImpl::getDefaultData() const
    {
      static bits_t defaultData = { 0, 0, 0 };
      return &defaultData;
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

    void VariableArrayImpl::setData( void const *_src, void *_dst ) const
    {
      bits_t const *src = reinterpret_cast<bits_t const *>(_src);
      bits_t *dst = reinterpret_cast<bits_t *>(_dst);
      
      if ( dst->numMembers > src->numMembers )
        m_memberImpl->disposeDatas( dst->memberDatas + src->numMembers * m_memberSize, dst->numMembers - src->numMembers, m_memberSize );
      dst->numMembers = src->numMembers;

      dst->allocNumMembers = src->numMembers;
      if ( dst->allocNumMembers == 0 )
      {
        if ( dst->memberDatas )
          free( dst->memberDatas );
        dst->memberDatas = 0;
      }
      else if ( dst->memberDatas )
        dst->memberDatas = reinterpret_cast<uint8_t *>( realloc( dst->memberDatas, dst->allocNumMembers * m_memberSize ) );
      else
        dst->memberDatas = reinterpret_cast<uint8_t *>( malloc( dst->allocNumMembers * m_memberSize ) );
      
      size_t totalSize = src->numMembers * m_memberSize;
      uint8_t const *srcMemberData = src->memberDatas;
      uint8_t const * const srcMemberDataEnd = srcMemberData + totalSize;
      uint8_t *dstMemberData = dst->memberDatas;
      memset( dstMemberData, 0, totalSize );
      while ( srcMemberData != srcMemberDataEnd )
      {
        m_memberImpl->setData( srcMemberData, dstMemberData );
        srcMemberData += m_memberSize;
        dstMemberData += m_memberSize;
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

    void VariableArrayImpl::disposeDatasImpl( void *_data, size_t count, size_t stride ) const
    {
      uint8_t *data = static_cast<uint8_t *>( _data );
      uint8_t * const dataEnd = data + count * stride;
      while ( data != dataEnd )
      {
        bits_t *bits = reinterpret_cast<bits_t *>(data);
        m_memberImpl->disposeDatas( bits->memberDatas, bits->numMembers, m_memberSize );
        free( bits->memberDatas );
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
      setNumMembers( dst, oldNumMembers + 1 );
      getMemberImpl()->setData( src, getMutableMemberData_NoCheck( dst, oldNumMembers ) );
    }
    
    void VariableArrayImpl::pop( void *dst, void *result ) const
    {
      size_t oldNumMembers = getNumMembers( dst );
      if ( result )
        getMemberImpl()->setData( getImmutableMemberData_NoCheck( (void const *)dst, oldNumMembers - 1 ), result );
      setNumMembers( dst, oldNumMembers - 1 );
    }
    
    void VariableArrayImpl::append( void *dst, void const *src ) const
    {
      bits_t const *srcBits = reinterpret_cast<bits_t const *>(src);
      if ( srcBits )
      {
        bits_t *dstBits = static_cast<bits_t *>( dst );
        size_t oldNumMembers = dstBits->numMembers;
        size_t newNumMembers = oldNumMembers + srcBits->numMembers;
        setNumMembers( dst, newNumMembers );
        copyMemberDatas( dstBits, oldNumMembers, srcBits, 0, srcBits->numMembers, false );
      }
    }
    
    bool VariableArrayImpl::isShallow() const
    {
      return false;
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
      bits_t const *bits = static_cast<bits_t const *>(data);
      return bits->numMembers;
    }
    
    void const *VariableArrayImpl::getImmutableMemberData( void const *data, size_t index ) const
    { 
      bits_t const *bits = reinterpret_cast<bits_t const *>(data);
      size_t numMembers = bits->numMembers;
      if ( index >= numMembers )
        throw Exception( "index ("+_(index)+") out of range ("+_(numMembers)+")" );
      return getImmutableMemberData_NoCheck( data, index );
    }
    
    void *VariableArrayImpl::getMutableMemberData( void *data, size_t index ) const
    { 
      bits_t *bits = reinterpret_cast<bits_t *>(data);
      size_t numMembers = bits->numMembers;
      if ( index >= numMembers )
        throw Exception( "index ("+_(index)+") out of range ("+_(numMembers)+")" );
      return getMutableMemberData_NoCheck( data, index );
    }

    void VariableArrayImpl::setMembers( void *data, size_t numMembers, void const *members ) const
    {
      setNumMembers( data, numMembers );
      setMembers( data, 0, numMembers, members );
    }

    void VariableArrayImpl::setMembers( void *data, size_t dstOffset, size_t numMembers, void const *members ) const
    {
      FABRIC_ASSERT( numMembers + dstOffset <= getNumMembers( data ) );
      if ( !isMemberShallow() )
      {
        for ( size_t i=0; i<numMembers; ++i )
        {
          void *memberData = getMutableMemberData( data, i+dstOffset );
          getMemberImpl()->setData( &((uint8_t const *)members)[i*m_memberSize], memberData );
        }
      }
      else
      {
        bits_t *bits = reinterpret_cast<bits_t *>(data);
        memcpy( bits->memberDatas + dstOffset*m_memberSize, members, numMembers*m_memberSize );
      }
    }

    void VariableArrayImpl::setNumMembers( void *data, size_t newNumMembers, void const *defaultMemberData ) const
    {
      //Util::Timer ft;
      bits_t *bits = reinterpret_cast<bits_t *>(data);
      size_t oldNumMembers = bits->numMembers;
      size_t oldAllocNumMembers = bits->allocNumMembers;
      if ( oldNumMembers != newNumMembers )
      {
        if ( newNumMembers < oldNumMembers )
          getMemberImpl()->disposeDatas( bits->memberDatas + m_memberSize * newNumMembers, oldNumMembers - newNumMembers, m_memberSize );
          
        if ( newNumMembers == 0 )
        {
          free( bits->memberDatas );
          bits->allocNumMembers = 0;
          bits->memberDatas = 0;
        }
        else
        {
          if ( newNumMembers > oldAllocNumMembers )
          {
            size_t newAllocNumMembers = ComputeAllocatedSize( oldAllocNumMembers, newNumMembers );
            if ( oldNumMembers )
            {
              size_t size = m_memberSize * newAllocNumMembers;
              //FABRIC_LOG( "realloc(%u)", (unsigned)size );
              bits->memberDatas = static_cast<uint8_t *>( realloc( bits->memberDatas, size ) );
            }
            else
            {
              size_t size = m_memberSize * newAllocNumMembers;
              //FABRIC_LOG( "malloc(%u)", (unsigned)size );
              bits->memberDatas = static_cast<uint8_t *>( malloc( size ) );
            }
            bits->allocNumMembers = newAllocNumMembers;
          }
        }
        bits->numMembers = newNumMembers;
        
        if ( oldNumMembers < newNumMembers )
        {
          if ( !defaultMemberData )
            defaultMemberData = getMemberImpl()->getDefaultData();
          size_t memberSize = getMemberImpl()->getAllocSize();
          uint8_t *memberData = bits->memberDatas + m_memberSize * oldNumMembers;
          uint8_t *memberDataEnd = bits->memberDatas + m_memberSize * newNumMembers;
          memset( memberData, 0, memberDataEnd - memberData );
          // [pzion 20110908] Very special case: if the default data is
          // all 0, we just memset to 0 and leave it at that
          if ( !Util::IsZero( memberSize, defaultMemberData ) )
          {
            for ( ; memberData!=memberDataEnd; memberData += m_memberSize )
            {
              if ( !isMemberShallow() )
                getMemberImpl()->setData( defaultMemberData, memberData );
              else
                memcpy( memberData, defaultMemberData, memberSize );
            }
          }
        }
      }
      //FABRIC_LOG( "VariableArrayImpl::setNumMembers: %fms", ft.getElapsedMS() );
    }

    size_t VariableArrayImpl::getIndirectMemoryUsage( void const *data ) const
    {
      bits_t const *bits = reinterpret_cast<bits_t const *>(data);
      size_t total = bits->allocNumMembers * m_memberImpl->getAllocSize();
      for ( size_t i=0; i<bits->numMembers; ++i )
        total += m_memberImpl->getIndirectMemoryUsage( getImmutableMemberData_NoCheck( data, i ) );
      return total;
    }
    
    bool VariableArrayImpl::isExportable() const
    {
      return m_memberImpl->isExportable();
    }
  }
}

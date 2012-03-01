/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/RT/ContainerImpl.h>
#include <Fabric/Core/DG/Container.h>
#include <Fabric/Base/RC/WeakHandle.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace RT
  {

    std::string ContainerImpl::undefinedName("(uninitialized Container)");

    ContainerImpl::ContainerImpl( std::string const &codeName )
      : Impl( codeName, DT_CONTAINER )
    {
      setSize( sizeof(RC::WeakHandle<DG::Container> const *) );
    }
    
    bool ContainerImpl::isExportable() const
    {
      return false;
    }

    void const *ContainerImpl::getDefaultData() const
    {
      static RC::WeakHandle<DG::Container> const *defaultData = 0;
      return &defaultData;
    }
    
    void ContainerImpl::setData( void const *src, void *dst ) const
    {
      SetData( src, dst );
    }
     
    bool ContainerImpl::equalsData( void const *lhs, void const *rhs ) const
    {
      RC::WeakHandle<DG::Container> const *lhsBits = *static_cast<RC::WeakHandle<DG::Container> const * const *>( lhs );
      RC::WeakHandle<DG::Container> const *rhsBits = *static_cast<RC::WeakHandle<DG::Container> const * const *>( rhs );
      if( lhsBits && rhsBits )
        return *lhsBits == *rhsBits;

      return (!lhsBits || lhsBits->isNull()) && (!rhsBits || rhsBits->isNull());
    }
    
    void ContainerImpl::encodeJSON( void const *data, JSON::Encoder &encoder ) const
    {
      // [JeromeCG 20120209] Could be the DG::Container full name, but decodeJSON would need the DG::Context to convert back to a DG::Container*...
      // Anyway to support a Container as a member we would required others changes if ever we want that...
      throw Exception( "unable to convert Container to JSON" );
    }
    
    void ContainerImpl::decodeJSON( JSON::Entity const &entity, void *dst ) const
    {
      throw Exception( "unable to convert Container from JSON" );
    }

    void ContainerImpl::disposeDatasImpl( void *dst, size_t count, size_t stride ) const
    {
      uint8_t *data = static_cast<uint8_t *>( dst );
      uint8_t * const dataEnd = data + count * stride;
      while ( data != dataEnd )
      {
        DisposeData( data );
        data += stride;
      }
    }
    
    std::string ContainerImpl::descData( void const *data ) const
    {
      std::string result = "Container( ";

      RC::WeakHandle<DG::Container> const *bits = *static_cast<RC::WeakHandle<DG::Container> const * const *>( data );
      if( !bits || bits->isNull() )
        result += "undefined";
      else
        result += bits->makeStrong()->getName();
      return result + " )";
    }
    
    bool ContainerImpl::isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const
    {
      return isContainer( impl->getType() );
    }
    
    bool ContainerImpl::isShallow() const
    {
      return false;
    }

    bool ContainerImpl::isNoAliasSafe() const
    {
      return false;
    }

    size_t ContainerImpl::getIndirectMemoryUsage( void const *data ) const
    {
      return 0;
    }

    void ContainerImpl::setValue( RC::Handle< DG::Container> const &container, void *data ) const
    {
      RC::WeakHandle<DG::Container> *&bits = *static_cast<RC::WeakHandle<DG::Container> **>( data );
      if( container )
      {
        if( !bits )
          bits = new RC::WeakHandle<DG::Container>();

        *bits = container;
      }
      else if( bits )
      {
        *bits = 0;
      }
    }

    RC::Handle<DG::Container> ContainerImpl::getValue( void const *data ) const
    {
      RC::WeakHandle<DG::Container> const *bits = *static_cast<RC::WeakHandle<DG::Container> const * const *>( data );
      if( !bits )
        return 0;
      return bits->makeStrong();
    }

    void ContainerImpl::DisposeData( void *data )
    {
      RC::WeakHandle<DG::Container> *&bits = *reinterpret_cast<RC::WeakHandle<DG::Container> **>( data );
      if ( bits )
      {
        delete bits;
        bits = 0;
      }
    }

    void ContainerImpl::SetData( void const *src, void *dst )
    {
      FABRIC_ASSERT( src );
      FABRIC_ASSERT( dst );
      
      RC::WeakHandle<DG::Container> const *srcBits = *static_cast<RC::WeakHandle<DG::Container> const * const *>( src );
      RC::WeakHandle<DG::Container> *&dstBits = *static_cast<RC::WeakHandle<DG::Container> **>( dst );

      if ( srcBits && !srcBits->isNull() )
      {
        if( !dstBits )
          dstBits = new RC::WeakHandle<DG::Container>();
        *dstBits = *srcBits;
      }
      else
      {
        if ( dstBits )
          *dstBits = 0;
      }
    }


    size_t ContainerImpl::size( void const *data )
    {
      RC::WeakHandle<DG::Container> const *bits = *static_cast<RC::WeakHandle<DG::Container> const * const *>( data );
      if( !bits )
        throw Exception( "Calling size() on an uninitialized Container" );

      return bits->makeStrong()->size();
    }

    void ContainerImpl::resize( void *data, size_t count )
    {
      RC::WeakHandle<DG::Container> *bits = *static_cast<RC::WeakHandle<DG::Container> * const *>( data );
      if( !bits )
        throw Exception( "Calling resize() on an uninitialized Container" );

      return bits->makeStrong()->resize( count );
    }

    std::string ContainerImpl::GetName( void const *data )
    {
      RC::WeakHandle<DG::Container> const *bits = *static_cast<RC::WeakHandle<DG::Container> const * const *>( data );
      if( !bits )
        return undefinedName;
      else
        return bits->makeStrong()->getName();
    }

    bool ContainerImpl::IsValid( void const *data )
    {
      RC::WeakHandle<DG::Container> const *bits = *static_cast<RC::WeakHandle<DG::Container> const * const *>( data );
      return bits && !bits->isNull();
    }
  }
}

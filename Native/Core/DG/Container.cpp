/*
 *  Copyright 2010-2011 Fabric Technologies Inc. All rights reserved.
 */
 
#include <Fabric/Core/DG/Container.h>
#include <Fabric/Core/DG/Scope.h>
#include <Fabric/Core/DG/Context.h>
#include <Fabric/Core/DG/ExecutionEngine.h>
#include <Fabric/Core/DG/Binding.h>
#include <Fabric/Core/DG/FabricResource.h>
#include <Fabric/Core/RT/StructDesc.h>
#include <Fabric/Core/IO/Manager.h>
#include <Fabric/Core/IO/FileHandleManager.h>
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Core/RT/ContainerDesc.h>
#include <Fabric/Core/RT/NumericDesc.h>
#include <Fabric/Core/RT/SlicedArrayDesc.h>
#include <Fabric/Core/RT/SlicedArrayImpl.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/Util/Base64.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace DG
  {
    class Container::Member : public RC::Object
    {
    public:
    
      static RC::Handle<Member> Create( RC::Handle<RT::Manager> const &rtManager, RC::ConstHandle<RT::Desc> memberDesc, size_t size, void const *defaultMemberData )
      {
        return new Member( rtManager, memberDesc, size, defaultMemberData );
      }
      
      void const *getDefaultData() const
      {
        return m_defaultMemberData;
      }
      
      RC::ConstHandle<RT::Desc> getDesc() const
      {
        return m_memberDesc;
      }
      
      void getArrayDescAndData(
        RC::ConstHandle<RT::SlicedArrayDesc> &slicedArrayDesc,
        void *&slicedArrayData
        ) const
      {
        slicedArrayDesc = m_slicedArrayDesc;
        slicedArrayData = m_slicedArrayData;
      }
      
      void const *getImmutableElementData( size_t index ) const
      {
        return m_slicedArrayDesc->getImmutableMemberData( (void const *)m_slicedArrayData, index );
      }
      
      void *getMutableElementData( size_t index )
      {
        return m_slicedArrayDesc->getMutableMemberData( m_slicedArrayData, index );
      }
      
      size_t size() const
      {
        return m_slicedArrayDesc->getNumMembers( m_slicedArrayData );
      }
      
      void resize( size_t newSize )
      {
        m_slicedArrayDesc->setNumMembers( m_slicedArrayData, newSize, m_defaultMemberData );
      }
      
      size_t getMemoryUsage() const
      {
        return m_slicedArrayDesc->getAllocSize() + m_slicedArrayDesc->getIndirectMemoryUsage( m_slicedArrayData );
      }

    protected:
      
      Member( RC::Handle<RT::Manager> const &rtManager, RC::ConstHandle<RT::Desc> memberDesc, size_t size, void const *defaultMemberData );
      ~Member();

    private:
    
      RC::ConstHandle<RT::Desc> m_memberDesc;
      void *m_defaultMemberData;
      RC::ConstHandle<RT::SlicedArrayDesc> m_slicedArrayDesc;
      void *m_slicedArrayData;
    };

    Container::Member::Member( RC::Handle<RT::Manager> const &rtManager, RC::ConstHandle<RT::Desc> memberDesc, size_t size, void const *defaultMemberData )
      : m_memberDesc( memberDesc )
    {
      if ( !defaultMemberData )
        defaultMemberData = memberDesc->getDefaultData();
      
      size_t memberSize = memberDesc->getAllocSize();
      m_defaultMemberData = malloc( memberSize );
      memset( m_defaultMemberData, 0, memberSize );
      memberDesc->setData( defaultMemberData, m_defaultMemberData );
      
      m_slicedArrayDesc = rtManager->getSlicedArrayOf( memberDesc );
      
      size_t arraySize = m_slicedArrayDesc->getAllocSize();
      m_slicedArrayData = malloc( arraySize );
      memset( m_slicedArrayData, 0, arraySize );
      m_slicedArrayDesc->setNumMembers( m_slicedArrayData, size, m_defaultMemberData );
    }
    
    Container::Member::~Member()
    {
      m_slicedArrayDesc->disposeData( m_slicedArrayData );
      free( m_slicedArrayData );
      
      m_memberDesc->disposeData( m_defaultMemberData );
      free( m_defaultMemberData );
    }

    Container::Container( std::string const &name, RC::Handle<Context> const &context )
      : NamedObject( name, context )
      , m_context( context.ptr() )
      , m_size( 1 )
      , m_rtContainerData( 0 )
    {
    }
    
    Container::~Container()
    {
      if( m_rtContainerData )
      {
        RC::ConstHandle<RT::ContainerDesc> desc = m_context->getRTManager()->getContainerDesc();
        desc->disposeData( m_rtContainerData );
        free( m_rtContainerData );
      }
    }
    
    Container::MemberDescs Container::getMemberDescs() const
    {
      MemberDescs result;
      for ( Members::const_iterator it=m_members.begin(); it!=m_members.end(); ++it )
      {
        MemberDesc &memberDesc = result[it->first];
        memberDesc.desc = it->second->getDesc();
        memberDesc.defaultData = it->second->getDefaultData();
      }
      return result;
    }

    void Container::addMember( std::string const &name, RC::ConstHandle<RT::Desc> const &desc, void const *defaultData )
    {
      if ( name.empty() )
        throw Exception( "name must be non-empty" );

      if ( !desc->isNoAliasSafe() )
        throw Exception( "cannot add member " + name + " of type " + desc->getUserName() + ": members cannot contain Container type" );
      
      RC::Handle<Member> member = Member::Create( m_context->getRTManager(), desc, m_size, defaultData );
      bool insertResult = m_members.insert( Members::value_type( name, member ) ).second;
      if ( !insertResult )
        throw Exception( "node already has a member named '"+name+"'" );
      
      markForRecompile();
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        jsonDescMembers( jg );
      }
      jsonNotifyMemberDelta( "members", 7, json );
    }
    
    void Container::removeMember( std::string const &name )
    {
      Members::iterator it = m_members.find( name );
      if ( it == m_members.end() )
        throw Exception( name + ": member not found" );
      m_members.erase( it );
      
      markForRecompile();
      
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        jsonDescMembers( jg );
      }
      jsonNotifyMemberDelta( "members", 7, json );
    }

    void Container::getMemberArrayDescAndData(
      std::string const &name,
      RC::ConstHandle<RT::SlicedArrayDesc> &slicedArrayDesc,
      void *&slicedArrayData
      )
    {
      Members::const_iterator it = m_members.find( name );
      if ( it == m_members.end() )
        throw Exception( _(name) + ": no such member" );
      it->second->getArrayDescAndData( slicedArrayDesc, slicedArrayData );
    }

    void *Container::getRTContainerData()
    {
      if( !m_rtContainerData )
      {
        RC::ConstHandle<RT::ContainerDesc> desc = m_context->getRTManager()->getContainerDesc();
      
        m_rtContainerData = malloc( desc->getAllocSize() );
        memset( m_rtContainerData, 0, desc->getAllocSize() );
        desc->setValue( this, m_rtContainerData );
      }
      return m_rtContainerData;
    }

    size_t Container::size() const
    {
      return m_size;
    }
    
    void Container::resize( size_t size )
    {
      if ( size != m_size )
      {
        for ( Members::const_iterator it=m_members.begin(); it!=m_members.end(); ++it )
          it->second->resize( size );
        m_size = size;

        markForRefresh();
      
        Util::SimpleString json;
        {
          JSON::Encoder jg( &json );
          jsonDescSize( jg );
        }
        jsonNotifyMemberDelta( "size", 4, json );
      }
    }
    
    RC::ConstHandle<Container::Member> Container::getImmutableMember( std::string const &name ) const
    {
      Members::const_iterator it = m_members.find( name );
      if ( it == m_members.end() )
        throw Exception( _(name) + ": no such member" );
      return it->second;
    }

    RC::Handle<Container::Member> Container::getMutableMember( std::string const &name )
    {
      Members::const_iterator it = m_members.find( name );
      if ( it == m_members.end() )
        throw Exception( _(name) + ": no such member" );
      setOutOfDate();
      return it->second;
    }

    RC::ConstHandle< RT::Desc > Container::getDesc( std::string const &name ) const
    {
      return getImmutableMember( name )->getDesc();
    }
    
    void const *Container::getConstData( std::string const &name, size_t index ) const
    {
      if ( index >= m_size )
        throw Exception( "index out of range" );
      return getImmutableMember( name )->getImmutableElementData( index );
    }

    void *Container::getMutableData( std::string const &name, size_t index )
    {
      if ( index >= m_size )
        throw Exception( "index out of range" );
        
      setOutOfDate();
      
      Util::SimpleString json;
      {
        JSON::Encoder jsonEncoder( &json );
        JSON::ObjectEncoder jsonObjectEncoder = jsonEncoder.makeObject();
        
        {
          JSON::Encoder memberNameEncoder = jsonObjectEncoder.makeMember( "memberName", 10 );
          memberNameEncoder.makeString( name );
        }
        
        {
          JSON::Encoder memberIndexEncoder = jsonObjectEncoder.makeMember( "sliceIndex", 10 );
          memberIndexEncoder.makeInteger( index );
        }
      }
      jsonNotify( "dataChange", 10, &json );

      return getMutableMember( name )->getMutableElementData( index );
    }
    
    void Container::getData( std::string const &name, size_t index, void *dstData ) const
    {
      if ( index >= m_size )
        throw Exception( "index out of range" );
      RC::ConstHandle<Member> member = getImmutableMember( name );
      return member->getDesc()->setData( member->getImmutableElementData( index ), dstData );
    }

    void Container::getDataJSON( std::string const &name, size_t index, JSON::Encoder &resultEncoder ) const
    {
      if ( index >= m_size )
        throw Exception( "index out of range" );
      RC::ConstHandle<Member> member = getImmutableMember( name );
      return member->getDesc()->encodeJSON( member->getImmutableElementData( index ), resultEncoder );
    }
    
    void Container::setData( std::string const &name, size_t index, void const *data )
    {
      if ( index >= m_size )
        throw Exception( "index out of range" );
      
      RC::Handle<Member> member = getMutableMember( name );
      setOutOfDate();
      member->getDesc()->setData( data, member->getMutableElementData( index ) );
      
      Util::SimpleString json;
      {
        JSON::Encoder jsonEncoder( &json );
        JSON::ObjectEncoder jsonObjectEncoder = jsonEncoder.makeObject();
        
        {
          JSON::Encoder memberNameEncoder = jsonObjectEncoder.makeMember( "memberName", 10 );
          memberNameEncoder.makeString( name );
        }
        
        {
          JSON::Encoder memberIndexEncoder = jsonObjectEncoder.makeMember( "sliceIndex", 10 );
          memberIndexEncoder.makeInteger( index );
        }
      }
      jsonNotify( "dataChange", 10, &json );
    }
    
    void Container::encodeJSON( JSON::Encoder &encoder ) const
    {
      JSON::ObjectEncoder objectEncoder = encoder.makeObject();
      for ( Members::const_iterator it=m_members.begin(); it!=m_members.end(); ++it )
      {
        std::string const &name = it->first;
        RC::ConstHandle<Member> member = it->second;
        RC::ConstHandle<RT::Desc> memberDesc = member->getDesc();
        JSON::Encoder memberEncoder = objectEncoder.makeMember( name );
        JSON::ArrayEncoder memberArrayEncoder = memberEncoder.makeArray();
        for ( size_t i=0; i<m_size; ++i )
        {
          JSON::Encoder elementEncoder = memberArrayEncoder.makeElement();
          memberDesc->encodeJSON( member->getImmutableElementData(i), elementEncoder );
        }
      }
    }
    
    void Container::generateSliceJSON( size_t index, JSON::Encoder &encoder ) const
    {
      if ( index >= m_size )
        throw Exception( "index "+_(index)+" out of range (slice size is "+_(m_size)+")" );
        
      JSON::ObjectEncoder objectEncoder = encoder.makeObject();
      for ( Members::const_iterator it=m_members.begin(); it!=m_members.end(); ++it )
      {
        std::string const &name = it->first;
        RC::ConstHandle<Member> member = it->second;
        RC::ConstHandle<RT::Desc> memberDesc = member->getDesc();
        JSON::Encoder memberEncoder = objectEncoder.makeMember(name);
        memberDesc->encodeJSON( member->getImmutableElementData( index ), memberEncoder );
      }
    }
    
    void Container::generateMemberJSON( std::string const &memberName, JSON::Encoder &encoder ) const
    {
      Members::const_iterator it = m_members.find( memberName );
      if ( it == m_members.end() )
        throw Exception( "no such member "+_(memberName) );
      RC::ConstHandle<Member> member = it->second;
      RC::ConstHandle<RT::Desc> memberDesc = member->getDesc();
      
      size_t size = member->size();
      JSON::ArrayEncoder jsonArrayEncoder = encoder.makeArray();
      for ( size_t sliceIndex=0; sliceIndex<size; ++sliceIndex )
      {
        JSON::Encoder elementEncoder = jsonArrayEncoder.makeElement();
        memberDesc->encodeJSON( member->getImmutableElementData( sliceIndex ), elementEncoder );
      }
    }

    void Container::setJSON( JSON::Entity const &entity )
    {
      if ( entity.isArray() )
      {
        resize( entity.arraySize() );
        JSON::ArrayDecoder arrayDecoder( entity );
        JSON::Entity elementEntity;
        for ( size_t i=0; i<m_size; ++i )
        {
          FABRIC_VERIFY( arrayDecoder.getNext( elementEntity ) );
          setSliceJSON( i, elementEntity );
        }
        FABRIC_ASSERT( !arrayDecoder.getNext( elementEntity ) );
      }
      else if ( entity.isObject() )
      {
        bool first = true;
        JSON::ObjectDecoder objectDecoder( entity );
        JSON::Entity keyString, valueEntity;
        while ( objectDecoder.getNext( keyString, valueEntity ) )
        {
          std::string name = keyString.stringToStdString();
          Members::const_iterator jt = m_members.find( name );
          if ( jt == m_members.end() )
            throw Exception( "node does not have member named '" + name + "'" );
          RC::Handle<Member> member = jt->second;
          
          try
          {
            valueEntity.requireArray();
            if ( first )
            {
              resize( valueEntity.arraySize() );
              first = false;
            }
            else
            {
              if ( valueEntity.arraySize() != m_size )
                throw Exception( "inconsistent array length" );
            }
            
            RC::ConstHandle<RT::Desc> memberDesc = member->getDesc();
            JSON::ArrayDecoder arrayDecoder( valueEntity );
            JSON::Entity elementEntity;
            for ( size_t i=0; i<m_size; ++i )
            {
              FABRIC_VERIFY( arrayDecoder.getNext( elementEntity ) );
              memberDesc->decodeJSON( elementEntity, member->getMutableElementData(i) );
            }
            FABRIC_ASSERT( !arrayDecoder.getNext( elementEntity ) );
            setOutOfDate();
          }
          catch ( Exception e )
          {
            throw _(name) + ": " + e;
          }
        }
      }
      else throw Exception( "JSON must be an object or an array" );
    }

    void Container::setSliceJSON( size_t index, JSON::Entity const &entity )
    {
      if ( index >= m_size )
        throw Exception( "index "+_(index)+" out of range (slice size is "+_(m_size)+")" );
      
      entity.requireObject();
      
      JSON::ObjectDecoder objectDecoder( entity );
      JSON::Entity keyString, valueEntity;
      while ( objectDecoder.getNext( keyString, valueEntity ) )
      {
        std::string name = keyString.stringToStdString();
        Members::const_iterator jt = m_members.find( name );
        if ( jt == m_members.end() )
          throw Exception( "node does not have member named " + _(name) );
        RC::Handle<Member> member = jt->second;
        
        try
        {
          member->getDesc()->decodeJSON( valueEntity, member->getMutableElementData(index) );
          setOutOfDate();
        }
        catch ( Exception e )
        {
          throw _(name) + ": " + e;
        }
      }
    }
      
    RC::Handle<MT::ParallelCall> Container::bind(
      std::vector<std::string> &errors,
      RC::ConstHandle<Binding> const &binding,
      Scope const &scope,
      unsigned prefixCount,
      void * const *prefixes
      )
    {
      SelfScope selfScope( this, &scope );

      return binding->bind( errors, selfScope, prefixCount, prefixes );
    }
    
    void Container::jsonDescMembers( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      for ( Members::const_iterator it=m_members.begin(); it!=m_members.end(); ++it )
      {
        std::string const &name = it->first;
        RC::ConstHandle<Member> member = it->second;
        RC::ConstHandle<RT::Desc> memberDesc = member->getDesc();
        JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( name );
        JSON::ObjectEncoder memberObjectEncoder = memberEncoder.makeObject();
        memberObjectEncoder.makeMember( "type", 4 ).makeString( memberDesc->getUserName() );
      }
    }
      
    void Container::jsonDescSize( JSON::Encoder &resultEncoder ) const
    {
      resultEncoder.makeInteger( size() );
    }
      
    void Container::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      NamedObject::jsonDesc( resultEncoder );
    }
      
    void Container::jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const
    {
      NamedObject::jsonDesc( resultObjectEncoder );
      
      {
        JSON::Encoder membersEncoder = resultObjectEncoder.makeMember( "members", 7 );
        jsonDescMembers( membersEncoder );
      }
      
      {
        JSON::Encoder sizeEncoder = resultObjectEncoder.makeMember( "size", 4 );
        jsonDescSize( sizeEncoder );
      }
    }
    
    void Container::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "getData", 7 ) )
        jsonExecGetData( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "setData", 7 ) )
        jsonExecSetData( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "getDataJSON", 11 ) )
        jsonExecGetDataJSON( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "getDataSize", 11 ) )
        jsonExecGetDataSize( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "getDataElement", 14 ) )
        jsonExecGetDataElement( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "putResourceToFile", 17 ) )
        jsonExecPutResourceToFile( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "getBulkData", 11 ) )
        jsonExecGetBulkData( resultArrayEncoder );
      else if ( cmd.stringIs( "setBulkData", 11 ) )
        jsonExecSetBulkData( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "getBulkDataJSON", 15 ) )
        jsonExecGetBulkDataJSON( resultArrayEncoder );
      else if ( cmd.stringIs( "setBulkDataJSON", 15 ) )
        jsonExecSetBulkDataJSON( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "getSlicesBulkData", 17 ) )
        jsonExecGetSlicesBulkData( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "setSlicesBulkData", 17 ) )
        jsonExecSetSlicesBulkData( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "getMembersBulkData", 18 ) )
        jsonExecGetMembersBulkData( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "addMember", 9 ) )
        jsonExecAddMember( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "removeMember", 12 ) )
        jsonExecRemoveMember( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "resize", 6 ) )
        jsonResize( arg, resultArrayEncoder );
      else
        NamedObject::jsonExec( cmd, arg, resultArrayEncoder );
    }

    void Container::jsonExecAddMember( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      std::string name;
      RC::ConstHandle<RT::Desc> desc;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity, defaultValueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "name", 4 ) )
          {
            valueEntity.requireString();
            name = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "type", 4 ) )
          {
            valueEntity.requireString();
            std::string typeName = valueEntity.stringToStdString();
            desc = m_context->getRTManager()->getDesc( typeName );
          }
          else if ( keyString.stringIs( "defaultValue", 12 ) )
          {
            defaultValueEntity = valueEntity;
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }

      std::vector<uint8_t> defaultValue;
      if ( defaultValueEntity )
      {
        defaultValue.resize( desc->getAllocSize(), 0 );
        desc->decodeJSON( defaultValueEntity, &defaultValue[0] );
      }
      
      if ( name.empty() )
        throw Exception( "missing 'name'" );
      if ( !desc )
        throw Exception( "missing 'desc'" );
      
      addMember( name, desc, defaultValue.size()>0? &defaultValue[0]: 0 );

      if ( defaultValue.size() > 0 )
        desc->disposeData( &defaultValue[0] );
    }

    void Container::jsonExecRemoveMember( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      std::string name = arg.stringToStdString();
      removeMember( name );
    }

    void Container::jsonResize( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireInteger();
      int32_t newSize = arg.integerValue();
      if ( newSize < 0 )
        throw Exception( "size must be non-negative" );
      resize( size_t( newSize ) );
    }
    
    void Container::jsonGenerateMemberSliceJSON( JSON::Entity const &arg, JSON::Encoder &resultEncoder ) const
    {
      RC::ConstHandle<Member> member;
      size_t sliceIndex = SIZE_MAX;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "memberName", 10 ) )
          {
            valueEntity.requireString();
            std::string memberName = valueEntity.stringToStdString();
            member = getImmutableMember( memberName );
          }
          else if ( keyString.stringIs( "sliceIndex", 10 ) )
          {
            valueEntity.requireInteger();
            int32_t sliceIndexInt32 = valueEntity.integerValue();
            if ( sliceIndexInt32 < 0 || size_t(sliceIndexInt32) > m_size )
              throw Exception( "out of range" );
            sliceIndex = size_t( sliceIndexInt32 );
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }

      if ( !member )
        throw Exception( "missing 'memberName'" );
      if ( sliceIndex == SIZE_MAX )
        throw Exception( "missing 'sliceIndex'" );

      member->getDesc()->encodeJSON( member->getImmutableElementData( sliceIndex ), resultEncoder );
    }
      
    void Container::jsonExecGetData( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      jsonGenerateMemberSliceJSON( arg, resultEncoder );
    }
      
    void Container::jsonExecGetDataJSON( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      Util::SimpleString json;
      {
        JSON::Encoder jsonEncoder( &json );
        jsonGenerateMemberSliceJSON( arg, jsonEncoder );
      }
      
      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      resultEncoder.makeString( json );
    }
      
    void Container::jsonExecGetDataSize( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      std::string memberName;
      size_t sliceIndex = SIZE_MAX;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "memberName", 10 ) )
          {
            valueEntity.requireString();
            memberName = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "sliceIndex", 10 ) )
          {
            valueEntity.requireInteger();
            int32_t sliceIndexInt32 = valueEntity.integerValue();
            if ( sliceIndexInt32 < 0 || size_t(sliceIndexInt32) > m_size )
              throw Exception( "out of range" );
            sliceIndex = size_t( sliceIndexInt32 );
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( memberName.empty() )
        throw Exception( "missing 'memberName'" );
      if ( sliceIndex == SIZE_MAX )
        throw Exception( "missing 'sliceIndex'" );
      
      RC::ConstHandle<RT::Desc> desc = getDesc( memberName );
      if ( !RT::isArray( desc->getType() ) )
        throw Exception( "member type is not an array" );
      RC::ConstHandle<RT::ArrayDesc> arrayDesc = RC::ConstHandle<RT::ArrayDesc>::StaticCast( desc );
      
      void const *data = getConstData( memberName, sliceIndex );
      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      resultEncoder.makeInteger( arrayDesc->getNumMembers( data ) );
    }
      
    void Container::jsonExecGetDataElement( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      std::string memberName;
      size_t sliceIndex = SIZE_MAX;
      size_t elementIndex = SIZE_MAX;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "memberName", 10 ) )
          {
            valueEntity.requireString();
            memberName = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "sliceIndex", 10 ) )
          {
            valueEntity.requireInteger();
            int32_t sliceIndexInt32 = valueEntity.integerValue();
            if ( sliceIndexInt32 < 0 || size_t(sliceIndexInt32) > m_size )
              throw Exception( "out of range" );
            sliceIndex = size_t( sliceIndexInt32 );
          }
          else if ( keyString.stringIs( "elementIndex", 12 ) )
          {
            valueEntity.requireInteger();
            int32_t elementIndexInt32 = valueEntity.integerValue();
            if ( elementIndexInt32 < 0 )
              throw Exception( "out of range" );
            elementIndex = size_t( elementIndexInt32 );
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( memberName.empty() )
        throw Exception( "missing 'memberName'" );
      if ( sliceIndex == SIZE_MAX )
        throw Exception( "missing 'sliceIndex'" );
      if ( elementIndex == SIZE_MAX )
        throw Exception( "missing 'elementIndex'" );
      
      RC::ConstHandle<RT::Desc> desc = getDesc( memberName );
      if ( !RT::isArray( desc->getType() ) )
        throw Exception( "member type is not an array" );
      RC::ConstHandle<RT::ArrayDesc> arrayDesc = RC::ConstHandle<RT::ArrayDesc>::StaticCast( desc );
      
      void const *data = getConstData( memberName, sliceIndex );
      
      void const *memberData = arrayDesc->getImmutableMemberData( data, elementIndex );
      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      arrayDesc->getMemberDesc()->encodeJSON( memberData, resultEncoder );
    }
      
    void Container::jsonExecSetData( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      std::string memberName;
      size_t sliceIndex = SIZE_MAX;
      JSON::Entity dataEntity;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "memberName", 10 ) )
          {
            valueEntity.requireString();
            memberName = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "sliceIndex", 10 ) )
          {
            valueEntity.requireInteger();
            int32_t sliceIndexInt32 = valueEntity.integerValue();
            if ( sliceIndexInt32 < 0 || size_t(sliceIndexInt32) > m_size )
              throw Exception( "out of range" );
            sliceIndex = size_t( sliceIndexInt32 );
          }
          else if ( keyString.stringIs( "data", 4 ) )
          {
            dataEntity = valueEntity;
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( memberName.empty() )
        throw Exception( "missing 'memberName'" );
      if ( sliceIndex == SIZE_MAX )
        throw Exception( "missing 'sliceIndex'" );
      if ( !dataEntity )
        throw Exception( "missing 'data'" );
      
      getDesc( memberName )->decodeJSON( dataEntity, getMutableData( memberName, sliceIndex ) );
    }
      
    void Container::jsonExecGetBulkData( JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      encodeJSON( resultEncoder );
    }
      
    void Container::jsonExecGetBulkDataJSON( JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      Util::SimpleString json;
      {
        JSON::Encoder jg( &json );
        encodeJSON( jg );
      }
      
      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      resultEncoder.makeString( json );
    }
      
    void Container::jsonExecSetBulkData( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      setJSON( arg );
    }
      
    void Container::jsonExecSetBulkDataJSON( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireString();
      std::string jsonString = arg.stringToStdString();
      JSON::Decoder jsonStringDecoder( jsonString.data(), jsonString.length() );
      JSON::Entity jsonEntity;
      if ( !jsonStringDecoder.getNext( jsonEntity ) )
        throw Exception( "missing JSON entity" );
      setJSON( jsonEntity );
      if ( jsonStringDecoder.getNext( jsonEntity ) )
        throw Exception( "extra JSON entity" );
    }
      
    void Container::jsonExecGetSlicesBulkData( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      JSON::ArrayEncoder slicesArrayEncoder = resultEncoder.makeArray();

      arg.requireArray();
      JSON::ArrayDecoder argArrayDecoder( arg );
      JSON::Entity elementEntity;
      size_t i = 0;
      while ( argArrayDecoder.getNext( elementEntity ) )
      {
        try
        {
          elementEntity.requireInteger();
          int32_t indexInt32 = elementEntity.integerValue();
          size_t index = size_t(indexInt32);
          if ( indexInt32 < 0 || index >= m_size )
            throw Exception( "out of range" );
          
          JSON::Encoder sliceEncoder = slicesArrayEncoder.makeElement();
          generateSliceJSON( index, sliceEncoder );
        }
        catch ( Exception e )
        {
          throw "index " + _(i) + ": " + e;
        }
        
        ++i;
      }
    }
      
    void Container::jsonExecGetMembersBulkData( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      JSON::Encoder resultEncoder = resultArrayEncoder.makeElement();
      JSON::ObjectEncoder membersObjectEncoder = resultEncoder.makeObject();

      arg.requireArray();
      JSON::ArrayDecoder argArrayDecoder( arg );
      JSON::Entity elementEntity;
      while ( argArrayDecoder.getNext( elementEntity ) )
      {
        try
        {
          elementEntity.requireString();
          std::string memberName = elementEntity.stringToStdString();
          JSON::Encoder memberEncoder = membersObjectEncoder.makeMember( memberName );
          generateMemberJSON( memberName, memberEncoder );
        }
        catch ( Exception e )
        {
          argArrayDecoder.rethrow( e );
        }
      }
    }
      
    void Container::jsonExecSetSlicesBulkData( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      arg.requireArray();
      JSON::ArrayDecoder argArrayDecoder( arg );
      JSON::Entity elementEntity;
      while ( argArrayDecoder.getNext( elementEntity ) )
      {
        try
        {
          size_t sliceIndex = SIZE_MAX;
          JSON::Entity dataEntity;

          elementEntity.requireObject();
          JSON::ObjectDecoder elementObjectDecoder( elementEntity );
          JSON::Entity keyString, valueEntity;
          while ( elementObjectDecoder.getNext( keyString, valueEntity ) )
          {
            try
            {
              if ( keyString.stringIs( "sliceIndex", 10 ) )
              {
                valueEntity.requireInteger();
                int32_t sliceIndexInt32 = valueEntity.integerValue();
                sliceIndex = size_t( sliceIndexInt32 );
                if ( sliceIndexInt32 < 0 || sliceIndex >= m_size )
                  throw Exception( "out of range" );
              }
              else if ( keyString.stringIs( "data", 4 ) )
              {
                dataEntity = valueEntity;
              }
            }
            catch ( Exception e )
            {
              elementObjectDecoder.rethrow( e );
            }
          }
          
          if ( sliceIndex == SIZE_MAX )
            throw Exception( "missing 'sliceIndex'" );
          if ( !dataEntity )
            throw Exception( "missing 'data'" );
          
          setSliceJSON( sliceIndex, dataEntity );
        }
        catch ( Exception e )
        {
          argArrayDecoder.rethrow( e );
        }
      }
    }

    void Container::jsonExecPutResourceToFile( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const
    {
      std::string memberName, handle;
      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "memberName", 10 ) )
          {
            valueEntity.requireString();
            memberName = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "file", 4 ) )
          {
            valueEntity.requireString();
            handle = valueEntity.stringToStdString();
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( memberName.empty() )
        throw Exception( "missing 'memberName'" );
      if ( handle.empty() )
        throw Exception( "missing 'file'" );

      RC::ConstHandle<Container::Member> member = getImmutableMember( memberName );
      RC::ConstHandle<RT::Desc> desc = member->getDesc();
      if ( desc->getUserName() != "FabricResource" )
        throw Exception( "member " + _(memberName) + " is not of type FabricResource" );

      FabricResourceWrapper resource( m_context->getRTManager(), (void*)member->getImmutableElementData( 0 ) );

      std::string dataExternalLocation = resource.getDataExternalLocation();
      if( dataExternalLocation.empty() )
        m_context->getIOManager()->getFileHandleManager()->putFile( handle, resource.getDataSize(), resource.getDataPtr(), false );
      else
        m_context->getIOManager()->getFileHandleManager()->copyFile( dataExternalLocation, handle );
    }

    void Container::jsonGetMemoryUsage( JSON::Encoder &encoder ) const
    {
      JSON::ObjectEncoder objectEncoder = encoder.makeObject();
      for ( Members::const_iterator it = m_members.begin(); it != m_members.end(); ++it )
      {
        JSON::Encoder memberEncoder = objectEncoder.makeMember( it->first );
        RC::Handle<Member> const &member = it->second;
        memberEncoder.makeInteger( int32_t( member->getMemoryUsage() ) );
      }
    }
  };
};

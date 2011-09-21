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
#include <Fabric/Core/MT/LogCollector.h>
#include <Fabric/Core/RT/NumericDesc.h>
#include <Fabric/Core/RT/VariableArrayDesc.h>
#include <Fabric/Core/RT/SlicedArrayDesc.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/RT/VariableArrayImpl.h>
#include <Fabric/Base/JSON/Value.h>
#include <Fabric/Base/JSON/Object.h>
#include <Fabric/Base/JSON/Array.h>
#include <Fabric/Base/JSON/Integer.h>
#include <Fabric/Base/JSON/String.h>
#include <Fabric/Core/Util/Base64.h>
#include <Fabric/Core/Util/Encoder.h>
#include <Fabric/Core/Util/Decoder.h>
#include <Fabric/Base/Util/SimpleString.h>

namespace Fabric
{
  namespace DG
  {
    class Container::Member : public RC::Object
    {
    public:
    
      static RC::Handle<Member> Create( RC::Handle<RT::Manager> const &rtManager, RC::ConstHandle<RT::Desc> memberDesc, size_t count, void const *defaultMemberData )
      {
        return new Member( rtManager, memberDesc, count, defaultMemberData );
      }
      
      void const *getDefaultData() const
      {
        return m_defaultMemberData;
      }
      
      RC::ConstHandle<RT::Desc> getDesc() const
      {
        return m_memberDesc;
      }
      
      void getDescs( RC::ConstHandle<RT::Desc> &memberDesc, RC::ConstHandle<RT::VariableArrayDesc> &variableArrayDesc, RC::ConstHandle<RT::SlicedArrayDesc> &slicedArrayDesc ) const
      {
        memberDesc = m_memberDesc;
        variableArrayDesc = m_variableArrayDesc;
        slicedArrayDesc = m_slicedArrayDesc;
      }
      
      void *getArrayData() const
      {
        return m_arrayData;
      }
      
      void const *getElementData( size_t index ) const
      {
        return m_variableArrayDesc->getMemberData( (void const *)m_arrayData, index );
      }
      
      void *getElementData( size_t index )
      {
        return m_variableArrayDesc->getMemberData( m_arrayData, index );
      }
      
      void resize( size_t newCount )
      {
        m_variableArrayDesc->setNumMembers( m_arrayData, newCount, m_defaultMemberData );
      }

    protected:
      
      Member( RC::Handle<RT::Manager> const &rtManager, RC::ConstHandle<RT::Desc> memberDesc, size_t count, void const *defaultMemberData );
      ~Member();

    private:
    
      RC::ConstHandle<RT::Desc> m_memberDesc;
      void *m_defaultMemberData;
      RC::ConstHandle<RT::VariableArrayDesc> m_variableArrayDesc;
      RC::ConstHandle<RT::SlicedArrayDesc> m_slicedArrayDesc;
      void *m_arrayData;
    };

    Container::Container( std::string const &name, RC::Handle<Context> const &context )
      : NamedObject( name, context )
      , m_context( context.ptr() )
      , m_count( 1 )
    {
    }
    
    Container::~Container()
    {
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
      if ( name.length() == 0 )
        throw Exception( "name must be non-empty" );
      
      RC::Handle<Member> member = Member::Create( m_context->getRTManager(), desc, m_count, defaultData );
      bool insertResult = m_members.insert( Members::value_type( name, member ) ).second;
      if ( !insertResult )
        throw Exception( "node already has a member named '"+name+"'" );
      
      markForRecompile();
      
      notifyDelta( "members", jsonDescMembers() );
    }
    
    void Container::removeMember( std::string const &name )
    {
      Members::iterator it = m_members.find( name );
      if ( it == m_members.end() )
        throw Exception( name + ": member not found" );
      m_members.erase( it );
      
      markForRecompile();
      
      notifyDelta( "members", jsonDescMembers() );
    }

    void Container::getMemberDescs( std::string const &name, RC::ConstHandle<RT::Desc> &memberDesc, RC::ConstHandle<RT::VariableArrayDesc> &variableArrayDesc, RC::ConstHandle<RT::SlicedArrayDesc> &slicedArrayDesc )
    {
      Members::const_iterator it = m_members.find( name );
      if ( it == m_members.end() )
        throw Exception( "'" + name + "': no such member" );
      it->second->getDescs( memberDesc, variableArrayDesc, slicedArrayDesc );
    }
    
    void *Container::getMemberArrayData( std::string const &name )
    {
      Members::const_iterator it = m_members.find( name );
      if ( it == m_members.end() )
        throw Exception( "'" + name + "': no such member" );
      return it->second->getArrayData();
    }

    size_t Container::getCount() const
    {
      return m_count;
    }
    
    void Container::setCount( size_t count )
    {
      if ( count != m_count )
      {
        for ( Members::const_iterator it=m_members.begin(); it!=m_members.end(); ++it )
          it->second->resize( count );
        m_count = count;

        markForRefresh();
      
        notifyDelta( "count", jsonDescCount() );
      }
    }
    
    RC::ConstHandle<Container::Member> Container::getMember( std::string const &name ) const
    {
      Members::const_iterator it = m_members.find( name );
      if ( it == m_members.end() )
        throw Exception( _(name) + ": no such member" );
      return it->second;
    }

    RC::Handle<Container::Member> Container::getMember( std::string const &name )
    {
      Members::const_iterator it = m_members.find( name );
      if ( it == m_members.end() )
        throw Exception( _(name) + ": no such member" );
      setOutOfDate();
      return it->second;
    }

    RC::ConstHandle< RT::Desc > Container::getDesc( std::string const &name ) const
    {
      return getMember( name )->getDesc();
    }
    
    void const *Container::getConstData( std::string const &name, size_t index ) const
    {
      if ( index >= m_count )
        throw Exception( "index out of range" );
      return getMember( name )->getElementData( index );
    }

    void *Container::getMutableData( std::string const &name, size_t index )
    {
      if ( index >= m_count )
        throw Exception( "index out of range" );
        
      setOutOfDate();
      
      RC::Handle<JSON::Object> dataChangeJSONObject = JSON::Object::Create();
      dataChangeJSONObject->set( "memberName", JSON::String::Create( name ) );
      dataChangeJSONObject->set( "sliceIndex", JSON::Integer::Create( index ) );
      notify( "dataChange", dataChangeJSONObject );

      return getMember( name )->getElementData( index );
    }
    
    void Container::getData( std::string const &name, size_t index, void *dstData ) const
    {
      if ( index >= m_count )
        throw Exception( "index out of range" );
      RC::ConstHandle<Member> member = getMember( name );
      return member->getDesc()->setData( member->getElementData( index ), dstData );
    }

    void Container::setData( std::string const &name, size_t index, void const *data )
    {
      if ( index >= m_count )
        throw Exception( "index out of range" );
      
      RC::Handle<Member> member = getMember( name );
      setOutOfDate();
      member->getDesc()->setData( data, member->getElementData( index ) );
      
      RC::Handle<JSON::Object> dataChangeJSONObject = JSON::Object::Create();
      dataChangeJSONObject->set( "memberName", JSON::String::Create( name ) );
      dataChangeJSONObject->set( "sliceIndex", JSON::Integer::Create( index ) );
      notify( "dataChange", dataChangeJSONObject );
    }
    
    RC::ConstHandle<JSON::Value> Container::getJSON() const
    {
      RC::Handle<JSON::Object> result = JSON::Object::Create();
      for ( Members::const_iterator it=m_members.begin(); it!=m_members.end(); ++it )
      {
        RC::ConstHandle<Member> member = it->second;
        RC::ConstHandle<RT::Desc> memberDesc = member->getDesc();
        RC::Handle<JSON::Array> memberJSONArray = JSON::Array::Create();
        for ( size_t i=0; i<m_count; ++i )
          memberJSONArray->push_back( memberDesc->getJSONValue( member->getElementData( i ) ) );
        
        std::string const &name = it->first;
        result->set( name, memberJSONArray );
      }
      return result;
    }
    
    RC::ConstHandle<JSON::Value> Container::getSliceJSON( size_t index ) const
    {
      if ( index >= m_count )
        throw Exception( "index "+_(index)+" out of range (slice count is "+_(m_count)+")" );
        
      RC::Handle<JSON::Object> result = JSON::Object::Create();
      for ( Members::const_iterator it=m_members.begin(); it!=m_members.end(); ++it )
      {
        RC::ConstHandle<Member> member = it->second;
        RC::ConstHandle<RT::Desc> memberDesc = member->getDesc();
        std::string const &name = it->first;
        result->set( name, memberDesc->getJSONValue( member->getElementData( index ) ) );
      }
      return result;
    }

    void Container::setJSON( RC::ConstHandle<JSON::Value> const &value )
    {
      if ( value->isArray() )
      {
        RC::ConstHandle<JSON::Array> const &arrayValue = RC::ConstHandle<JSON::Array>::StaticCast( value );
        setCount( arrayValue->size() );
        
        for ( size_t i=0; i<m_count; ++i )
          setSliceJSON( i, arrayValue->get(i) );
      }
      else if ( value->isObject() )
      {
        RC::ConstHandle<JSON::Object> objectValue = RC::ConstHandle<JSON::Object>::StaticCast( value );
        
        for ( JSON::Object::const_iterator it=objectValue->begin(); it!=objectValue->end(); ++it )
        {
          std::string const &name = it->first;
          Members::const_iterator jt = m_members.find( name );
          if ( jt == m_members.end() )
            throw Exception( "node does not have member named '" + name + "'" );
          RC::Handle<Member> member = jt->second;
          
          try
          {
            RC::ConstHandle<JSON::Array> const &arrayValue = it->second->toArray();
            if ( arrayValue->size() != m_count )
              throw Exception( "array length ("+_(arrayValue->size())+") does not match slice count ("+_(m_count)+")" );
          
            RC::ConstHandle<RT::Desc> memberDesc = member->getDesc();
            for ( size_t i=0; i<m_count; ++i )
              memberDesc->setDataFromJSONValue( arrayValue->get(i), member->getElementData(i) );
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

    void Container::setSliceJSON( size_t index, RC::ConstHandle<JSON::Value> const &value )
    {
      if ( index >= m_count )
        throw Exception( "index "+_(index)+" out of range (slice count is "+_(m_count)+")" );
        
      if ( !value->isObject() )
        throw Exception( "malformed JSON" );
      RC::ConstHandle<JSON::Object> objectValue = RC::ConstHandle<JSON::Object>::StaticCast( value );
      
      for ( JSON::Object::const_iterator it=objectValue->begin(); it!=objectValue->end(); ++it )
      {
        std::string const &name = it->first;
        Members::const_iterator jt = m_members.find( name );
        if ( jt == m_members.end() )
          throw Exception( "node does not have member named " + _(name) );
        RC::Handle<Member> member = jt->second;
        
        try
        {
          member->getDesc()->setDataFromJSONValue( it->second, member->getElementData(index) );
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
      size_t *newCount,
      unsigned prefixCount,
      void * const *prefixes
      )
    {
      SelfScope selfScope( this, &scope );

      return binding->bind( errors, selfScope, newCount, prefixCount, prefixes );
    }

    Container::Member::Member( RC::Handle<RT::Manager> const &rtManager, RC::ConstHandle<RT::Desc> memberDesc, size_t count, void const *defaultMemberData )
      : m_memberDesc( memberDesc )
    {
      if ( !defaultMemberData )
        defaultMemberData = memberDesc->getDefaultData();
      
      size_t memberSize = memberDesc->getSize();
      m_defaultMemberData = malloc( memberSize );
      memset( m_defaultMemberData, 0, memberSize );
      memberDesc->setData( defaultMemberData, m_defaultMemberData );
      
      m_variableArrayDesc = rtManager->getVariableArrayOf( memberDesc );
      m_slicedArrayDesc = rtManager->getSlicedArrayOf( memberDesc );
      
      size_t arraySize = m_variableArrayDesc->getSize();
      m_arrayData = malloc( arraySize );
      memset( m_arrayData, 0, arraySize );
      m_variableArrayDesc->setNumMembers( m_arrayData, count, m_defaultMemberData );
    }
    
    Container::Member::~Member()
    {
      m_variableArrayDesc->disposeData( m_arrayData );
      free( m_arrayData );
      
      m_memberDesc->disposeData( m_defaultMemberData );
      free( m_defaultMemberData );
    }
    
    RC::ConstHandle<JSON::Value> Container::jsonDescMembers() const
    {
      RC::Handle<JSON::Object> membersObject = JSON::Object::Create();
      for ( Members::const_iterator it=m_members.begin(); it!=m_members.end(); ++it )
      {
        RC::ConstHandle<Member> member = it->second;
        RC::ConstHandle<RT::Desc> memberDesc = member->getDesc();
        RC::Handle<JSON::Object> memberObject = JSON::Object::Create();
        memberObject->set( "type", JSON::String::Create( memberDesc->getName() ) );
        
        std::string const &name = it->first;
        membersObject->set( name, memberObject );
      }
      return membersObject;
    }
      
    RC::ConstHandle<JSON::Value> Container::jsonDescCount() const
    {
      return JSON::Integer::Create( getCount() );
    }
      
    RC::Handle<JSON::Object> Container::jsonDesc() const
    {
      RC::Handle<JSON::Object> result = NamedObject::jsonDesc();
      result->set( "members", jsonDescMembers() );
      result->set( "count", jsonDescCount() );
      return result;
    }
    
    RC::ConstHandle<JSON::Value> Container::jsonExec( std::string const &cmd, RC::ConstHandle<JSON::Value> const &arg )
    {
      RC::ConstHandle<JSON::Value> result;
      if ( cmd == "getData" )
        result = jsonExecGetData( arg );
      else if ( cmd == "getDataSize" )
        result = jsonExecGetDataSize( arg );
      else if ( cmd == "getDataElement" )
        result = jsonExecGetDataElement( arg );
      else if ( cmd == "putResourceToUserFile" )
        jsonExecPutResourceToFile( arg, true );
      else if ( cmd == "getResourceFromUserFile" )
        jsonExecGetResourceFromFile( arg, true );
      else if ( cmd == "putResourceToFile" )
        jsonExecPutResourceToFile( arg, false );
      else if ( cmd == "getResourceFromFile" )
        jsonExecGetResourceFromFile( arg, false );
      else if ( cmd == "setData" )
        jsonExecSetData( arg );
      else if ( cmd == "getBulkData" )
        result = jsonExecGetBulkData();
      else if ( cmd == "setBulkData" )
        jsonExecSetBulkData( arg );
      else if ( cmd == "getSlicesBulkData" )
        result = jsonExecGetSlicesBulkData( arg );
      else if ( cmd == "setSlicesBulkData" )
        jsonExecSetSlicesBulkData( arg );
      else if ( cmd == "addMember" )
        jsonExecAddMember( arg );
      else if ( cmd == "removeMember" )
        jsonExecRemoveMember( arg );
      else if ( cmd == "setCount" )
        jsonSetCount( arg );
      else
        result = NamedObject::jsonExec( cmd, arg );
      return result;
    }

    void Container::jsonExecAddMember( RC::ConstHandle<JSON::Value> const &arg )
    {
      RC::ConstHandle<JSON::Object> argJSONObject = arg->toObject();
      
      std::string name;
      try
      {
        name = argJSONObject->get( "name" )->toString()->value();
      }
      catch ( Exception e )
      {
        throw "'name': " + e;
      }
      
      RC::ConstHandle<RT::Desc> desc;
      try
      {
        std::string type = argJSONObject->get( "type" )->toString()->value();
        desc = m_context->getRTManager()->getDesc( type );
      }
      catch ( Exception e )
      {
        throw "'desc': " + e;
      }
      
      std::vector<uint8_t> defaultValue;
      try
      {
        RC::ConstHandle<JSON::Value> defaultValueJSONValue = argJSONObject->maybeGet( "defaultValue" );
        if ( defaultValueJSONValue )
        {
          defaultValue.resize( desc->getSize() );
          desc->setDataFromJSONValue( defaultValueJSONValue, &defaultValue[0] );
        }
      }
      catch ( Exception e )
      {
        throw "'defaultValue': " + e;
      }
      
      addMember( name, desc, defaultValue.size()>0? &defaultValue[0]: 0 );

      if ( defaultValue.size() > 0 )
        desc->disposeData( &defaultValue[0] );
    }

    void Container::jsonExecRemoveMember( RC::ConstHandle<JSON::Value> const &arg )
    {
      removeMember( arg->toString()->value() );
    }

    void Container::jsonSetCount( RC::ConstHandle<JSON::Value> const &arg )
    {
      int32_t newCount = arg->toInteger()->value();
      if ( newCount < 0 )
        throw Exception( "count must be non-negative" );
      setCount( size_t( newCount ) );
    }
      
    RC::ConstHandle<JSON::Value> Container::jsonExecGetData( RC::ConstHandle<JSON::Value> const &arg )
    {
      RC::ConstHandle<JSON::Object> argJSONObject = arg->toObject();
      
      std::string memberName;
      try
      {
        memberName = argJSONObject->get( "memberName" )->toString()->value();
      }
      catch ( Exception e )
      {
        throw "'memberName': " + e;
      }
      
      RC::ConstHandle<RT::Desc> desc = getDesc( memberName );

      size_t sliceIndex;
      try
      {
        sliceIndex = argJSONObject->get( "sliceIndex" )->toInteger()->value();
      }
      catch ( Exception e )
      {
        throw "'sliceIndex': " + e;
      }
      
      void const *data = getConstData( memberName, sliceIndex );
      return desc->getJSONValue( data );
    }
      
    RC::ConstHandle<JSON::Integer> Container::jsonExecGetDataSize( RC::ConstHandle<JSON::Value> const &arg )
    {
      RC::ConstHandle<JSON::Object> argJSONObject = arg->toObject();
      
      std::string memberName;
      try
      {
        memberName = argJSONObject->get( "memberName" )->toString()->value();
      }
      catch ( Exception e )
      {
        throw "'memberName': " + e;
      }
      
      RC::ConstHandle<RT::Desc> desc = getDesc( memberName );
      if ( !RT::isArray( desc->getType() ) )
        throw Exception( "member type is not an array" );
      RC::ConstHandle<RT::ArrayDesc> arrayDesc = RC::ConstHandle<RT::ArrayDesc>::StaticCast( desc );
      
      size_t sliceIndex;
      try
      {
        sliceIndex = argJSONObject->get( "sliceIndex" )->toInteger()->value();
      }
      catch ( Exception e )
      {
        throw "'sliceIndex': " + e;
      }
      
      void const *data = getConstData( memberName, sliceIndex );
      return JSON::Integer::Create( arrayDesc->getNumMembers( data ) );
    }
      
    RC::ConstHandle<JSON::Value> Container::jsonExecGetDataElement( RC::ConstHandle<JSON::Value> const &arg )
    {
      RC::ConstHandle<JSON::Object> argJSONObject = arg->toObject();
      
      std::string memberName;
      try
      {
        memberName = argJSONObject->get( "memberName" )->toString()->value();
      }
      catch ( Exception e )
      {
        throw "'memberName': " + e;
      }
      
      RC::ConstHandle<RT::Desc> desc = getDesc( memberName );
      if ( !RT::isArray( desc->getType() ) )
        throw Exception( "member type is not an array" );
      RC::ConstHandle<RT::ArrayDesc> arrayDesc = RC::ConstHandle<RT::ArrayDesc>::StaticCast( desc );
      
      size_t sliceIndex;
      try
      {
        sliceIndex = argJSONObject->get( "sliceIndex" )->toInteger()->value();
      }
      catch ( Exception e )
      {
        throw "'sliceIndex': " + e;
      }
      
      void const *data = getConstData( memberName, sliceIndex );

      size_t elementIndex;
      try
      {
        elementIndex = argJSONObject->get( "elementIndex" )->toInteger()->value();
      }
      catch ( Exception e )
      {
        throw "'elementIndex': " + e;
      }
      
      void const *memberData = arrayDesc->getMemberData( data, elementIndex );
      return arrayDesc->getMemberDesc()->getJSONValue( memberData );
    }
      
    void Container::jsonExecSetData( RC::ConstHandle<JSON::Value> const &arg )
    {
      RC::ConstHandle<JSON::Object> argJSONObject = arg->toObject();
      
      std::string memberName;
      try
      {
        memberName = argJSONObject->get( "memberName" )->toString()->value();
      }
      catch ( Exception e )
      {
        throw "'memberName': " + e;
      }
      
      size_t sliceIndex;
      try
      {
        sliceIndex = argJSONObject->get( "sliceIndex" )->toInteger()->value();
      }
      catch ( Exception e )
      {
        throw "'sliceIndex': " + e;
      }
      
      RC::ConstHandle<JSON::Value> dataJSONValue;
      try
      {
        dataJSONValue = argJSONObject->get( "data" );
      }
      catch ( Exception e )
      {
        throw "'data': " + e;
      }
      
      getDesc( memberName )->setDataFromJSONValue( dataJSONValue, getMutableData( memberName, sliceIndex ) );
    }
      
    RC::ConstHandle<JSON::Value> Container::jsonExecGetBulkData() const
    {
      return getJSON();
    }
      
    void Container::jsonExecSetBulkData( RC::ConstHandle<JSON::Value> const &arg )
    {
      setJSON( arg );
    }
      
    RC::ConstHandle<JSON::Value> Container::jsonExecGetSlicesBulkData( RC::ConstHandle<JSON::Value> const &arg ) const
    {
      RC::ConstHandle<JSON::Array> argJSONArray = arg->toArray();
      size_t argJSONArraySize = argJSONArray->size();
      
      RC::Handle<JSON::Array> result = JSON::Array::Create();
      for ( size_t i=0; i<argJSONArraySize; ++i )
      {
        try
        {
          size_t index = argJSONArray->get( i )->toInteger()->value();
          result->push_back( getSliceJSON( index ) );
        }
        catch ( Exception e )
        {
          throw "index " + _(i) + ": " + e;
        }
      }
      return result;
    }
      
    void Container::jsonExecSetSlicesBulkData( RC::ConstHandle<JSON::Value> const &arg )
    {
      RC::ConstHandle<JSON::Array> argJSONArray = arg->toArray();
      size_t argJSONArraySize = argJSONArray->size();
      for ( size_t i=0; i<argJSONArraySize; ++i )
      {
        try
        {
          RC::ConstHandle<JSON::Object> argJSONObject = argJSONArray->get(i)->toObject();
          
          size_t sliceIndex;
          try
          {
            sliceIndex = argJSONObject->get( "sliceIndex" )->toInteger()->value();
          }
          catch ( Exception e )
          {
            throw "'sliceIndex': " + e;
          }
          
          RC::ConstHandle<JSON::Value> dataJSONValue;
          try
          {
            dataJSONValue = argJSONObject->get( "data" );
          }
          catch ( Exception e )
          {
            throw "'data': " + e;
          }
          
          setSliceJSON( sliceIndex, dataJSONValue );
        }
        catch ( Exception e )
        {
          throw "index " + _(i) + ": " + e;
        }
      }
    }

    void* Container::jsonGetResourceMember( RC::ConstHandle<JSON::Value> const &arg, std::string& memberName ) const
    {
      RC::ConstHandle<JSON::Object> argJSONObject = arg->toObject();
      
      try
      {
        memberName = argJSONObject->get( "memberName" )->toString()->value();
      }
      catch ( Exception e )
      {
        throw "'memberName': " + e;
      }

      RC::ConstHandle<Container::Member> member = getMember( memberName );
      RC::ConstHandle<RT::Desc> desc = member->getDesc();
      if( desc->getName() != "FabricResource" )
      {
        throw Exception( "member" + memberName + " is not of type FabricResource" );
      }
      return (void*)member->getElementData( 0 );
    }

    struct FabricResourceByteContainerAdapter : public IO::Manager::ByteContainer
    {
      FabricResourceByteContainerAdapter( FabricResourceWrapper& resource )
        : m_resource(resource)
      {}

      virtual void* Allocate( size_t size )
      {
        m_resource.resizeData( size );
        return (void*)m_resource.getDataPtr();
      }

      FabricResourceWrapper& m_resource;
    };

    void Container::jsonExecGetResourceFromFile( RC::ConstHandle<JSON::Value> const &arg, bool userFile )
    {
      std::string memberName;
      FabricResourceWrapper resourceMember( m_context->getRTManager(), jsonGetResourceMember( arg, memberName ) );

      //Load to a temporary resource then swap to preserve existing resource in case an exception is thrown
      FabricResourceWrapper tempResource( m_context->getRTManager() );
      FabricResourceByteContainerAdapter tempResourceAdapter(tempResource);

      std::string filename, extension;
      if( userFile )
        m_context->getIOManager()->jsonExecGetUserFile( arg, tempResourceAdapter, true, filename, extension );
      else
        m_context->getIOManager()->jsonExecGetFile( arg, tempResourceAdapter, true, filename, extension );

      tempResource.setExtension( extension );
      tempResource.setURL( filename );
      setData( memberName, 0, tempResource.get() );
    }

    void Container::jsonExecPutResourceToFile( RC::ConstHandle<JSON::Value> const &arg, bool userFile ) const
    {
      std::string memberName;
      FabricResourceWrapper resource( m_context->getRTManager(), jsonGetResourceMember( arg, memberName ) );
      if( userFile )
        m_context->getIOManager()->jsonExecPutUserFile( arg, resource.getDataSize(), resource.getDataPtr(), resource.getExtension().c_str() );
      else
        m_context->getIOManager()->jsonExecPutFile( arg, resource.getDataSize(), resource.getDataPtr() );
    }

  };
};

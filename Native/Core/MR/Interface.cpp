/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/MR/Interface.h>
#include <Fabric/Core/MR/ArrayGeneratorWrapper.h>
#include <Fabric/Core/MR/ArrayMapWrapper.h>
#include <Fabric/Core/MR/ArrayTransformWrapper.h>
#include <Fabric/Core/MR/ConstArrayWrapper.h>
#include <Fabric/Core/MR/ConstValueWrapper.h>
#include <Fabric/Core/MR/ReduceWrapper.h>
#include <Fabric/Core/MR/ValueCacheWrapper.h>
#include <Fabric/Core/MR/ArrayCacheWrapper.h>
#include <Fabric/Core/MR/ValueGeneratorWrapper.h>
#include <Fabric/Core/MR/ValueMapWrapper.h>
#include <Fabric/Core/MR/ValueTransformWrapper.h>
#include <Fabric/Core/KLC/ArrayGeneratorOperatorWrapper.h>
#include <Fabric/Core/KLC/ArrayMapOperatorWrapper.h>
#include <Fabric/Core/KLC/ArrayTransformOperatorWrapper.h>
#include <Fabric/Core/KLC/ReduceOperatorWrapper.h>
#include <Fabric/Core/KLC/ValueGeneratorOperatorWrapper.h>
#include <Fabric/Core/KLC/ValueMapOperatorWrapper.h>
#include <Fabric/Core/KLC/ValueTransformOperatorWrapper.h>
#include <Fabric/Core/GC/Object.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Base/JSON/Decoder.h>
#include <Fabric/Base/JSON/Encoder.h>
#include <Fabric/Core/Util/Parse.h>

namespace Fabric
{
  namespace MR
  {
    Interface::Interface(
      GC::Container *gcContainer,
      RC::ConstHandle<RT::Manager> const &rtManager
      )
      : m_gcContainer( gcContainer )
      , m_rtManager( rtManager )
    {
    }
  
    void Interface::jsonRoute(
      std::vector<JSON::Entity> const &dst,
      size_t dstOffset,
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( dst.size() - dstOffset == 0 )
        jsonExec( cmd, arg, resultArrayEncoder );
      else m_gcContainer->jsonRoute( dst, dstOffset, cmd, arg, resultArrayEncoder );
    }
      
    void Interface::jsonExec(
      JSON::Entity const &cmd,
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      if ( cmd.stringIs( "createConstValue", 16 ) )
        jsonExecCreateConstValue( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createValueGenerator", 20 ) )
        jsonExecCreateValueGenerator( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createValueMap", 14 ) )
        jsonExecCreateValueMap( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createValueTransform", 20 ) )
        jsonExecCreateValueTransform( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createValueCache", 16 ) )
        jsonExecCreateValueCache( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createConstArray", 16 ) )
        jsonExecCreateConstArray( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createArrayGenerator", 20 ) )
        jsonExecCreateArrayGenerator( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createArrayMap", 14 ) )
        jsonExecCreateArrayMap( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createArrayTransform", 20 ) )
        jsonExecCreateArrayTransform( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createArrayCache", 16 ) )
        jsonExecCreateArrayCache( arg, resultArrayEncoder );
      else if ( cmd.stringIs( "createReduce", 12 ) )
        jsonExecCreateReduce( arg, resultArrayEncoder );
      else throw Exception( "unknown command: " + _(cmd) );
    }
    
    void Interface::jsonExecCreateConstArray(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      RC::ConstHandle<RT::Desc> elementTypeRTDesc;
      JSON::Entity dataEntity;
      std::string jsonData;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "elementType", 11 ) )
          {
            valueEntity.requireString();
            std::string elementTypeName = valueEntity.stringToStdString();
            elementTypeRTDesc = m_rtManager->getDesc( elementTypeName );
          }
          else if ( keyString.stringIs( "data", 4 ) )
          {
            dataEntity = valueEntity;
          }
          else if ( keyString.stringIs( "jsonData", 8 ) )
          {
            valueEntity.requireString();
            jsonData = valueEntity.stringToStdString();
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      if ( !elementTypeRTDesc )
        throw Exception( "missing 'elementType'" );
        
      if ( !jsonData.empty() )
      {
        if ( dataEntity )
          throw Exception( "cannot have both 'data' and 'jsonData'" );
        
        JSON::Decoder jsonDataDecoder( jsonData.data(), jsonData.length() );
        try
        {
          if ( !jsonDataDecoder.getNext( dataEntity ) )
            throw Exception( "missing JSON entity" );
          JSON::Entity extraEntity;
          if ( jsonDataDecoder.getNext( extraEntity ) )
            throw Exception( "extra JSON entity" );
        }
        catch ( Exception e )
        {
          throw "'jsonData': " + e;
        }
      }
      
      if ( dataEntity )
      {
        ConstArrayWrapper::Create(
          m_rtManager,
          elementTypeRTDesc,
          dataEntity
          )->reg( m_gcContainer, id_ );
      }
      else throw Exception( "must have exactly one of 'data' and 'jsonData'" );
    }
    
    void Interface::jsonExecCreateArrayMap(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      RC::Handle<ArrayProducerWrapper> input;
      RC::Handle<KLC::ArrayMapOperatorWrapper> operator_;
      RC::Handle<ValueProducerWrapper> shared;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "inputID", 7 ) )
          {
            valueEntity.requireString();
            std::string inputID = valueEntity.stringToStdString();
            input = GC::DynCast<ArrayProducerWrapper>( m_gcContainer->getObject( inputID ) );
            if ( !input )
              throw "must be an array producer";
          }
          else if ( keyString.stringIs( "operatorID", 10 ) )
          {
            valueEntity.requireString();
            std::string operatorID = valueEntity.stringToStdString();
            operator_ = GC::DynCast<KLC::ArrayMapOperatorWrapper>( m_gcContainer->getObject( operatorID ) );
            if ( !operator_ )
              throw "must be an array map operator";
          }
          else if ( keyString.stringIs( "sharedID", 8 ) )
          {
            valueEntity.requireString();
            std::string sharedID = valueEntity.stringToStdString();
            shared = GC::DynCast<ValueProducerWrapper>( m_gcContainer->getObject( sharedID ) );
            if ( !shared )
              throw "must be a value producer";
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      if ( !input )
        throw Exception( "missing 'inputID'" );
      if ( !operator_ )
        throw Exception( "missing 'operatorID'" );
      
      ArrayMapWrapper::Create(
        input,
        operator_,
        shared
        )->reg( m_gcContainer, id_ );
    }
    
    void Interface::jsonExecCreateValueMap(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      RC::Handle<ValueProducerWrapper> input;
      RC::Handle<KLC::ValueMapOperatorWrapper> operator_;
      RC::Handle<ValueProducerWrapper> shared;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "inputID", 7 ) )
          {
            valueEntity.requireString();
            std::string inputID = valueEntity.stringToStdString();
            input = GC::DynCast<ValueProducerWrapper>( m_gcContainer->getObject( inputID ) );
            if ( !input )
              throw "must be a value producer";
          }
          else if ( keyString.stringIs( "operatorID", 10 ) )
          {
            valueEntity.requireString();
            std::string operatorID = valueEntity.stringToStdString();
            operator_ = GC::DynCast<KLC::ValueMapOperatorWrapper>( m_gcContainer->getObject( operatorID ) );
            if ( !operator_ )
              throw "must be a value map operator";
          }
          else if ( keyString.stringIs( "sharedID", 8 ) )
          {
            valueEntity.requireString();
            std::string sharedID = valueEntity.stringToStdString();
            shared = GC::DynCast<ValueProducerWrapper>( m_gcContainer->getObject( sharedID ) );
            if ( !shared )
              throw "must be a value producer";
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      if ( !input )
        throw Exception( "missing 'inputID'" );
      if ( !operator_ )
        throw Exception( "missing 'operatorID'" );
      
      ValueMapWrapper::Create(
        input,
        operator_,
        shared
        )->reg( m_gcContainer, id_ );
    }
    
    void Interface::jsonExecCreateValueTransform(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      RC::Handle<ValueProducerWrapper> input;
      RC::Handle<KLC::ValueTransformOperatorWrapper> operator_;
      RC::Handle<ValueProducerWrapper> shared;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "inputID", 7 ) )
          {
            valueEntity.requireString();
            std::string inputID = valueEntity.stringToStdString();
            input = GC::DynCast<ValueProducerWrapper>( m_gcContainer->getObject( inputID ) );
            if ( !input )
              throw "must be a value producer";
          }
          else if ( keyString.stringIs( "operatorID", 10 ) )
          {
            valueEntity.requireString();
            std::string operatorID = valueEntity.stringToStdString();
            operator_ = GC::DynCast<KLC::ValueTransformOperatorWrapper>( m_gcContainer->getObject( operatorID ) );
            if ( !operator_ )
              throw "must be a value transform operator";
          }
          else if ( keyString.stringIs( "sharedID", 8 ) )
          {
            valueEntity.requireString();
            std::string sharedID = valueEntity.stringToStdString();
            shared = GC::DynCast<ValueProducerWrapper>( m_gcContainer->getObject( sharedID ) );
            if ( !shared )
              throw "must be a value producer";
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      if ( !input )
        throw Exception( "missing 'inputID'" );
      if ( !operator_ )
        throw Exception( "missing 'operatorID'" );
      
      ValueTransformWrapper::Create(
        input,
        operator_,
        shared
        )->reg( m_gcContainer, id_ );
    }
    
    void Interface::jsonExecCreateValueGenerator(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      RC::Handle<KLC::ValueGeneratorOperatorWrapper> operator_;
      RC::Handle<ValueProducerWrapper> shared;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "operatorID", 10 ) )
          {
            valueEntity.requireString();
            std::string operatorID = valueEntity.stringToStdString();
            operator_ = GC::DynCast<KLC::ValueGeneratorOperatorWrapper>( m_gcContainer->getObject( operatorID ) );
            if ( !operator_ )
              throw "must be a value generator operator";
          }
          else if ( keyString.stringIs( "sharedID", 8 ) )
          {
            valueEntity.requireString();
            std::string sharedID = valueEntity.stringToStdString();
            shared = GC::DynCast<ValueProducerWrapper>( m_gcContainer->getObject( sharedID ) );
            if ( !shared )
              throw "must be a value producer";
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      if ( !operator_ )
        throw Exception( "missing 'operatorID'" );
      
      ValueGeneratorWrapper::Create(
        operator_,
        shared
        )->reg( m_gcContainer, id_ );
    }
 
    void Interface::jsonExecCreateArrayCache(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      RC::Handle<ArrayProducerWrapper> input;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "inputID", 7 ) )
          {
            valueEntity.requireString();
            std::string inputID = valueEntity.stringToStdString();
            input = GC::DynCast<ArrayProducerWrapper>( m_gcContainer->getObject( inputID ) );
            if ( !input )
              throw "must be an array producer";
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      if ( !input )
        throw Exception( "missing 'inputID'" );
      
      ArrayCacheWrapper::Create(
        input
        )->reg( m_gcContainer, id_ );
    }
    
    void Interface::jsonExecCreateValueCache(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      RC::Handle<ValueProducerWrapper> input;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "inputID", 7 ) )
          {
            valueEntity.requireString();
            std::string inputID = valueEntity.stringToStdString();
            input = GC::DynCast<ValueProducerWrapper>( m_gcContainer->getObject( inputID ) );
            if ( !input )
              throw "must be a value producer";
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      if ( !input )
        throw Exception( "missing 'inputID'" );
      
      ValueCacheWrapper::Create(
        input
        )->reg( m_gcContainer, id_ );
    }
    
    void Interface::jsonExecCreateArrayTransform(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      RC::Handle<ArrayProducerWrapper> input;
      RC::Handle<KLC::ArrayTransformOperatorWrapper> operator_;
      RC::Handle<ValueProducerWrapper> shared;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "inputID", 7 ) )
          {
            valueEntity.requireString();
            std::string inputID = valueEntity.stringToStdString();
            input = GC::DynCast<ArrayProducerWrapper>( m_gcContainer->getObject( inputID ) );
            if ( !input )
              throw "must be an array producer";
          }
          else if ( keyString.stringIs( "operatorID", 10 ) )
          {
            valueEntity.requireString();
            std::string operatorID = valueEntity.stringToStdString();
            operator_ = GC::DynCast<KLC::ArrayTransformOperatorWrapper>( m_gcContainer->getObject( operatorID ) );
            if ( !operator_ )
              throw "must be an array transform operator";
          }
          else if ( keyString.stringIs( "sharedID", 8 ) )
          {
            valueEntity.requireString();
            std::string sharedID = valueEntity.stringToStdString();
            shared = GC::DynCast<ValueProducerWrapper>( m_gcContainer->getObject( sharedID ) );
            if ( !shared )
              throw "must be a value producer";
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      if ( !input )
        throw Exception( "missing 'inputID'" );
      if ( !operator_ )
        throw Exception( "missing 'operatorID'" );
      
      ArrayTransformWrapper::Create(
        input,
        operator_,
        shared
        )->reg( m_gcContainer, id_ );
    }
    
    void Interface::jsonExecCreateReduce(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      RC::Handle<ArrayProducerWrapper> input;
      RC::Handle<KLC::ReduceOperatorWrapper> operator_;
      RC::Handle<ValueProducerWrapper> shared;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "inputID", 7 ) )
          {
            valueEntity.requireString();
            std::string inputID = valueEntity.stringToStdString();
            input = GC::DynCast<ArrayProducerWrapper>( m_gcContainer->getObject( inputID ) );
            if ( !input )
              throw "must be an array producer";
          }
          else if ( keyString.stringIs( "operatorID", 10 ) )
          {
            valueEntity.requireString();
            std::string operatorID = valueEntity.stringToStdString();
            operator_ = GC::DynCast<KLC::ReduceOperatorWrapper>( m_gcContainer->getObject( operatorID ) );
            if ( !operator_ )
              throw "must be a reduce operator";
          }
          else if ( keyString.stringIs( "sharedID", 8 ) )
          {
            valueEntity.requireString();
            std::string sharedID = valueEntity.stringToStdString();
            shared = GC::DynCast<ValueProducerWrapper>( m_gcContainer->getObject( sharedID ) );
            if ( !shared )
              throw "must be a value producer";
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      if ( !input )
        throw Exception( "missing 'inputID'" );
      if ( !operator_ )
        throw Exception( "missing 'operatorID'" );
      
      ReduceWrapper::Create(
        input,
        operator_,
        shared
        )->reg( m_gcContainer, id_ );
    }
    
    void Interface::jsonExecCreateArrayGenerator(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      RC::Handle<ValueProducerWrapper> count;
      RC::Handle<KLC::ArrayGeneratorOperatorWrapper> operator_;
      RC::Handle<ValueProducerWrapper> shared;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "countID", 7 ) )
          {
            valueEntity.requireString();
            std::string countID = valueEntity.stringToStdString();
            count = GC::DynCast<ValueProducerWrapper>( m_gcContainer->getObject( countID ) );
            if ( !count )
              throw "must be a value producer";
          }
          else if ( keyString.stringIs( "operatorID", 10 ) )
          {
            valueEntity.requireString();
            std::string operatorID = valueEntity.stringToStdString();
            operator_ = GC::DynCast<KLC::ArrayGeneratorOperatorWrapper>( m_gcContainer->getObject( operatorID ) );
            if ( !operator_ )
              throw "must be an array generator operator";
          }
          else if ( keyString.stringIs( "sharedID", 8 ) )
          {
            valueEntity.requireString();
            std::string sharedID = valueEntity.stringToStdString();
            shared = GC::DynCast<ValueProducerWrapper>( m_gcContainer->getObject( sharedID ) );
            if ( !shared )
              throw "must be a value producer";
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      if ( !count )
        throw Exception( "missing 'countID'" );
      if ( !operator_ )
        throw Exception( "missing 'operatorID'" );
      
      ArrayGeneratorWrapper::Create(
        count,
        operator_,
        shared
        )->reg( m_gcContainer, id_ );
    }
    
    void Interface::jsonExecCreateConstValue(
      JSON::Entity const &arg,
      JSON::ArrayEncoder &resultArrayEncoder
      )
    {
      std::string id_;
      RC::ConstHandle<RT::Desc> valueTypeRTDesc;
      JSON::Entity dataEntity;

      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "id", 2 ) )
          {
            valueEntity.requireString();
            id_ = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "valueType", 9 ) )
          {
            valueEntity.requireString();
            std::string valueTypeName = valueEntity.stringToStdString();
            valueTypeRTDesc = m_rtManager->getDesc( valueTypeName );
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
      
      if ( id_.empty() )
        throw Exception( "missing 'id'" );
      if ( !valueTypeRTDesc )
        throw Exception( "missing 'valueType'" );
      if ( !dataEntity )
        throw Exception( "missing 'data'" );
        
      ConstValueWrapper::Create(
        m_rtManager,
        valueTypeRTDesc,
        dataEntity
        )->reg( m_gcContainer, id_ );
    }
  }
}

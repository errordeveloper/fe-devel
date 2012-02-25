/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Manager.h"
#include "ArrayProducerDesc.h"
#include "ArrayProducerImpl.h"
#include "BooleanDesc.h"
#include "BooleanImpl.h"
#include "ConstStringDesc.h"
#include "ConstStringImpl.h"
#include "ContainerDesc.h"
#include "ContainerImpl.h"
#include "DictDesc.h"
#include "DictImpl.h"
#include "FixedArrayDesc.h"
#include "FixedArrayImpl.h"
#include "FloatDesc.h"
#include "FloatImpl.h"
#include "IntegerDesc.h"
#include "IntegerImpl.h"
#include "StringDesc.h"
#include "StringImpl.h"
#include "ValueProducerDesc.h"
#include "ValueProducerImpl.h"
#include "VariableArrayDesc.h"
#include "VariableArrayImpl.h"
#include "SlicedArrayDesc.h"
#include "SlicedArrayImpl.h"
#include "StructDesc.h"
#include "StructImpl.h"
#include "OpaqueDesc.h"
#include "OpaqueImpl.h"

#include <Fabric/Core/JSON/CommandChannel.h>
#include <Fabric/Base/Config.h>
#include <Fabric/Base/Util/Format.h>
#include <Fabric/Core/Util/Debug.h>

namespace Fabric
{
  namespace RT
  {
    RC::Handle<Manager> Manager::Create( RC::ConstHandle<KLCompiler> const &klCompiler )
    {
#if defined( FABRIC_OS_WINDOWS )
      _set_output_format( _TWO_DIGIT_EXPONENT );
#endif

      return new Manager( klCompiler );
    }

    Manager::Manager( RC::ConstHandle<KLCompiler> const &klCompiler )
      : m_jsonCommandChannel( 0 )
      , m_klCompiler( klCompiler )
    {
      registerDesc( m_booleanDesc = new BooleanDesc( "Boolean", new BooleanImpl( "Boolean" ) ) );
      registerDesc( m_byteDesc = new ByteDesc( "Byte", new ByteImpl( "Byte" ) ) );
      registerDesc( m_integerDesc = new SI32Desc( "Integer", new SI32Impl( "Integer" ) ) );
      registerDesc( m_sizeDesc = new SizeDesc( "Size", new SizeImpl( "Size" ) ) );
      m_indexDesc = registerAlias( "Index", m_sizeDesc );
      registerDesc( m_fp32Desc = new Float32Desc( "Float32", new Float32Impl( "Float32" ) ) );
      registerAlias( "Scalar", m_fp32Desc );
      registerDesc( m_fp64Desc = new Float64Desc( "Float64", new Float64Impl( "Float64" ) ) );
      registerDesc( m_stringDesc = new StringDesc( "String", new StringImpl( "String" ) ) );
      registerDesc( m_dataDesc = new OpaqueDesc( "Data", new OpaqueImpl( "Data", sizeof(size_t) ) ) );
      registerDesc( m_constStringDesc = new ConstStringDesc( "ConstString", new ConstStringImpl( "ConstString" ) ) );
      registerDesc( m_containerDesc = new ContainerDesc( "Container", new ContainerImpl( "Container" ) ) );
    }
    
    void Manager::setJSONCommandChannel( JSON::CommandChannel *jsonCommandChannel )
    {
      m_jsonCommandChannel = jsonCommandChannel;
    }
    
    RC::ConstHandle<VariableArrayDesc> Manager::getVariableArrayOf( RC::ConstHandle<RT::Desc> const &memberDesc ) const
    {
      std::string variableArrayName = memberDesc->getUserName() + "[]";
      RC::ConstHandle<VariableArrayImpl> variableArrayImpl = memberDesc->getImpl()->getVariableArrayImpl();
      RC::ConstHandle<VariableArrayDesc> variableArrayDesc = new VariableArrayDesc( variableArrayName, variableArrayImpl, memberDesc );
      return RC::ConstHandle<VariableArrayDesc>::StaticCast( registerDesc( variableArrayDesc ) );
    }

    RC::ConstHandle<SlicedArrayDesc> Manager::getSlicedArrayOf( RC::ConstHandle<RT::Desc> const &memberDesc ) const
    {
      std::string slicedArrayName = memberDesc->getUserName() + "<>";
      RC::ConstHandle<SlicedArrayImpl> slicedArrayImpl = memberDesc->getImpl()->getSlicedArrayImpl();
      RC::ConstHandle<SlicedArrayDesc> slicedArrayDesc = new SlicedArrayDesc( slicedArrayName, slicedArrayImpl, memberDesc );
      return RC::ConstHandle<SlicedArrayDesc>::StaticCast( registerDesc( slicedArrayDesc ) );
    }

    RC::ConstHandle<FixedArrayDesc> Manager::getFixedArrayOf( RC::ConstHandle<RT::Desc> const &memberDesc, size_t length ) const
    {
      std::string fixedArrayName = memberDesc->getUserName() + "[" + _(length) + "]";
      RC::ConstHandle<FixedArrayImpl> fixedArrayImpl = memberDesc->getImpl()->getFixedArrayImpl( length );
      RC::ConstHandle<FixedArrayDesc> fixedArrayDesc = new FixedArrayDesc( fixedArrayName, fixedArrayImpl, memberDesc );
      return RC::ConstHandle<FixedArrayDesc>::StaticCast( registerDesc( fixedArrayDesc ) );
    }

    RC::ConstHandle<DictDesc> Manager::getDictOf(
      RC::ConstHandle<RT::ComparableDesc> const &keyDesc,
      RC::ConstHandle<RT::Desc> const &valueDesc
      ) const
    {
      std::string dictName = valueDesc->getUserName() + "[" + keyDesc->getUserName() + "]";
      RC::ConstHandle<DictImpl> dictImpl = valueDesc->getImpl()->getDictImpl( keyDesc->getImpl() );
      RC::ConstHandle<DictDesc> dictDesc = new DictDesc( dictName, dictImpl, keyDesc, valueDesc );
      return RC::ConstHandle<DictDesc>::StaticCast( registerDesc( dictDesc ) );
    }

    RC::ConstHandle<ValueProducerDesc> Manager::getValueProducerOf(
      RC::ConstHandle<RT::Desc> const &valueDesc
      ) const
    {
      std::string valueProducerName = "ValueProducer<" + valueDesc->getUserName() + ">";
      RC::ConstHandle<ValueProducerImpl> valueProducerImpl = valueDesc->getImpl()->getValueProducerImpl();
      RC::ConstHandle<ValueProducerDesc> valueProducerDesc = new ValueProducerDesc( valueProducerName, valueProducerImpl, valueDesc );
      return RC::ConstHandle<ValueProducerDesc>::StaticCast( registerDesc( valueProducerDesc ) );
    }

    RC::ConstHandle<ArrayProducerDesc> Manager::getArrayProducerOf(
      RC::ConstHandle<RT::Desc> const &elementDesc
      ) const
    {
      std::string arrayProducerName = "ArrayProducer<" + elementDesc->getUserName() + ">";
      RC::ConstHandle<ArrayProducerImpl> arrayProducerImpl = elementDesc->getImpl()->getArrayProducerImpl();
      RC::ConstHandle<ArrayProducerDesc> arrayProducerDesc = new ArrayProducerDesc( arrayProducerName, arrayProducerImpl, elementDesc );
      return RC::ConstHandle<ArrayProducerDesc>::StaticCast( registerDesc( arrayProducerDesc ) );
    }

    Manager::Types const &Manager::getTypes() const
    {
      return m_types;
    }

    RC::ConstHandle<StructDesc> Manager::registerStruct( std::string const &name, StructMemberInfoVector const &memberInfos )
    {
      RC::ConstHandle<StructDesc> structDesc = new StructDesc( name, new StructImpl( name, memberInfos ) );
      return RC::ConstHandle<StructDesc>::StaticCast( registerDesc( structDesc ) );
    }

    RC::ConstHandle<OpaqueDesc> Manager::registerOpaque( std::string const &name, size_t size )
    {
      RC::ConstHandle<OpaqueDesc> result = new OpaqueDesc( name, new OpaqueImpl( name, size ) );
      return RC::ConstHandle<OpaqueDesc>::StaticCast( registerDesc( result ) );
    }

    RC::ConstHandle<Desc> Manager::registerAlias( std::string const &name, RC::ConstHandle<RT::Desc> const &desc )
    {
      RC::ConstHandle<Desc> aliasDesc;
      switch ( desc->getType() )
      {
        case DT_BOOLEAN:
          aliasDesc = new BooleanDesc( name, RC::ConstHandle<BooleanImpl>::StaticCast( desc->getImpl() ) );
          break;
        case DT_INTEGER:
          aliasDesc = new IntegerDesc( name, RC::ConstHandle<IntegerImpl>::StaticCast( desc->getImpl() ) );
          break;
        case DT_FLOAT:
          aliasDesc = new FloatDesc( name, RC::ConstHandle<FloatImpl>::StaticCast( desc->getImpl() ) );
          break;
        case DT_CONST_STRING:
          aliasDesc = new ConstStringDesc( name, RC::ConstHandle<ConstStringImpl>::StaticCast( desc->getImpl() ) );
          break;
        case DT_STRING:
          aliasDesc = new StringDesc( name, RC::ConstHandle<StringImpl>::StaticCast( desc->getImpl() ) );
          break;
        case DT_OPAQUE:
          aliasDesc = new OpaqueDesc( name, RC::ConstHandle<OpaqueImpl>::StaticCast( desc->getImpl() ) );
          break;
        case DT_STRUCT:
          aliasDesc = new StructDesc( name, RC::ConstHandle<StructImpl>::StaticCast( desc->getImpl() ) );
          break;
        case DT_VARIABLE_ARRAY:
          aliasDesc = new VariableArrayDesc( name, RC::ConstHandle<VariableArrayImpl>::StaticCast( desc->getImpl() ), RC::ConstHandle<VariableArrayDesc>::StaticCast( desc )->getMemberDesc() );
          break;
        case DT_FIXED_ARRAY:
          aliasDesc = new FixedArrayDesc( name, RC::ConstHandle<FixedArrayImpl>::StaticCast( desc->getImpl() ), RC::ConstHandle<FixedArrayDesc>::StaticCast( desc )->getMemberDesc() );
          break;
        default:
          FABRIC_ASSERT( false && "Unhandled type" );
          break;
      };
      return registerDesc( aliasDesc );
    }
    
    RC::ConstHandle<Desc> Manager::registerDesc( RC::ConstHandle<RT::Desc> const &desc ) const
    {
      std::string const &name = desc->getUserName();
      std::pair< Types::iterator, bool > insertResult = m_types.insert( Types::value_type( name, desc ) );
      if ( !insertResult.second )
      {
        RC::ConstHandle< RT::Desc > existingDesc = insertResult.first->second;
        if ( desc->isEquivalentTo( existingDesc ) )
          return existingDesc;
        throw Exception( "type '" + name + "' already registered with a different definition" );
      }
      
      if ( desc->isExportable() )
      {
        if ( m_jsonCommandChannel )
        {
          std::vector<std::string> dst;
          dst.push_back( "RT" );
          dst.push_back( desc->getUserName() );
          
          Util::SimpleString json;
          {
            JSON::Encoder jg( &json );
            desc->jsonDesc( jg );
          }
          m_jsonCommandChannel->jsonNotify( dst, "delta", 5, &json );
        }
      }
      
      return desc;
    }
    
    static inline bool isFirstTypeNameChar( char ch )
    {
      return isalpha( ch ) || ch == '_';
    }
    
    static inline bool isRemainingTypeNameChar( char ch )
    {
      return isFirstTypeNameChar( ch ) || isdigit( ch );
    }

    RC::ConstHandle<RT::Desc> Manager::maybeGetBaseDesc( std::string const &baseName ) const
    {
      RC::ConstHandle<RT::Desc> baseDesc;
      Types::const_iterator it = m_types.find( baseName );
      if ( it != m_types.end() )
        baseDesc = it->second;
      return baseDesc;
    }

    RC::ConstHandle<RT::Desc> Manager::maybeGetDesc( std::string const &name ) const
    {
      char const *data = name.data();
      char const *dataEnd = data + name.length();
      
      if ( data == dataEnd || !isFirstTypeNameChar( *data ) )
        throw Exception( "malformed type expression" );
      char const *baseNameStart = data++;
      
      while ( data != dataEnd && isRemainingTypeNameChar( *data ) )
        ++data;
      char const *baseNameEnd = data;
      
      std::string baseName( baseNameStart, baseNameEnd - baseNameStart );
      if ( baseName == "ValueProducer" )
      {
        if ( baseNameEnd == dataEnd || *baseNameEnd != '<' )
          throw Exception( "malformed type expression" );
        ++baseNameEnd;
          
        // [pzion 20111228] count angle brackets to get subtype
        
        size_t bracketCount = 0;
        char const *type = baseNameEnd;
        char const *typeEnd = type;
        for (;;)
        {
          if ( typeEnd == dataEnd )
            throw Exception( "malformed type expression" );
          else if ( *typeEnd == '>' )
          {
            if ( bracketCount == 0 )
              break;
            else --bracketCount;
          }
          else if ( *typeEnd == '<' )
            ++bracketCount;
          ++typeEnd;
        }
        
        RC::ConstHandle<Desc> valueDesc = maybeGetDesc( std::string( type, typeEnd - type ) );

        RC::ConstHandle<RT::Desc> desc;
        if ( valueDesc )
          desc = getValueProducerOf( valueDesc );
        return desc;
      }
      else if ( baseName == "ArrayProducer" )
      {
        if ( baseNameEnd == dataEnd || *baseNameEnd != '<' )
          throw Exception( "malformed type expression" );
        ++baseNameEnd;
          
        // [pzion 20111228] count angle brackets to get subtype
        
        size_t bracketCount = 0;
        char const *type = baseNameEnd;
        char const *typeEnd = type;
        for (;;)
        {
          if ( typeEnd == dataEnd )
            throw Exception( "malformed type expression" );
          else if ( *typeEnd == '>' )
          {
            if ( bracketCount == 0 )
              break;
            else --bracketCount;
          }
          else if ( *typeEnd == '<' )
            ++bracketCount;
          ++typeEnd;
        }
        
        RC::ConstHandle<Desc> valueDesc = maybeGetDesc( std::string( type, typeEnd - type ) );

        RC::ConstHandle<RT::Desc> desc;
        if ( valueDesc )
          desc = getArrayProducerOf( valueDesc );
        return desc;
      }
      else
      {
        RC::ConstHandle<RT::Desc> desc = maybeGetBaseDesc( baseName );
        if ( desc )
          desc = getComplexDesc( desc, data, dataEnd );
        return desc;
      }
    }

    RC::ConstHandle<RT::Desc> Manager::getDesc( std::string const &name ) const
    {
      RC::ConstHandle<RT::Desc> result = maybeGetDesc( name );
      if ( !result )
        throw Exception( _(name) + ": type not registered" );
      return result;
    }
    
    RC::ConstHandle<RT::Desc> Manager::getComplexDesc( RC::ConstHandle<RT::Desc> const &desc, char const *data, char const *dataEnd ) const
    {
      if ( data != dataEnd && *data == '[' )
      {
        ++data;
        
        if ( data != dataEnd && *data == ']' )
        {
          ++data;
          return getVariableArrayOf( getComplexDesc( desc, data, dataEnd ) );
        }
        else if ( data != dataEnd && isdigit( *data ) )
        {
          size_t length = 0;
          
          while ( data != dataEnd && isdigit( *data ) )
          {
            length = 10 * length + (*data - '0');
            ++data;
          }
          
          if ( data == dataEnd || *data != ']' )
            throw Exception( "malformed type expression" );
          ++data;
          
          return getFixedArrayOf( getComplexDesc( desc, data, dataEnd ), length );
        }
        else
        {
          // [pzion 20111014] count brackets to get subtype
          
          size_t bracketCount = 0;
          char const *type = data;
          char const *typeEnd = data;
          for (;;)
          {
            if ( typeEnd == dataEnd )
              throw Exception( "malformed type expression" );
            else if ( *typeEnd == ']' )
            {
              if ( bracketCount == 0 )
                break;
              else --bracketCount;
            }
            else if ( *typeEnd == '[' )
              ++bracketCount;
            ++typeEnd;
          }
          
          RC::ConstHandle<Desc> keyDesc = getDesc( std::string( type, typeEnd - type ) );
          if ( !isComparable( keyDesc->getType() ) )
            throw Exception( "key type must be comparable" );
          RC::ConstHandle<ComparableDesc> keyComparableDesc = RC::ConstHandle<ComparableDesc>::StaticCast( keyDesc );
          
          ++typeEnd;
          return getDictOf( keyComparableDesc, getComplexDesc( desc, typeEnd, dataEnd ) );
        }
      }
      else if ( data != dataEnd && *data == '<' )
      {
        ++data;
        
        if ( data != dataEnd && *data == '>' )
        {
          ++data;
          return getSlicedArrayOf( getComplexDesc( desc, data, dataEnd ) );
        }
        else throw Exception( "malformed type expression" );
      }
      else return desc;
    }

    RC::ConstHandle<BooleanDesc> Manager::getBooleanDesc() const
    {
      return m_booleanDesc;
    }
    
    RC::ConstHandle<ByteDesc> Manager::getByteDesc() const
    {
      return m_byteDesc;
    }
    
    RC::ConstHandle<IntegerDesc> Manager::getIntegerDesc() const
    {
      return m_integerDesc;
    }
    
    RC::ConstHandle<SizeDesc> Manager::getSizeDesc() const
    {
      return m_sizeDesc;
    }
    
    RC::ConstHandle<Desc> Manager::getIndexDesc() const
    {
      return m_indexDesc;
    }
    
    RC::ConstHandle<FloatDesc> Manager::getFloat32Desc() const
    {
      return m_fp32Desc;
    }
    
    RC::ConstHandle<FloatDesc> Manager::getFloat64Desc() const
    {
      return m_fp64Desc;
    }
    
    RC::ConstHandle<StringDesc> Manager::getStringDesc() const
    {
      return m_stringDesc;
    }
    
    RC::ConstHandle<OpaqueDesc> Manager::getDataDesc() const
    {
      return m_dataDesc;
    }
    
    RC::ConstHandle<ConstStringDesc> Manager::getConstStringDesc() const
    {
      return m_constStringDesc;
    }

    RC::ConstHandle<ContainerDesc> Manager::getContainerDesc() const
    {
      return m_containerDesc;
    }

    void Manager::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      JSON::Encoder registeredTypesEncoder = resultObjectEncoder.makeMember( "registeredTypes", 15 );
      jsonDescRegisteredTypes( registeredTypesEncoder );
    }
      
    void Manager::jsonDescRegisteredTypes( JSON::Encoder &resultEncoder ) const
    {
      JSON::ObjectEncoder resultObjectEncoder = resultEncoder.makeObject();
      for ( Types::const_iterator it=m_types.begin(); it!=m_types.end(); ++it )
      {
        if ( it->second->isExportable() )
        {
          JSON::Encoder memberEncoder = resultObjectEncoder.makeMember( it->first );
          it->second->jsonDesc( memberEncoder );
        }
      }
    }

    void Manager::jsonRoute( std::vector<JSON::Entity> const &dst, size_t dstOffset, JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      if ( dst.size() - dstOffset == 0 )
        jsonExec( cmd, arg, resultArrayEncoder );
      else throw Exception( "unroutable" );
    }

    void Manager::jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      if ( cmd.stringIs( "registerType", 12 ) )
        jsonExecRegisterType( arg, resultArrayEncoder );
      else throw Exception( "unknown command" );
    }
    
    void Manager::jsonExecRegisterType( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder )
    {
      std::string name;
      bool haveMembers = false;
      RT::StructMemberInfoVector memberInfos;
      std::map< std::string, size_t > memberNameToIndexMap;
      JSON::Entity defaultValueObject;
      RC::ConstHandle<RC::Object> klBindingsAST;
      
      arg.requireObject();
      JSON::ObjectDecoder argObjectDecoder( arg );
      JSON::Entity keyString, valueEntity;
      while ( argObjectDecoder.getNext( keyString, valueEntity ) )
      {
        try
        {
          if ( keyString.stringIs( "name", 4 ) )
          {
            valueEntity.requireString();
            name = valueEntity.stringToStdString();
          }
          else if ( keyString.stringIs( "members", 7 ) )
          {
            valueEntity.requireArray();
            JSON::ArrayDecoder membersArrayDecoder( valueEntity );
            JSON::Entity memberEntity;
            while ( membersArrayDecoder.getNext( memberEntity ) )
            {
              try
              {
                RT::StructMemberInfo memberInfo;
                
                memberEntity.requireObject();
                JSON::ObjectDecoder memberObjectDecoder( memberEntity );
                JSON::Entity memberKeyString, memberValueEntity;
                while ( memberObjectDecoder.getNext( memberKeyString, memberValueEntity ) )
                {
                  try
                  {
                    if ( memberKeyString.stringIs( "name", 4 ) )
                    {
                      memberValueEntity.requireString();
                      memberInfo.name = memberValueEntity.stringToStdString();
                    }
                    else if ( memberKeyString.stringIs( "type", 4 ) )
                    {
                      memberValueEntity.requireString();
                      std::string typeName = memberValueEntity.stringToStdString();
                      if ( typeName.empty() )
                        throw Exception( "must be non-empty" );
                      memberInfo.desc = getDesc( typeName );
                    }
                  }
                  catch ( Exception e )
                  {
                    memberObjectDecoder.rethrow( e );
                  }
                }
                
                size_t memberIndex = memberInfos.size();
                memberInfos.push_back( memberInfo );
                memberNameToIndexMap.insert( std::map< std::string, size_t >::value_type( memberInfo.name, memberIndex ) );
              }
              catch ( Exception e )
              {
                membersArrayDecoder.rethrow( e );
              }
            }
            
            haveMembers = true;
          }
          else if ( keyString.stringIs( "defaultValue", 12 ) )
          {
            valueEntity.requireObject();
            defaultValueObject = valueEntity;
          }
          else if ( keyString.stringIs( "klBindings", 10 ) )
          {
            std::string filename;
            std::string sourceCode;

            valueEntity.requireObject();
            JSON::ObjectDecoder klBindingsObjectDecoder( valueEntity );
            JSON::Entity klBindingsKeyString, klBindingsValueEntity;
            while ( klBindingsObjectDecoder.getNext( klBindingsKeyString, klBindingsValueEntity ) )
            {
              try
              {
                if ( klBindingsKeyString.stringIs( "filename", 8 ) )
                {
                  klBindingsValueEntity.requireString();
                  filename = klBindingsValueEntity.stringToStdString();
                }
                else if ( klBindingsKeyString.stringIs( "sourceCode", 10 ) )
                {
                  klBindingsValueEntity.requireString();
                  sourceCode = klBindingsValueEntity.stringToStdString();
                }
              }
              catch ( Exception e )
              {
                klBindingsObjectDecoder.rethrow( e );
              }
            }

            klBindingsAST = m_klCompiler->compile( filename, sourceCode );
          }
        }
        catch ( Exception e )
        {
          argObjectDecoder.rethrow( e );
        }
      }
      
      if ( name.empty() )
        throw Exception( "missing 'name'" );
      if ( !haveMembers )
        throw Exception( "missing or empty 'members'" );
      
      try
      {
        JSON::ObjectDecoder defaultValueObjectDecoder( defaultValueObject );
        JSON::Entity defaultValueKeyString, defaultValueValueEntity;
        while ( defaultValueObjectDecoder.getNext( defaultValueKeyString, defaultValueValueEntity ) )
        {
          try
          {
            std::string memberName = defaultValueKeyString.stringToStdString();
            std::map< std::string, size_t >::const_iterator it = memberNameToIndexMap.find( memberName );
            if ( it == memberNameToIndexMap.end() )
            {
              // [pzion 20120124] for historical reasons we silent drop unknown members...
              //throw Exception( "no such member" );
              continue;
            }
            RT::StructMemberInfo &memberInfo = memberInfos[it->second];
              
            memberInfo.defaultData.resize( memberInfo.desc->getAllocSize() );
            memberInfo.desc->decodeJSON( defaultValueValueEntity, &memberInfo.defaultData[0] );
          }
          catch ( Exception e )
          {
            defaultValueObjectDecoder.rethrow( e );
          }
        }
      }
      catch ( Exception e )
      {
        throw "'defaultValue': " + e;
      }
      
      RC::ConstHandle<RT::StructDesc> structDesc = registerStruct( name, memberInfos );

      // [pzion 20110927] Special case: if we are registering a structure, replace
      // the default values.  This exists purely so that if a structure is registered
      // from an extension KL file first you can then register the same structure from
      // Javascript but with default values
      RC::ConstHandle<RT::StructImpl> structImpl = RC::ConstHandle<RT::StructImpl>::StaticCast( structDesc->getImpl() );
      structImpl->setDefaultValues( memberInfos );

      for ( size_t i=0; i<memberInfos.size(); ++i )
      {
        RT::StructMemberInfo &memberInfo = memberInfos[i];
        if ( !memberInfo.defaultData.empty() )
          memberInfo.desc->disposeData( &memberInfo.defaultData[0] );
      }

      if ( klBindingsAST )
        structDesc->setKLBindingsAST( klBindingsAST );
    }
    
    RC::ConstHandle<Desc> Manager::getStrongerTypeOrNone( RC::ConstHandle<Desc> const &lhsDesc, RC::ConstHandle<Desc> const &rhsDesc ) const
    {
      ImplType lhsType = lhsDesc->getType(), rhsType = rhsDesc->getType();
      if ( isNumeric( lhsType ) && isNumeric( rhsType ) )
      {
        RC::ConstHandle<NumericDesc> lhsNumericDesc = RC::ConstHandle<NumericDesc>::StaticCast( lhsDesc );
        RC::ConstHandle<NumericDesc> rhsNumericDesc = RC::ConstHandle<NumericDesc>::StaticCast( rhsDesc );
        if ( lhsNumericDesc->isFloat() )
        {
          if ( rhsNumericDesc->isFloat() && rhsNumericDesc->getAllocSize() > lhsNumericDesc->getAllocSize() )
            return rhsNumericDesc;
          else return lhsNumericDesc;
        }
        else if ( rhsNumericDesc->isFloat() )
          return rhsNumericDesc;
        else
        {
          if ( rhsNumericDesc->getAllocSize() > lhsNumericDesc->getAllocSize()
            || ( rhsNumericDesc->getAllocSize() == lhsNumericDesc->getAllocSize() && RC::ConstHandle<IntegerDesc>::StaticCast(rhsNumericDesc)->isSigned() ) )
            return rhsNumericDesc;
          else return lhsNumericDesc;
        }
      }
      else if( lhsType == rhsType )
      {
        return lhsDesc;
      }
      else if ( isConstString( lhsType ) || isConstString( rhsType ) )
      {
        return m_stringDesc;
      }
      else if ( isString( lhsType ) )
      {
        return lhsDesc;
      }
      else if ( isString( rhsType ) )
      {
        return rhsDesc;
      }
      else return 0;
    }

    bool Manager::maybeGetASTForType( std::string const &typeName, RC::ConstHandle<RC::Object> &ast ) const
    {
      Types::const_iterator it = m_types.find( typeName );
      if ( it != m_types.end() )
      {
        ast = it->second->getKLBindingsAST();
        return true;
      }
      else return false;
    }
  };
};

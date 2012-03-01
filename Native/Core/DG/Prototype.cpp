/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Prototype.h"
#include "Node.h"
#include "Scope.h"
#include "Debug.h"

#include <Fabric/Core/DG/Function.h>
#include <Fabric/Core/AST/Operator.h>
#include <Fabric/Core/AST/ParamVector.h>
#include <Fabric/Core/AST/Param.h>
#include <Fabric/Core/CG/Manager.h>
#include <Fabric/Core/RT/IntegerDesc.h>
#include <Fabric/Core/RT/Manager.h>
#include <Fabric/Core/RT/SlicedArrayImpl.h>
#include <Fabric/Core/RT/SlicedArrayDesc.h>
#include <Fabric/Core/RT/ContainerDesc.h>
#include <Fabric/Core/RT/ContainerImpl.h>
#include <Fabric/Core/MT/Util.h>
#include <Fabric/Base/JSON/Encoder.h>

namespace Fabric
{
  namespace DG
  {
    class Prototype::Param
    {
    public:
      
      virtual ~Param()
      {
      }
      
      unsigned index() const
      {
        return m_index;
      }
      
      virtual bool isContainerParam() const { return false; }
      virtual bool isSizeParam() const { return false; }
      virtual bool isIndexParam() const { return false; }
      virtual bool isMemberParam() const { return false; }
      virtual bool isElementParam() const { return false; }
      virtual bool isArrayParam() const { return false; }
    
      virtual std::string desc() const = 0;
      
    protected:
    
      Param( unsigned index )
        : m_index( index )
      {
      }
      
    private:
    
      unsigned m_index;
    };

    class Prototype::ContainerParam : public Prototype::Param
    {
    public:
    
      ContainerParam( unsigned index )
        : Param( index )
      {
      }

      virtual bool isContainerParam() const { return true; }

      virtual std::string desc() const
      {
        return "";//??
      }
    };
    
    class Prototype::IndexParam : public Prototype::Param
    {
    public:
    
      IndexParam( unsigned index )
        : Param( index )
      {
      }
    
      virtual bool isIndexParam() const { return true; }

      virtual std::string desc() const
      {
        return "index";
      }
    };
    
    class Prototype::MemberParam : public Prototype::Param
    {
    public:
    
      MemberParam( unsigned index, std::string const &name )
        : Param( index )
        , m_name( name )
      {
      }
      
      std::string const &name() const
      {
        return m_name;
      }

      virtual bool isMemberParam() const { return true; }
      
    private:
    
      std::string m_name;
    };
    
    class Prototype::ElementParam : public Prototype::MemberParam
    {
    public:
    
      ElementParam( unsigned index, std::string const &name )
        : MemberParam( index, name )
      {
      }
    
      virtual bool isElementParam() const { return true; }

      virtual std::string desc() const
      {
        return name();
      }
    };
    
    class Prototype::ArrayParam : public Prototype::MemberParam
    {
    public:
    
      ArrayParam( unsigned index, std::string const &name )
        : MemberParam( index, name )
      {
      }
    
      virtual bool isArrayParam() const { return true; }

      virtual std::string desc() const
      {
        return name() + "<>";
      }
    };
    
    Prototype::Prototype( RC::Handle<CG::Manager> const &cgManager )
      : m_cgManager( cgManager )
      , m_rtSizeDesc( cgManager->getRTManager()->getSizeDesc() )
      , m_rtSizeImpl( m_rtSizeDesc->getImpl() )
      , m_rtIndexDesc( cgManager->getRTManager()->getIndexDesc() )
      , m_rtIndexImpl( m_rtIndexDesc->getImpl() )
      , m_rtContainerDesc( cgManager->getRTManager()->getContainerDesc() )
      , m_rtContainerImpl( m_rtContainerDesc->getImpl() )
    {
    }
    
    void Prototype::setDescs( std::vector<std::string> const &descs )
    {
      clear();
      
      m_paramCount = descs.size();
      
      for ( size_t i=0; i<m_paramCount; ++i )
      {
        std::string const &desc = descs[i];
          
        try
        {
          std::string::size_type nodeNameStart = 0;
          std::string::size_type nodeNameEnd = desc.find( '.' );
          if ( nodeNameEnd == std::string::npos )
            nodeNameEnd = desc.size();

          if( nodeNameEnd - nodeNameStart < 1 )
            throw Exception( "missing node name" );

          std::string nodeName = desc.substr( nodeNameStart, nodeNameEnd - nodeNameStart );

          Param *param;
          std::string memberName;

          if( nodeNameEnd == desc.size() )
              param = new ContainerParam(i);
          else
          {
            std::string::size_type memberNameStart = nodeNameEnd + 1;
            std::string::size_type memberNameEnd = desc.find( '<', nodeNameEnd );
            if ( memberNameEnd == std::string::npos )
              memberNameEnd = desc.size();
            if ( memberNameEnd - memberNameStart < 1 )
              throw Exception( "missing member name" );
            memberName = desc.substr( memberNameStart, memberNameEnd - memberNameStart );
          
            if ( memberName == "index" )
              param = new IndexParam(i);
            else if ( desc.substr( memberNameEnd ) == "<>" )
              param = new ArrayParam( i, memberName );
            else
              param = new ElementParam( i, memberName );
          }
          m_params[nodeName].insert( std::multimap< std::string, Param * >::value_type( memberName, param ) );
        }
        catch ( Exception e )
        {
          char buf[32];
          sprintf( buf, "%u", (unsigned)i );
          throw "parameter " + std::string(buf) + ": " + e;
        }
      }
    }
    
    void Prototype::clear()
    {
      while ( !m_params.empty() )
      {
        std::map< std::string, std::multimap< std::string, Param * > >::iterator it=m_params.begin();
        for ( std::multimap< std::string, Param * >::const_iterator jt=it->second.begin(); jt!=it->second.end(); ++jt )
          delete jt->second;
        m_params.erase( it );
        --m_paramCount;
      }
    }
    
    Prototype::~Prototype()
    {
      clear();
    }

    RC::Handle<MT::ParallelCall> Prototype::bind(
      std::vector<std::string> &errors,
      RC::ConstHandle<AST::Operator> const &astOperator,
      Scope const &scope,
      RC::ConstHandle<Function> const &function,
      unsigned prefixCount,
      void * const *prefixes
      )
    {
      FABRIC_ASSERT( function );
      
      RC::ConstHandle<AST::ParamVector> astParamList = astOperator->getParams( m_cgManager );
      size_t numASTParams = astParamList->size();
      size_t expectedNumASTParams = prefixCount + m_paramCount;
      if ( numASTParams != expectedNumASTParams )
        errors.push_back( "operator takes incorrect number of parameters (expected "+_(expectedNumASTParams)+", actual "+_(numASTParams)+")" );

      RC::Handle<MT::ParallelCall> result = MT::ParallelCall::Create( function, prefixCount+m_paramCount, astOperator->getDeclaredName() );
      for ( unsigned i=0; i<prefixCount; ++i )
        result->setBaseAddress( i, prefixes[i] );
      for ( std::map< std::string, std::multimap< std::string, Param * > >::const_iterator it=m_params.begin(); it!=m_params.end(); ++it )
      {
        std::string const &nodeName = it->first;
        std::string const nodeErrorPrefix = "node " + _(nodeName) + ": ";
        {
          RC::Handle<Container> container = scope.find( nodeName );
          if ( !container )
          {
            errors.push_back( nodeErrorPrefix + "not found" );
            continue;
          }
          
          bool haveAdjustmentIndex = false;
          unsigned adjustmentIndex = 0;
          
          std::set<void *> elementAccessSet;
          std::set<void *> arrayAccessSet;

          bool haveLValueContainerAccess = false;
          
          for ( std::multimap< std::string, Param * >::const_iterator jt=it->second.begin(); jt!=it->second.end(); ++jt )
          {
            Param const *param = jt->second;
            
            if ( prefixCount + param->index() >= numASTParams )
              continue;

            RC::ConstHandle<AST::Param> astParam = astParamList->get( prefixCount + param->index() );
            CG::ExprType astParamExprType = astParam->getExprType( m_cgManager );
            RC::ConstHandle<RT::Desc> astParamDesc = astParamExprType.getDesc();
            RC::ConstHandle<RT::Impl> astParamImpl = astParamDesc->getImpl();
            
            std::string const parameterErrorPrefix = nodeErrorPrefix + "parameter " + _(size_t(prefixCount+param->index()+1)) + ": ";
            {
              if ( param->isContainerParam() )
              {
                if ( astParamImpl != m_rtContainerImpl )
                  errors.push_back( parameterErrorPrefix + "'container' parmeters must bind to operator in parameters of type "+_(m_rtContainerDesc->getUserName()) );
                if( astParamExprType.getUsage() != CG::USAGE_RVALUE )
                  haveLValueContainerAccess = true;

                result->setBaseAddress( prefixCount+param->index(), container->getRTContainerData() );
              }
              else if ( param->isIndexParam() )
              {
                if ( astParamImpl != m_rtIndexImpl )
                  errors.push_back( parameterErrorPrefix + "'index' parmeters must bind to operator in parameters of type "+_(m_rtIndexDesc->getUserName()) );
                else if( astParamExprType.getUsage() != CG::USAGE_RVALUE )
                  errors.push_back( parameterErrorPrefix + "'index' cannot be an 'io' parameter" );
                else
                {
                  result->setBaseAddress( prefixCount+param->index(), (void *)0 );
                  if ( container->size() != 1 )
                  {
                    if ( !haveAdjustmentIndex )
                    {
                      adjustmentIndex = result->addAdjustment( container->size(), std::max<size_t>( 1, container->size()/(4*MT::getNumCores()) ) );
                      haveAdjustmentIndex = true;
                    }
                    result->setAdjustmentOffset( adjustmentIndex, prefixCount+param->index(), 1 );
                  }
                }
              }
              else if ( param->isMemberParam() )
              {
                MemberParam const *memberParam = static_cast<MemberParam const *>(param);
                std::string const &memberName = memberParam->name();
                std::string const memberErrorPrefix = parameterErrorPrefix + "member '" + memberName + "': ";
                {
                  RC::ConstHandle<RT::SlicedArrayDesc> slicedArrayDesc;
                  void *slicedArrayData;
                  try
                  {
                    container->getMemberArrayDescAndData( memberName, slicedArrayDesc, slicedArrayData );
                  }
                  catch ( Exception e )
                  {
                    errors.push_back( memberErrorPrefix + std::string(e) );
                  }
                  if ( slicedArrayDesc )
                  {
                    RC::ConstHandle<RT::Desc> memberDesc = slicedArrayDesc->getMemberDesc();
                    RC::ConstHandle<RT::SlicedArrayImpl> slicedArrayImpl = slicedArrayDesc->getImpl();
                    if ( param->isElementParam() )
                    {
                      if ( arrayAccessSet.find( slicedArrayData ) != arrayAccessSet.end() )
                        errors.push_back( memberErrorPrefix + "cannot access both per-slice and whole array data for the same member" );
                      else
                      {
                        elementAccessSet.insert( slicedArrayData );
                        
                        RC::ConstHandle<RT::Impl> memberImpl = memberDesc->getImpl();
                        if ( astParamImpl != memberImpl )
                          errors.push_back( memberErrorPrefix + "parameter type mismatch: member element type is "+_(memberDesc->getUserName())+", operator parameter type is "+_(astParamDesc->getUserName()) );
                        if ( astParamExprType.getUsage() != CG::USAGE_LVALUE )
                          errors.push_back( memberErrorPrefix + "element parmeters must bind to operator io parameters" );
                        void *baseAddress;
                        if ( slicedArrayDesc->getNumMembers( slicedArrayData ) > 0 )
                          baseAddress = slicedArrayImpl->getMutableMemberData( slicedArrayData, 0 );
                        else baseAddress = 0;
                        result->setBaseAddress( prefixCount+param->index(), baseAddress );
                        if ( container->size() != 1 )
                        {
                          if ( !haveAdjustmentIndex )
                          {
                            adjustmentIndex = result->addAdjustment( container->size(), std::max<size_t>( 1, container->size()/(4*MT::getNumCores()) ) );
                            haveAdjustmentIndex = true;
                          }
                          result->setAdjustmentOffset( adjustmentIndex, prefixCount+param->index(), memberImpl->getAllocSize() );
                        }
                      }
                    }
                    else
                    {
                      if ( elementAccessSet.find( slicedArrayData ) != elementAccessSet.end() )
                        errors.push_back( memberErrorPrefix + "cannot access both per-slice and whole array data for the same member" );
                      else arrayAccessSet.insert( slicedArrayData );
                      
                      RC::ConstHandle<RT::SlicedArrayImpl> slicedArrayImpl = slicedArrayDesc->getImpl();
                      if ( astParamImpl != slicedArrayImpl )
                        errors.push_back( memberErrorPrefix + "parameter type mismatch: member array type is "+_(slicedArrayDesc->getUserName())+", operator parameter type is "+_(astParamDesc->getUserName()) );
                      //if ( astParamExprType.getUsage() != CG::USAGE_LVALUE )
                      //  throw Exception( "array parmeters must bind to operator io parameters" );
                      
                      result->setBaseAddress( prefixCount+param->index(), slicedArrayData );
                    }
                  }
                }
              }
            }
          }
          if( haveLValueContainerAccess )
          {
            if( !elementAccessSet.empty() )
              errors.push_back( nodeErrorPrefix + "cannot have both per-slice data parameters and an 'io' Container parameter (calling Container::resize() would invalidate the data)" );
          }
        }
      }
      return result;
    }
    
    std::vector<std::string> Prototype::desc() const
    {
      std::vector<std::string> result;
      result.resize( m_paramCount );
      for ( std::map< std::string, std::multimap< std::string, Param * > >::const_iterator it=m_params.begin(); it!=m_params.end(); ++it )
      {
        std::string const &nodeName = it->first;
        for ( std::multimap< std::string, Param * >::const_iterator jt=it->second.begin(); jt!=it->second.end(); ++jt )
        {
          Param const *param = jt->second;
          result[param->index()] = nodeName + "." + param->desc();
        }
      }
      return result;
    }
      
    void Prototype::jsonDesc( JSON::Encoder &resultEncoder ) const
    {
      JSON::ArrayEncoder resultArrayEncoder = resultEncoder.makeArray();
      
      std::vector<std::string> items = desc();
      for ( size_t i=0; i<items.size(); ++i )
      {
        JSON::Encoder elementEncoder = resultArrayEncoder.makeElement();
        elementEncoder.makeString( items[i] );
      }
    }
  }
}

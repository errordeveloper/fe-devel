/*
 *
 *  Created by Peter Zion on 10-10-08.
 *  Copyright 2010 Fabric 3D Inc. All rights reserved.
 *
 */

#ifndef _FABRIC_DG_PROTOTYPE_H
#define _FABRIC_DG_PROTOTYPE_H

#include <Fabric/Core/MT/ParallelCall.h>
#include <Fabric/Base/RC/ConstHandle.h>

#include <map>
#include <set>

namespace Fabric
{
  namespace JSON
  {
    class Encoder;
  };
  
  namespace RT
  {
    class Manager;
    class Desc;
    class Impl;
  };
  
  namespace CG
  {
    class Manager;
  };
  
  namespace AST
  {
    class Operator;
  };
  
  namespace DG
  {
    class Scope;
    class Function;
    
    class Prototype
    {
    public:
    
      Prototype( RC::Handle<CG::Manager> const &cgManager );
      virtual ~Prototype();
      
      void setDescs( std::vector<std::string> const &descs );
      void clear();
    
      RC::Handle<MT::ParallelCall> bind(
        std::vector<std::string> &errors,
        RC::ConstHandle<AST::Operator> const &astOperator,
        Scope const &scope,
        RC::ConstHandle<Function> const &function,
        unsigned prefixCount=0,
        void * const *prefixes=0
        );
      
      std::vector<std::string> desc() const;

      void jsonDesc( JSON::Encoder &resultEncoder ) const;
    
    private:
    
      class Param;
      class ContainerParam;
      class SizeParam;
      class IndexParam;
      class MemberParam;
      class ElementParam;
      class ArrayParam;
    
      size_t m_paramCount;
      std::map< std::string, std::multimap< std::string, Param * > > m_params;
      
      RC::Handle<CG::Manager> m_cgManager;
      RC::ConstHandle<RT::Desc> m_rtSizeDesc;
      RC::ConstHandle<RT::Impl> m_rtSizeImpl;
      RC::ConstHandle<RT::Desc> m_rtIndexDesc;
      RC::ConstHandle<RT::Impl> m_rtIndexImpl;
      RC::ConstHandle<RT::Desc> m_rtContainerDesc;
      RC::ConstHandle<RT::Impl> m_rtContainerImpl;
    };
  };
};

#endif //_FABRIC_DG_PROTOTYPE_H

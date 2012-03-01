/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_MR_INTERFACE_H
#define _FABRIC_MR_INTERFACE_H

#include <Fabric/Core/GC/Object.h>
#include <Fabric/Core/GC/Container.h>
#include <Fabric/Core/RT/Manager.h>

namespace Fabric
{
  namespace JSON
  {
    class ArrayEncoder;
  };
  
  namespace MR
  {
    class Interface
    {
    public:
    
      Interface(
        GC::Container *gcContainer,
        RC::ConstHandle<RT::Manager> const &rtManager
        );
    
      void jsonRoute(
        std::vector<JSON::Entity> const &dst,
        size_t dstOffset,
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
        
      void jsonExec(
        JSON::Entity const &cmd,
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateConstValue(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateValueCache(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateValueGenerator(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateValueMap(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateValueTransform(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateConstArray(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateArrayGenerator(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateArrayMap(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateArrayTransform(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateArrayCache(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
      
      void jsonExecCreateReduce(
        JSON::Entity const &arg,
        JSON::ArrayEncoder &resultArrayEncoder
        );
        
    private:
    
      GC::Container *m_gcContainer;
      RC::ConstHandle<RT::Manager> m_rtManager;
    };
  };
};

#endif //_FABRIC_MR_INTERFACE_H

/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_CONTAINER_H
#define _FABRIC_DG_CONTAINER_H

#include <Fabric/Core/DG/NamedObject.h>
#include <Fabric/Core/DG/Prototype.h>
#include <Fabric/Core/Util/UnorderedMap.h>

#include <map>
#include <string>
#include <stdint.h>

namespace Fabric
{
  namespace Util
  {
    class SimpleString;
  };
  
  namespace JSON
  {
    class Encoder;
    class Decoder;
  };
  
  namespace MT
  {
    class LogCollector;
    class ParallelCall;
  };
  
  namespace RT
  {
    class Manager;
    class Desc;
    class Impl;
    class SlicedArrayDesc;
  };
  
  namespace DG
  {
    class Scope;
    class Context;
    class Binding;
    class SharedSlicedArray;
    
    class Container : public NamedObject
    {
      friend class Prototype;
      
    public:
      REPORT_RC_LEAKS
    
      virtual bool isContainer() const { return true; }
      virtual bool isNode() const { return false; }
      virtual bool isEvent() const { return false; }
      virtual bool isEventHandler() const { return false; }

      struct MemberDesc
      {
        RC::ConstHandle<RT::Desc> desc;
        void const *defaultData;
      };
      typedef Util::UnorderedMap< std::string, MemberDesc > MemberDescs;
      MemberDescs getMemberDescs() const;
      void addMember( std::string const &name, RC::ConstHandle<RT::Desc> const &desc, void const *defaultData );
      void removeMember( std::string const &name );
      void getMemberArrayDescAndData(
        std::string const &name,
        RC::ConstHandle<RT::SlicedArrayDesc> &slicedArrayDesc,
        void *&slicedArrayData
        );

      void *getRTContainerData();
      size_t size() const;
      void resize( size_t size );

      RC::ConstHandle<RT::Desc> getDesc( std::string const &name ) const;
      void const *getConstData( std::string const &name, size_t index ) const;
      void *getMutableData( std::string const &name, size_t index );
      void getData( std::string const &name, size_t index, void *dstData ) const;
      void getDataJSON( std::string const &name, size_t index, JSON::Encoder &resultEncoder ) const;
      void setData( std::string const &name, size_t index, void const *data );

      void encodeJSON( JSON::Encoder &encoder ) const;
      void generateSliceJSON( size_t index, JSON::Encoder &encoder ) const;
      void generateMemberJSON( std::string const &member, JSON::Encoder &encoder ) const;

      void setJSON( JSON::Entity const &entity );
      void setSliceJSON( size_t index, JSON::Entity const &entity );

      virtual void jsonExec( JSON::Entity const &cmd, JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecAddMember( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecRemoveMember( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonResize( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecGetData( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecGetDataJSON( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecGetDataSize( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecGetDataElement( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecSetData( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecGetBulkData( JSON::ArrayEncoder &resultArrayEncoder ) const;
      void jsonExecSetBulkData( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecGetBulkDataJSON( JSON::ArrayEncoder &resultArrayEncoder ) const;
      void jsonExecSetBulkDataJSON( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecGetSlicesBulkData( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const;
      void jsonExecSetSlicesBulkData( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder );
      void jsonExecGetMembersBulkData( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const;
      void jsonExecPutResourceToFile( JSON::Entity const &arg, JSON::ArrayEncoder &resultArrayEncoder ) const;
      void jsonDesc( JSON::Encoder &resultEncoder ) const;
      virtual void jsonDesc( JSON::ObjectEncoder &resultObjectEncoder ) const;
      void jsonDescMembers( JSON::Encoder &resultEncoder ) const;
      void jsonDescSize( JSON::Encoder &resultEncoder ) const;
      virtual void jsonGetMemoryUsage( JSON::Encoder &jg ) const;
      
    protected:
    
      Container( std::string const &name, RC::Handle<Context> const &context );
      ~Container();
      
      virtual void setOutOfDate() = 0;
      
      virtual RC::Handle<MT::ParallelCall> bind(
        std::vector<std::string> &errors,
        RC::ConstHandle<Binding> const &binding,
        Scope const &scope,
        unsigned prefixCount = 0,
        void * const *prefixes = 0
        );
      
      class Member;
      typedef std::map< std::string, RC::Handle<Member> > Members;
      
      RC::ConstHandle<Member> getImmutableMember( std::string const &name ) const;
      RC::Handle<Member> getMutableMember( std::string const &name );

      void jsonGenerateMemberSliceJSON( JSON::Entity const &arg, JSON::Encoder &resultEncoder ) const;

    private:
    
      Context *m_context;

      size_t m_size;
      Members m_members;
      void* m_rtContainerData;
    };
  };
};

#endif //_FABRIC_DG_CONTAINER_H

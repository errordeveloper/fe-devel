/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_DG_FABRICRESOURCE_H
#define _FABRIC_DG_FABRICRESOURCE_H

#include <Fabric/Base/RC/ConstHandle.h>
#include <string>

namespace Fabric
{
  namespace RT
  {
    class Manager;
    class StructDesc;
    class VariableArrayDesc;
  };

  namespace DG
  {

    void RegisterFabricResourceType( RC::Handle<RT::Manager> const &rtManager );
    RC::ConstHandle<RT::StructDesc> getFabricResourceDesc( RC::ConstHandle<RT::Manager> const &rtManager );

    class FabricResourceWrapper
    {
    public:

      FabricResourceWrapper( RC::ConstHandle<RT::Manager> rtManager, void *resourceToAttach = NULL );
      ~FabricResourceWrapper();

      RC::ConstHandle<RT::StructDesc> getDesc() const;
    
      const void* get() const { return m_resource; }
      bool isDataEqualTo( const void *other ) const;
      bool isDataExternalLocationEqualTo( const void *other ) const;

      std::string getExtension() const;
      void setExtension( std::string const &value );

      std::string getURL() const;
      void setURL( void const *value );
      void setURL( std::string const &value );
      bool isURLEqualTo( const void *otherStringData ) const;

      std::string getMIMEType() const;
      void setMIMEType( std::string const &value );

      //DataExternalLocation: contains either "" if stored in .data member or "fabricio://..." if stored in a file
      std::string getDataExternalLocation() const;
      void setDataExternalLocation( std::string const &value );

      void resizeData( size_t size );
      void setData( size_t offset, size_t size, const void* data );
      const void *getDataPtr() const;
      size_t getDataSize() const;
      
    private:

      void setStringMember( size_t index, std::string const &value );
      std::string getStringMember( size_t index ) const;

      void *m_resource;
      bool m_alloc;

      RC::ConstHandle<RT::Manager> m_rtManager;
      RC::ConstHandle<RT::StructDesc> m_desc;
      RC::ConstHandle<RT::VariableArrayDesc> m_dataMemberDesc;
    };
  };
};

#endif //_FABRIC_DG_FABRICRESOURCE_H

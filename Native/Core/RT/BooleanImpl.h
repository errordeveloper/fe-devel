#ifndef _FABRIC_RT_BOOLEAN_IMPL_H
#define _FABRIC_RT_BOOLEAN_IMPL_H

#include <Fabric/Core/RT/SimpleImpl.h>

namespace Fabric
{
  namespace RT
  {
    class BooleanImpl : public SimpleImpl
    {
      friend class Manager;
      
    public:
    
      // Impl
    
      virtual void setData( void const *value, void *data ) const;
      virtual std::string descData( void const *data ) const;
      virtual void const *getDefaultData() const;
      
      virtual RC::Handle<JSON::Value> getJSONValue( void const *data ) const;
      virtual void setDataFromJSONValue( RC::ConstHandle<JSON::Value> const &value, void *data ) const;

      // BooleanImpl

      bool getValue( void const *data ) const
      {
        return *static_cast<bool const *>(data);
      }
      
      void setValue( bool value, void *data ) const
      {
        *static_cast<bool *>(data) = value;
      }
      
    protected:

      BooleanImpl( std::string const &codeName );
    };
  };
};

#endif //_FABRIC_RT_BOOLEAN_IMPL_H

#ifndef _FABRIC_RT_SIMPLE_IMPL_H
#define _FABRIC_RT_SIMPLE_IMPL_H

#include <Fabric/Core/RT/ComparableImpl.h>

namespace Fabric
{
  namespace RT
  {
    class SimpleImpl : public ComparableImpl
    {
    public:
    
      // Impl
    
      virtual void disposeDatasImpl( void *data, size_t count, size_t stride ) const;
      
      virtual bool isEquivalentTo( RC::ConstHandle<Impl> const &impl ) const;
      virtual bool isShallow() const;
    
    protected:
    
      SimpleImpl( std::string const &codeName, ImplType implType, size_t size )
        : ComparableImpl( codeName, implType )
      {
        setSize( size );
      }
    };
  };
};

#endif //_FABRIC_RT_SIMPLE_IMPL_H

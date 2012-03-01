/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_UNORDERED_MAP_H
#define _FABRIC_UTIL_UNORDERED_MAP_H

#include <Fabric/Core/Util/Hash.h>

#if defined(FABRIC_OS_MACOSX) || defined(FABRIC_OS_LINUX)
# include <tr1/unordered_map>
#elif defined(FABRIC_OS_WINDOWS)
# include <unordered_map>
#endif

namespace Fabric
{
  namespace Util
  {
    template<
      class Key,
      class T,
      class Hash = Hash<Key>,
      class Pred = typename std::equal_to<Key>,
      class Alloc = typename std::allocator< std::pair<const Key, T> >
      > class UnorderedMap
    {
      typedef std::tr1::unordered_map< Key, T, Hash, Pred, Alloc > Impl;

    public:

      typedef typename Impl::key_type key_type;
      typedef T data_type;
      typedef typename Impl::value_type value_type;
      typedef typename Impl::const_iterator const_iterator;
      typedef typename Impl::iterator iterator;

      UnorderedMap()
      {
      }
      
      UnorderedMap( UnorderedMap const &that )
        : m_impl( that.m_impl )
      {
      }
      
      UnorderedMap &operator =( UnorderedMap const &that )
      {
        m_impl = that.m_impl;
        return *this;
      }
      
      bool empty() const
      {
        return m_impl.empty();
      }
      
      size_t size() const
      {
        return m_impl.size();
      }
      
      void clear()
      {
        m_impl.clear();
      }
      
      const_iterator begin() const
      {
        return m_impl.begin();
      }
      
      iterator begin()
      {
        return m_impl.begin();
      }
      
      const_iterator end() const
      {
        return m_impl.end();
      }
      
      iterator end() 
      {
        return m_impl.end();
      }
      
      std::pair< iterator, bool > insert( value_type const &x )
      {
        return m_impl.insert( x );
      }
      
      void erase( iterator pos )
      {
        m_impl.erase( pos );
      }
      
      const_iterator find( key_type const &k ) const
      {
        return m_impl.find( k );
      }
      
      iterator find( key_type const &k )
      {
        return m_impl.find( k );
      }
      
      data_type &operator []( key_type const &k )
      {
        return m_impl[k];
      }

    private:

      Impl m_impl;  
    };
  };
};

#endif //_FABRIC_UTIL_UNORDERED_MAP_H

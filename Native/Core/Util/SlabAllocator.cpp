/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/Util/SlabAllocator.h>

#if defined(FABRIC_POSIX)
# include <unistd.h>
# include <sys/mman.h>
//#else //[JCG 20111027] Not used for now so don't output an error in Windows
//# error "Unsupported FABRIC_PLATFORM_...."
//#endif

namespace Fabric
{
  namespace Util
  {
    size_t SlabAllocator::s_pageSize = 0;
    
    SlabAllocator::SlabAllocator( size_t bufferSize )
      : m_mutex( "SlabAllocator:" )
    {
      if ( s_pageSize == 0 )
      {
#if defined(FABRIC_POSIX)
        s_pageSize = getpagesize();
#else
# error "Unsupported FABRIC_PLATFORM_...."
#endif
      }
      
      static const size_t minBuffersPerSlab = 8;
      
      m_bufferSize = (bufferSize + sizeof(size_t) - 1) / sizeof(size_t) * sizeof(size_t);
      if ( m_bufferSize * minBuffersPerSlab + sizeof(Slab) <= s_pageSize )
        m_rawBufferSize = m_bufferSize;
      else
        m_rawBufferSize = m_bufferSize + sizeof(Slab *);
      m_rawSlabSize = (minBuffersPerSlab * m_rawBufferSize + s_pageSize - 1) / s_pageSize * s_pageSize;
      
      m_firstEmptySlab = m_firstUsedSlab = m_firstFullSlab = 0;
    }
    
    SlabAllocator::~SlabAllocator()
    {
      Mutex::Lock lock(m_mutex);
      
      Slab *slab = m_firstEmptySlab;
      while ( slab )
      {
        Slab *nextSlab = slab->next;
        destroySlab_CS( slab );
        slab = nextSlab;
      }
    
      FABRIC_ASSERT( m_firstUsedSlab == 0 );
      FABRIC_ASSERT( m_firstFullSlab == 0 );
    }

    void *SlabAllocator::alloc()
    {
      Mutex::Lock lock(m_mutex);
      
      Slab *slab = m_firstUsedSlab;
      if ( !slab )
        slab = m_firstEmptySlab;
      if ( !slab )
      {
        slab = createSlab_CS();
        m_firstEmptySlab = slab;
      }
      
      Buffer *buffer = slab->firstFreeBuffer;
      if ( slab->usedBufferCount++ == 0 )
      {
        if ( slab->next )
          slab->next->prev = slab->prev;
        
        if ( slab->prev )
          slab->prev->next = slab->next;
        else
        {
          FABRIC_ASSERT( m_firstEmptySlab == slab );
          m_firstEmptySlab = slab->next;
        }
        
        slab->prev = 0;
        slab->next = m_firstUsedSlab;
        if ( m_firstUsedSlab )
          m_firstUsedSlab->prev = slab;
        m_firstUsedSlab = slab;
      }
      slab->firstFreeBuffer = buffer->nextFreeBuffer;
      if ( !slab->firstFreeBuffer )
      {
        if ( slab->next )
          slab->next->prev = slab->prev;
        
        if ( slab->prev )
          slab->prev->next = slab->next;
        else
        {
          FABRIC_ASSERT( m_firstUsedSlab == slab );
          m_firstUsedSlab = slab->next;
        }
        
        slab->prev = 0;
        slab->next = m_firstFullSlab;
        if ( m_firstFullSlab )
          m_firstFullSlab->prev = slab;
        m_firstFullSlab = slab;
      }
      return bufferToRawBuffer( buffer );
    }

    void SlabAllocator::free( void *rawBuffer )
    {
      Buffer *buffer = rawBufferToBuffer( rawBuffer );
      Slab *slab = bufferToSlab( buffer );
      
      Mutex::Lock lock(m_mutex);
      if ( !slab->firstFreeBuffer )
      {
        if ( slab->next )
          slab->next->prev = slab->prev;
        
        if ( slab->prev )
          slab->prev->next = slab->next;
        else
        {
          FABRIC_ASSERT( m_firstFullSlab == slab );
          m_firstFullSlab = slab->next;
        }
        
        slab->prev = 0;
        slab->next = m_firstUsedSlab;
        if ( m_firstUsedSlab )
          m_firstUsedSlab->prev = slab;
        m_firstUsedSlab = slab;
      }
      buffer->nextFreeBuffer = slab->firstFreeBuffer;
      slab->firstFreeBuffer = buffer;
      if ( --slab->usedBufferCount == 0 )
      {
        if ( slab->next )
          slab->next->prev = slab->prev;
        
        if ( slab->prev )
          slab->prev->next = slab->next;
        else
        {
          FABRIC_ASSERT( m_firstUsedSlab == slab );
          m_firstUsedSlab = slab->next;
        }

        // [pzion 20111024] Ensure there is always at least one empty slab
        // ready to go if possible
        if ( m_firstEmptySlab )
        {
          destroySlab_CS( slab );
        }
        else
        {
          slab->prev = 0;
          slab->next = m_firstEmptySlab;
          if ( m_firstEmptySlab )
            m_firstEmptySlab->prev = slab;
          m_firstEmptySlab = slab;
        }
      }
    }

    void *SlabAllocator::AllocPages( size_t size )
    {
#if defined(FABRIC_POSIX)
      return mmap( 0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, 0, 0 );
#else
# error "Unsupported FABRIC_PLATFORM_...."
#endif
    }
    
    void SlabAllocator::FreePages( size_t size, void *pages )
    {
#if defined(FABRIC_POSIX)
      munmap( pages, size );
#else
# error "Unsupported FABRIC_PLATFORM_...."
#endif
    }
  };
};

#endif//defined(FABRIC_POSIX)

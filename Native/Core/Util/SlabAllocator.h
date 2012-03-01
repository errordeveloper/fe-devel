/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_UTIL_SLAB_ALLOCATOR_H
#define _FABRIC_UTIL_SLAB_ALLOCATOR_H

#include <Fabric/Base/Exception.h>
#include <Fabric/Core/Util/Debug.h>
#include <Fabric/Core/Util/Mutex.h>

namespace Fabric
{
  namespace Util
  {
    class SlabAllocator
    {
      struct Buffer
      {
        Buffer *nextFreeBuffer;
      };
      
      struct Slab
      {
        Buffer *firstFreeBuffer;
        size_t usedBufferCount;
        Slab *prev, *next;
      };
      
    public:
    
      SlabAllocator( size_t bufferSize );
      ~SlabAllocator();
      
      void *alloc();
      void free( void *block );
      
    protected:
    
      static void *AllocPages( size_t size );
      static void FreePages( size_t size, void *pages );
    
      Buffer *rawBufferToBuffer( void *rawBuffer ) const
      {
        return static_cast<Buffer *>( rawBuffer );
      }
      
      void *bufferToRawBuffer( Buffer *buffer ) const
      {
        return static_cast<void *>( buffer );
      }
    
      Slab *rawSlabToSlab( void *rawSlab ) const
      {
        return reinterpret_cast<Slab *>(
          reinterpret_cast<uint8_t *>(rawSlab) + m_rawSlabSize - sizeof(Slab)
          );
      }

      void *slabToRawSlab( Slab *slab ) const
      {
        return reinterpret_cast<void *>(
          reinterpret_cast<uint8_t *>(slab) - m_rawSlabSize + sizeof(Slab)
          );
      }

      Slab *bufferToSlab( void *buffer ) const
      {
        if ( m_rawSlabSize == s_pageSize )
          return rawSlabToSlab(
            reinterpret_cast<void *>(
              reinterpret_cast<size_t>(buffer) & ~(m_rawSlabSize-1)
              )
            );
        else return reinterpret_cast<Slab *>(
          reinterpret_cast<uint8_t *>(buffer) + m_rawBufferSize - sizeof(Slab *)
          );
      }

      Slab *createSlab_CS()
      {
        // [pzion 20111024] Release the mutex here just in case it results in a compatification
        m_mutex.release();
        void *rawSlab = AllocPages( m_rawSlabSize );
        
        if ( !rawSlab )
        {
          m_mutex.acquire();
          throw Exception( "out of memory" );
        }
        
        Slab *slab = rawSlabToSlab( rawSlab );

        uint8_t *rawBuffer = static_cast<uint8_t *>( rawSlab );
        uint8_t *rawBufferEnd = rawBuffer + (m_rawSlabSize - sizeof(Slab)) / m_rawBufferSize * m_rawBufferSize;
        Buffer *lastBuffer = 0;
        while ( rawBuffer != rawBufferEnd )
        {
          Buffer *buffer = rawBufferToBuffer( rawBuffer );
          buffer->nextFreeBuffer = lastBuffer;
          if ( m_rawSlabSize > s_pageSize )
            *reinterpret_cast<Slab **>(
              reinterpret_cast<uint8_t *>(buffer) + m_rawBufferSize - sizeof(Slab *)
              ) = slab;
          lastBuffer = buffer;
          rawBuffer += m_rawBufferSize;
        }

        m_mutex.acquire();
        slab->firstFreeBuffer = lastBuffer;
        slab->usedBufferCount = 0;
        slab->prev = 0;
        slab->next = 0;
        return slab;
      }
      
      void destroySlab_CS( Slab *slab )
      {
        FABRIC_ASSERT( slab->usedBufferCount == 0 );
        void *rawSlab = slabToRawSlab( slab );
        FreePages( m_rawSlabSize, rawSlab );
      }

    private:

      static size_t s_pageSize;
      size_t m_bufferSize;
      size_t m_rawBufferSize;
      size_t m_rawSlabSize;
      Mutex m_mutex;
      Slab *m_firstEmptySlab, *m_firstUsedSlab, *m_firstFullSlab;
    };
  };
};

#endif //_FABRIC_UTIL_SLAB_ALLOCATOR_H

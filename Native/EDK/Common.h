/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_EDK_COMMON_H
#define _FABRIC_EDK_COMMON_H

namespace Fabric
{
  namespace EDK
  {
    struct Callbacks
    {
      Callbacks()
        : m_malloc(NULL)
        , m_realloc(NULL)
        , m_free(NULL)
        , m_throwException(NULL)
        , m_fileHandleCreateFromPath(NULL)
        , m_fileGetPath(NULL)
        , m_fileHandleIsValid(NULL)
        , m_fileHandleIsReadOnly(NULL)
        , m_fileHandleIsFolder(NULL)
        , m_fileHandleTargetExists(NULL)
        , m_fileHandleEnsureTargetExists(NULL)
      {}

      void *( *m_malloc )( size_t );
      void *( *m_realloc )( void *, size_t );
      void ( *m_free )( void * );
      void (*m_throwException)( size_t length, char const *data );

      void ( *m_fileHandleCreateFromPath )( void *stringData, char const *filePathCString, bool folder, bool readOnly );
      void ( *m_fileGetPath )( void const *stringData, void *pathStringData );
      bool ( *m_fileHandleIsValid )( void const *stringData );
      bool ( *m_fileHandleIsReadOnly )( void const *stringData );
      bool ( *m_fileHandleIsFolder )( void const *stringData );
      bool ( *m_fileHandleTargetExists )( void const *stringData );
      void ( *m_fileHandleEnsureTargetExists )( void const *stringData );
    };
  };
};

typedef void (*FabricEDKInitPtr)( const Fabric::EDK::Callbacks& callbacks );

#endif // _FABRIC_EDK_COMMON_H

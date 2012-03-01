/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include "Random.h"
#include "Debug.h"

#include <Fabric/Base/Util/Assert.h>

#if defined(FABRIC_POSIX)
# include <sys/stat.h>
# include <fcntl.h>
#elif defined(FABRIC_WIN32)
# include <Wincrypt.h>
#endif

namespace Fabric
{
  namespace Util
  {
    void generateSecureRandomBytes( size_t count, uint8_t *bytes )
    {
#if defined(FABRIC_POSIX)
      int fd = open( "/dev/urandom", O_RDONLY );
      FABRIC_ASSERT( fd != -1 );
      FABRIC_VERIFY( read( fd, bytes, count ) == int(count) );
      close( fd );
#elif defined(FABRIC_WIN32)
      HCRYPTPROV  hCryptProvider = NULL;

      BOOL    success;
      success = ::CryptAcquireContext( &hCryptProvider, NULL, NULL, PROV_RSA_FULL, 0 );
	  if( !success && GetLastError() == NTE_BAD_KEYSET )
      {
        //On the very first run, a container might have to be created.
        success = ::CryptAcquireContext( &hCryptProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET );
      }
      FABRIC_ASSERT( success );

      success = ::CryptGenRandom( hCryptProvider, DWORD( count ), bytes );
      FABRIC_ASSERT( success );

      success = ::CryptReleaseContext( hCryptProvider, 0 );
      FABRIC_ASSERT( success );
#else
# error "Missing implementation of Fabric::Util::generateSecureRandomBytes()"
#endif
    }
  };
};

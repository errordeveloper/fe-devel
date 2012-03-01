/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/Util/MD5.h>
#include <Fabric/Core/Util/Hex.h>

#include <md5/md5.h>

namespace Fabric
{
  namespace Util
  {
    void md5Digest( void const *data, size_t length, uint8_t digest[16] )
    {
      md5_state_t md5State;
      md5_init( &md5State );
      md5_append( &md5State, (md5_byte_t const *)data, length );
      md5_finish( &md5State, digest );
    }
    
    std::string md5HexDigest( void const *data, size_t length )
    {
      uint8_t digest[16];
      md5Digest( data, length, digest );
      return hexBuf( 16, digest );
    }
  };
};

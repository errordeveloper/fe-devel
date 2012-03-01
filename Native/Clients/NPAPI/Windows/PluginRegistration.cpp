/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Base/Config.h>
#include <Fabric/Base/Util/Assert.h>
#include <Fabric/Base/Exception.h>

#include <windows.h>
#include <Shlwapi.h>

#include <vector>

namespace Fabric
{
  namespace NPAPI
  {
    class RegistryKey
    {
    public:
      RegistryKey( const char *key )
      {
        HKEY      masterKey;
        getMasterKey( key, masterKey, m_subKeyPath );

        m_keyPath.push_back( masterKey );

        trimSubKeyPath( );

        // Descend into the subkeys as much as possible.
        descendSubKey( false );
      }

      RegistryKey( const RegistryKey &parent, const char *subkey )
      {
        m_keyPath = parent.m_keyPath;

        if( !parent.m_subKeyPath.empty() )
        {
          m_subKeyPath = parent.m_subKeyPath + "\\";
          m_subKeyPath += subkey;
        }
        else
        {
          m_subKeyPath = subkey;
        }

        trimSubKeyPath( );

        // Duplicate the key handles save for the master key.
        for( size_t i = 1; i < m_keyPath.size(); i++ )
        {
          ::DuplicateHandle( 
            GetCurrentProcess(), m_keyPath[ i ], 
            GetCurrentProcess(), (HANDLE *)&m_keyPath[ i ],
            0, FALSE, DUPLICATE_SAME_ACCESS );
        }

        descendSubKey( false );
      }

      ~RegistryKey( )
      {
        for( size_t i = m_keyPath.size() - 1; i >= 1; i-- )
          ::RegCloseKey( m_keyPath[ i ] );
      }

      bool Exists( )
      {
        descendSubKey( false );
        return( m_subKeyPath.empty() );
      }

      void Create( )
      {
        if( m_subKeyPath.empty() )
          return;

        descendSubKey( true );
      }

      void Delete( )
      {
        if( !m_subKeyPath.empty() )
          throw Exception( "Can't delete key that's not been fully descended." );

        if( m_leafKeyName.empty() )
          throw Exception( "No leaf key. Something's wrong." );

        if( m_keyPath.size() < 2 )
          throw Exception( "Key path is too short." );

        ::SHDeleteKeyA( m_keyPath[ m_keyPath.size() - 2 ], m_leafKeyName.c_str() );
      }

      void SetValue( const char *name, const std::string &value )
      {
        if( !Exists() )
          throw Exception( "Can't set value on non-existing key." );

        if( ::RegSetValueExA( m_keyPath.back(), name, 0, REG_SZ, (const BYTE *)value.c_str(), value.size() + 1 ) != ERROR_SUCCESS )
          throw Exception( "Couldn't set value on key" );
      }

    private:
      void getMasterKey( const char *key, HKEY &rootKey, std::string &subKey )
      {
        if( strchr( key, '\\' ) == NULL )
          throw Exception( "Invalid registry key: " + std::string( key ) );

        if( strncmp( key, "HKLM\\", 5 ) == 0 )
          rootKey = HKEY_LOCAL_MACHINE;
        else if( strncmp( key, "HKCU\\", 5 ) == 0 )
          rootKey = HKEY_CURRENT_USER;
        else if( strncmp( key, "HKCR\\", 5 ) == 0 )
          rootKey = HKEY_CLASSES_ROOT;
        else
          throw Exception( "Unknown master key name: " + std::string( key ) );
        
        subKey = &key[5];
      }

      bool descendSubKey( bool createIfMissing )
      {
        FABRIC_ASSERT( !m_keyPath.empty() );

        // Nothing to do?
        if( m_subKeyPath.empty() )
          return( true );

        std::string   leafKeyName, subKeyPathNew;

        bool last = false;
        size_t    idx = m_subKeyPath.find( '\\' );
        if( idx == -1 )
        {
          leafKeyName = m_subKeyPath;
          last = true;
        }
        else
        {
          leafKeyName = m_subKeyPath.substr( 0, idx );
          subKeyPathNew = m_subKeyPath.substr( idx + 1 );
        }

        HKEY hNewKey;

        //All existing non-leaf keys can be opened with KEY_READ access
        bool missing = ::RegOpenKeyExA( m_keyPath.back(), leafKeyName.c_str(), 0, last ? KEY_ALL_ACCESS : KEY_READ, &hNewKey ) != ERROR_SUCCESS;

        if( missing )
        {
          if( createIfMissing )
          {
            if( ::RegCreateKeyExA( m_keyPath.back(), leafKeyName.c_str(), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hNewKey, NULL ) != ERROR_SUCCESS )
              throw Exception( "Unable to create key" );
          }
          else
          {
              return( false );
          }
        }

        m_keyPath.push_back( hNewKey );
        m_subKeyPath = subKeyPathNew;
        m_leafKeyName = leafKeyName;

        return( descendSubKey( createIfMissing ) );
      }

      void trimSubKeyPath( )
      {
        if( *m_subKeyPath.begin() == '\\' )
          m_subKeyPath = m_subKeyPath.substr( 1 );
        if( *m_subKeyPath.rbegin() == '\\' )
          m_subKeyPath = m_subKeyPath.substr( 0, m_subKeyPath.size() - 1 );
      }

      std::vector<HKEY> m_keyPath;

      std::string       m_subKeyPath;

      std::string       m_leafKeyName;
    };
  }
};

using namespace Fabric::NPAPI;

//Note: HKLM\\ won't work under Windows 7 with default UAC settings
static const char *s_pluginKey = "HKCU\\SOFTWARE\\MozillaPlugins\\@fabric-engine.com/Fabric,version=1.0";
static HINSTANCE  s_hDllInstance = 0;

extern"C" BOOL WINAPI DllMain( HINSTANCE hDllInstance, DWORD dwReason, void *)
{
  if( dwReason == DLL_PROCESS_ATTACH )
    s_hDllInstance = hDllInstance;
  
  return( TRUE );
}


STDAPI DllRegisterServer( )
{
  try
  {
    // Get own path
    char      dllPath[ _MAX_PATH ];
    ::GetModuleFileNameA( s_hDllInstance, dllPath, ARRAYSIZE( dllPath ) );

    RegistryKey   pluginKey( s_pluginKey );

    if( !pluginKey.Exists( ) )
      pluginKey.Create( );

    pluginKey.SetValue( "ProductName", "Fabric" );
    pluginKey.SetValue( "Vendor", "Fabric Technologies, Inc." );
    pluginKey.SetValue( "Version", "1.0" );
    pluginKey.SetValue( "Description", "Fabric Engine - High Performance Computing for the Web" );
    pluginKey.SetValue( "Path", dllPath );

    RegistryKey   mimeKey( pluginKey, "MimeTypes\\applications/fabric" );


    if( !mimeKey.Exists() )
      mimeKey.Create( );
  }
  catch( Fabric::Exception e )
  {
    fprintf( stderr, "Registration failed. Reason: %s\n", (const char*)e.getDesc() );
    return( E_UNEXPECTED );
  }

  return( S_OK );
}


STDAPI DllUnregisterServer( )
{
  try
  {
    RegistryKey   pluginKey( s_pluginKey );

    if( pluginKey.Exists( ) )
      pluginKey.Delete( );
  }
  catch( Fabric::Exception e )
  {
    fprintf( stderr, "Unregistration failed. Reason: %s\n", (const char*)e.getDesc() );
    return( E_UNEXPECTED );
  }

  return( S_OK );
}

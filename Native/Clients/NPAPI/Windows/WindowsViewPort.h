/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#ifndef _FABRIC_PLUGIN_WINDOWS_VIEW_PORT_H
#define _FABRIC_PLUGIN_WINDOWS_VIEW_PORT_H

#include <Fabric/Clients/NPAPI/ViewPort.h>

#include <windows.h>

namespace Fabric
{
  namespace NPAPI
  {
    class WindowsViewPort : public ViewPort
    {
    public:
      REPORT_RC_LEAKS
      
      static RC::Handle<ViewPort> Create( RC::ConstHandle<Interface> const &interface );

      virtual void getWindowSize( size_t &width, size_t &height ) const
      {
        width = m_windowWidth;
        height = m_windowHeight;
      }
      
      virtual void needsRedraw();
      
      virtual NPError nppSetWindow( NPWindow *window );
      virtual int16_t nppHandleEvent( void *event );

      virtual void pushOGLContext();
      virtual void popOGLContext();

      virtual std::string queryUserFilePath( bool existingFile, std::string const &title, std::string const &defaultFilename, std::string const &extension );
    
      virtual NPError nppDestroy( NPSavedData** save );

    protected:
  
      WindowsViewPort( RC::ConstHandle<Interface> const &interface );
      ~WindowsViewPort();
  
    private:  
    
      void fireRedrawEvent( );

      void initOGLContext( );
      void termOGLContext( );

      HWND getParentWindow( );

      LRESULT processMessage( UINT message, WPARAM wParam, LPARAM lParam );

      static LRESULT CALLBACK windowsMsgCB( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

      
      NPP       m_npp;

      bool      m_bWindowLess;

      RC::Handle<MT::LogCollector> m_logCollector;
      int m_windowLeft, m_windowTop, m_windowWidth, m_windowHeight;

      HWND      m_hWnd;
      HWND      m_hParentWnd;
      HDC       m_hDC;
      WNDPROC   m_oldWndProc;

      HGLRC     m_hGLRC;

      typedef std::pair< HDC, HGLRC > WGLDCAndContext;
      std::vector<WGLDCAndContext> m_wglStack;
    };
  }
}

#endif //_FABRIC_PLUGIN_NP_NA_CL_VIEW_PORT_H

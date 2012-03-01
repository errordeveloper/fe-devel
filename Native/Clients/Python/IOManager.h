/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/Core/IO/Helpers.h>
#include <Fabric/Core/IO/Manager.h>
#include <Fabric/Core/IO/ResourceManager.h>
#include <Fabric/Core/IO/FileHandleManager.h>
#include <Fabric/Core/Plug/Manager.h>
#include <Fabric/Core/CG/Manager.h>

#include <fstream>
#include <string>

namespace Fabric
{
  namespace Python
  {
    class TestSynchronousFileResourceProvider : public IO::ResourceProvider
    {
    public:
      REPORT_RC_LEAKS

      static RC::Handle<TestSynchronousFileResourceProvider> Create();
      virtual char const * getUrlScheme() const;
      virtual void get( char const *url, bool getAsFile, void* userData );

    private:
      TestSynchronousFileResourceProvider();
    };

    class IOManager : public IO::Manager
    {
    public:
      REPORT_RC_LEAKS
    
      static RC::Handle<IOManager> Create(
        IO::ScheduleAsyncCallbackFunc scheduleFunc,
        void *scheduleFuncUserData
        );

      virtual std::string queryUserFilePath(
        bool existingFile,
        std::string const &title,
        std::string const &defaultFilename,
        std::string const &extension
        ) const;
  
    protected:
    
      IOManager(
        IO::ScheduleAsyncCallbackFunc scheduleFunc,
        void *scheduleFuncUserData
        );
    };
  };
};

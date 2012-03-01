/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

#include <Fabric/SDK/SDK.h>
#include <Client.h>

using namespace Fabric;
using Fabric::SDK::Exception;

SDK::KLVariableArray<SDK::KLScalar> gPositions;
ViconDataStreamSDK::CPP::Client gClient;

#define maxTries 3

FABRIC_PLUGIN_EXPORT void FabricVICONInit(
  SDK::KLString &hostname,
  SDK::KLSize &nbPositions,
  SDK::KLScalar &time,
  SDK::KLInteger &tryCount
  )
{
  bool clientJustConnected = false;
  while( !gClient.IsConnected().Connected && tryCount < maxTries)
  {
    printf("====== VICON: Connecting to server host '%s', try %d...\n",hostname.data(),(int)(tryCount+1));
    // Multicast connection
    gClient.Connect( hostname.data() );
    clientJustConnected = gClient.IsConnected().Connected;
    tryCount++;
  }
  
  // in case we have no connection
  if(tryCount == maxTries)
  {
    nbPositions = 20;
    gPositions.resize(nbPositions * 3);
    unsigned int index = 0;
    for(unsigned int i=0;i<nbPositions;i++)
    {
      gPositions[index++] = sin(time * 0.001f + (float)i) * float(i+1);
      gPositions[index++] = (float)i;
      gPositions[index++] = 0.0f;
    }
    return;
  }

  if(clientJustConnected || nbPositions < 2)
  {
    // enable the data we want to read
    gClient.EnableMarkerData();
    gClient.EnableUnlabeledMarkerData();
  
    // set the stream mode
    gClient.SetStreamMode( ViconDataStreamSDK::CPP::StreamMode::ClientPull );
    //gClient.SetStreamMode( ViconDataStreamSDK::CPP::StreamMode::ServerPush );
  
    // Set the global up axis
    gClient.SetAxisMapping(ViconDataStreamSDK::CPP::Direction::Forward, 
                           ViconDataStreamSDK::CPP::Direction::Right, 
                           ViconDataStreamSDK::CPP::Direction::Up ); // Y-up

    int frameResult = gClient.GetFrame().Result;
    while( frameResult != ViconDataStreamSDK::CPP::Result::Success && frameResult != ViconDataStreamSDK::CPP::Result::NoFrame )
    {
      printf("====== VICON: Receiving first frame...%d\n",(int)frameResult);
      frameResult = gClient.GetFrame().Result;
    }

    // get the total number of markers
    nbPositions = 0;
    unsigned int subjectCount = gClient.GetSubjectCount().SubjectCount;
    unsigned int index = 0;
    for( unsigned int subjectIndex = 0 ; subjectIndex < subjectCount; ++subjectIndex )
    {
      std::string subjectName = gClient.GetSubjectName( subjectIndex ).SubjectName;
      unsigned int markerCount = gClient.GetMarkerCount( subjectName ).MarkerCount;
      nbPositions += markerCount;
      for( unsigned int markerIndex = 0 ; markerIndex < markerCount ; ++markerIndex )
      {
        std::string markerName = gClient.GetMarkerName( subjectName, markerIndex ).MarkerName;
        printf("====== VICON: Subject '%s' has marker %d '%s'.\n",subjectName.c_str(),index++,markerName.c_str());
      }
    }
    printf("====== VICON: Labeled markers: %d\n",(int)nbPositions);
    
    unsigned int unlabeledMarkerCount = gClient.GetUnlabeledMarkerCount().MarkerCount;
    printf("====== VICON: Unlabeled markers: %d\n",(int)unlabeledMarkerCount);
    nbPositions += unlabeledMarkerCount;
    
    printf("====== VICON: Total number of markers %d\n",(int)nbPositions);

    // sanity check, if no markers, use just one!
    if(nbPositions == 0)
      nbPositions = 1;
      
    gPositions.resize(nbPositions * 3);
  }
  else
  {
    int frameResult = gClient.GetFrame().Result;
    while( frameResult != ViconDataStreamSDK::CPP::Result::Success && frameResult != ViconDataStreamSDK::CPP::Result::NoFrame )
    {
      frameResult = gClient.GetFrame().Result;
    }
  }
  
  unsigned int index = 0;
  unsigned int subjectCount = gClient.GetSubjectCount().SubjectCount;
  for( unsigned int subjectIndex = 0 ; subjectIndex < subjectCount; ++subjectIndex )
  {
    std::string subjectName = gClient.GetSubjectName( subjectIndex ).SubjectName;
    unsigned int markerCount = gClient.GetMarkerCount( subjectName ).MarkerCount;
    for( unsigned int markerIndex = 0 ; markerIndex < markerCount ; ++markerIndex )
    {
      std::string markerName = gClient.GetMarkerName( subjectName, markerIndex ).MarkerName;
      ViconDataStreamSDK::CPP::Output_GetMarkerGlobalTranslation trans = gClient.GetMarkerGlobalTranslation( subjectName, markerName );
      gPositions[index++] = (float)trans.Translation[0] * 0.01;
      gPositions[index++] = (float)trans.Translation[1] * 0.01;
      gPositions[index++] = (float)trans.Translation[2] * 0.01;
    }
  }

  unsigned int unlabeledMarkerCount = gClient.GetUnlabeledMarkerCount().MarkerCount;
  for( unsigned int unlabeledMarkerIndex = 0 ; unlabeledMarkerIndex < unlabeledMarkerCount ; ++unlabeledMarkerIndex )
  { 
    ViconDataStreamSDK::CPP::Output_GetUnlabeledMarkerGlobalTranslation trans = gClient.GetUnlabeledMarkerGlobalTranslation( unlabeledMarkerIndex );
    gPositions[index++] = (float)trans.Translation[0] * 0.01;
    gPositions[index++] = (float)trans.Translation[1] * 0.01;
    gPositions[index++] = (float)trans.Translation[2] * 0.01;
  }
}

FABRIC_PLUGIN_EXPORT void FabricVICONReceive(
  SDK::KLVariableArray<SDK::KLScalar> &positions
  )
{
  positions.resize(gPositions.size());
  for(SDK::KLSize i=0;i<positions.size();i++)
  {
    positions[i] = gPositions[i];
  }
}

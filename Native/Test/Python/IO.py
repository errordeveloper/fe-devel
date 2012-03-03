#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

try:
  fabricClient.IO.putTextFileContent("dummy", "badHandle")
except Exception as e:
  print('Bad handle error: ' + str(e))

readOnlyFileAndFolderHandles = fabricClient.IO.queryUserFileAndFolderHandle(fabricClient.IO.forOpen, "dummy title", "txt", "testfile")

try: 
  fabricClient.IO.putTextFileContent(readOnlyFileAndFolderHandles['file'], "dummy")
except Exception as e:
  print('No write access error: ' + str(e))


fileAndFolderHandles = fabricClient.IO.queryUserFileAndFolderHandle(fabricClient.IO.forOpenWithWriteAccess, "dummy title", "txt", "testfile")
print( 'File and folder handles: ' + fabric.stringify(fileAndFolderHandles));
print('User file name: ' + fabricClient.IO.getFileHandleInfo(fileAndFolderHandles['file'])['fileName'])

relativeFileHandle = fabricClient.IO.buildFileHandleFromRelativePath(fileAndFolderHandles['folder'] + '/SubDir/testfile2.txt')

try:
  dummy = fabricClient.IO.getTextFileContent(relativeFileHandle)
except Exception as e:
  print('File not found error')#'e' has random handle name; don't display it

fabricClient.IO.putTextFileContent(relativeFileHandle,"FileBody")
body = fabricClient.IO.getTextFileContent(relativeFileHandle)
print('File content: ' + body)

fabricClient.IO.putTextFileContent(relativeFileHandle," with appended text",True)
body = fabricClient.IO.getTextFileContent(relativeFileHandle)
print('File content with appended text: ' + body)

fileInfo = fabricClient.IO.getFileHandleInfo(relativeFileHandle)
print('File info: ' + fabric.stringify(fileInfo))

node = fabricClient.DependencyGraph.createResourceLoadNode("node")

myvars = {}
myvars['callbackStep'] = 0

def loadFailureCallback( selfNode ):
  if myvars['callbackStep'] == 0:
    print( 'bad url load failure' )
    node.setData('url',0,relativeFileHandle)
    myvars['callbackStep'] = 1
    node.evaluate()
  else:
    print('failure callback: Unexpected')
    fabricClient.close()

node.addOnLoadFailureCallback( loadFailureCallback )

def loadSuccessCallback( selfNode ):
  print( 'url file load succeeded' )
  if myvars['callbackStep'] == 1:
    resourceResultFileHandle = fabricClient.IO.buildFileHandleFromRelativePath(fileAndFolderHandles['folder'] + '/SubDir/testfile3.txt')
    node.putResourceToFile(resourceResultFileHandle,'resource')
    body = fabricClient.IO.getTextFileContent(resourceResultFileHandle)
    print('Resource data saved to file: ' + body)

    sourceFileHandle = fabricClient.IO.buildFileHandleFromRelativePath(fileAndFolderHandles['folder'] + '/SubDir/testfile4.txt')
    fabricClient.IO.putTextFileContent(sourceFileHandle,"Resource source data from direct file")
    fabricClient.flush()

    #Load from explicit file location, to a file handle
    myvars['callbackStep'] = 2
    node.setData('storeDataAsFile',0,True)
    fabricClient.flush()
    node.setData('url',0,'TMP/SubDir/testfile4.txt')
    fabricClient.flush()
    node.evaluate()
  elif myvars['callbackStep'] == 2:
    resourceResult = node.getData('resource',0)
    resourceFileContent = fabricClient.IO.getTextFileContent(resourceResult['dataExternalLocation'])
    print('Resource file content: ' + resourceFileContent)

    fabricClient.close()
  else:
    print('success callback: Unexpected')
    fabricClient.close()

node.addOnLoadSuccessCallback( loadSuccessCallback )

node.setData('url',0,'badURL')
node.evaluate()


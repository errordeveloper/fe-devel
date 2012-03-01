/*
 *  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
 */

FABRIC = require('Fabric').createClient();

try
{
  FABRIC.IO.putTextFileContent("dummy", "badHandle");
}
catch(e)
{
  console.log('Bad handle error: ' + e);
}

var readOnlyFileAndFolderHandles = FABRIC.IO.queryUserFileAndFolderHandle(FABRIC.IO.forOpen, "dummy title", "txt", "testfile");

try {
  FABRIC.IO.putTextFileContent(readOnlyFileAndFolderHandles.file, "dummy");
}
catch (e) {
  console.log('No write access error: ' + e);
}

var fileAndFolderHandles = FABRIC.IO.queryUserFileAndFolderHandle(FABRIC.IO.forOpenWithWriteAccess, "dummy title", "txt", "testfile");
console.log( 'File and folder handles: ' + JSON.stringify(fileAndFolderHandles));
console.log('User file name: ' + FABRIC.IO.getFileHandleInfo(fileAndFolderHandles.file).fileName);

var relativeFileHandle = FABRIC.IO.buildFileHandleFromRelativePath(fileAndFolderHandles.folder + '/SubDir/testfile2.txt');

try {
  var dummy = FABRIC.IO.getTextFileContent(relativeFileHandle);
}
catch (e) {
  console.log('File not found error');//'e' has random handle name; don't display it
  }

FABRIC.IO.putTextFileContent(relativeFileHandle,"FileBody");
var body = FABRIC.IO.getTextFileContent(relativeFileHandle);
console.log('File content: ' + body);

FABRIC.IO.putTextFileContent(relativeFileHandle," with appended text",true);
body = FABRIC.IO.getTextFileContent(relativeFileHandle);
console.log('File content with appended text: ' + body);

var fileInfo = FABRIC.IO.getFileHandleInfo(relativeFileHandle);
console.log('File info: ' + JSON.stringify(fileInfo));

node = FABRIC.DependencyGraph.createResourceLoadNode("node");

var callbackStep = 0;

node.addOnLoadFailureCallback( function() {
  if( callbackStep == 0 ){
    console.log( 'bad url load failure' );
    node.setData('url',0,relativeFileHandle);
    callbackStep = 1;
    node.evaluate();
  }
  else
  {
    console.log('Unexpected');
    FABRIC.close();
  }
});

node.addOnLoadSuccessCallback( function() {
  console.log( 'url file load succeeded' );
  if( callbackStep == 1 ){
    var resourceResultFileHandle = FABRIC.IO.buildFileHandleFromRelativePath(fileAndFolderHandles.folder + '/SubDir/testfile3.txt');
    node.putResourceToFile(resourceResultFileHandle,'resource');
    var body = FABRIC.IO.getTextFileContent(resourceResultFileHandle);
    console.log('Resource data saved to file: ' + body);

    var sourceFileHandle = FABRIC.IO.buildFileHandleFromRelativePath(fileAndFolderHandles.folder + '/SubDir/testfile4.txt');
    FABRIC.IO.putTextFileContent(sourceFileHandle,"Resource source data from direct file");

    //Load from explicit file location, to a file handle
    callbackStep = 2;
    node.setData('storeDataAsFile',0,true);
    node.setData('url',0,'TMP/SubDir/testfile4.txt');
    node.evaluate();
  }
  else if( callbackStep == 2 ){
    var resourceResult = node.getData('resource',0);
    var resourceFileContent = FABRIC.IO.getTextFileContent(resourceResult.dataExternalLocation);
    console.log('Resource file content: ' + resourceFileContent);

    FABRIC.close();
  }
  else
  {
    console.log('Unexpected');
    FABRIC.close();
  }
});

node.setData('url',0,'badURL');
node.evaluate();

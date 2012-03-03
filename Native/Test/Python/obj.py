#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

loadOp = fabricClient.DependencyGraph.createOperator("load")
loadOp.setEntryFunctionName("load")
loadOp.setSourceCode('\
require FabricOBJ;\n\
\n\
operator load(\n\
  io String url,\n\
  io FabricResource resource,\n\
  io OBJDataHandle objParseHandle\n\
  )\n\
{\n\
  report "Loaded " + url + " (mime type " + resource.mimeType + ")";\n\
  report "OBJ data size is " + resource.data.dataSize();\n\
  FabricOBJDecode(resource.data.data(), resource.data.dataSize(), false, false, false, objParseHandle);\n\
}\n\
')

if len( loadOp.getDiagnostics() ) > 0:
  print('load: ' + fabric.stringify(loadOp.getDiagnostics()))

loadOpBinding = fabricClient.DG.createBinding()
loadOpBinding.setOperator(loadOp)
loadOpBinding.setParameterLayout([
  "self.url",
  "self.resource",
  "self.objParseHandle"
])

rlnode = fabricClient.DependencyGraph.createResourceLoadNode("rlnode")
rlnode.addMember("objParseHandle", "OBJDataHandle")
rlnode.bindings.append(loadOpBinding)
rlnode.setData("url", 0, "testfile://test.obj")

resizeOp = fabricClient.DependencyGraph.createOperator("resize")
resizeOp.setEntryFunctionName("resize")
resizeOp.setSourceCode('\
require FabricOBJ;\n\
operator resize(\n\
  io OBJDataHandle objParseHandle,\n\
  io Container container\n\
  )\n\
{\n\
  Size newSize;\n\
  FabricOBJGetNbEntityPoints(objParseHandle, -1, newSize);\n\
  container.resize(newSize);\n\
  report "rlnode: resized to " + newSize + " points";\n\
}\n\
')

if len( resizeOp.getDiagnostics() ) > 0:
  print('resize: ' + fabric.stringify(resizeOp.getDiagnostics()))

resizeBinding = fabricClient.DG.createBinding()
resizeBinding.setOperator( resizeOp )
resizeBinding.setParameterLayout([
  "rlnode.objParseHandle",
  "self"
])

setDataOp = fabricClient.DependencyGraph.createOperator("setData")
setDataOp.setEntryFunctionName("setData")
setDataOp.setSourceCode('\
require FabricOBJ;\n\
operator setData(\n\
  io OBJDataHandle objParseHandle,\n\
  io Vec3 positions<>\n\
  )\n\
{\n\
  FabricOBJGetEntityPointsSliced(objParseHandle, -1, positions);\n\
  report "rlnode: setData to " + positions.size + " points";\n\
  FabricOBJFreeParsedData(objParseHandle);\n\
}\n\
')

if len( setDataOp.getDiagnostics() ) > 0:
  print('setData: ' + fabric.stringify(setDataOp.getDiagnostics()))

setDataBinding = fabricClient.DG.createBinding()
setDataBinding.setOperator( setDataOp )
setDataBinding.setParameterLayout([
  "rlnode.objParseHandle",
  "self.position<>"
])

node = fabricClient.DG.createNode("dataNode")
node.addMember("position", "Vec3")
node.bindings.append(resizeBinding)
node.bindings.append(setDataBinding)
node.setDependency(rlnode, "rlnode")
if len( node.getErrors() ) > 0:
  print('node: ' + fabric.stringify((node.getErrors())))
node.evaluate()

fabricClient.close()


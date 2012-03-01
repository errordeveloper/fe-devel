#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

op = fabricClient.DependencyGraph.createOperator("op")
op.setEntryFunctionName("load")
op.setSourceCode('\
require FabricPNG;\n\
\n\
struct RGBA\n\
{\n\
  Byte r;\n\
  Byte g;\n\
  Byte b;\n\
  Byte a;\n\
};\n\
\n\
operator load( io String url, io FabricResource resource )\n\
{\n\
  report "Loaded " + url + " (mime type " + resource.mimeType + ")";\n\
  report "PNG data size is " + resource.data.dataSize();\n\
  Size imageWidth, imageHeight;\n\
  RGBA imagePixels[];\n\
  FabricPNGDecode( resource.data.data(), resource.data.dataSize(), imageWidth, imageHeight, imagePixels );\n\
  report "Image dimentions are "+imageWidth+" by "+imageHeight;\n\
  report "Image pixels size is "+imagePixels.size;\n\
}\n\
')

if len( op.getDiagnostics() ) > 0:
  print(op.getDiagnostics())

binding = fabricClient.DG.createBinding()
binding.setOperator(op)
binding.setParameterLayout([
  "loadnode.url",
  "loadnode.resource"
])

rlnode = fabricClient.DependencyGraph.createResourceLoadNode("rlnode")
rlnode.setData("url", 0, "testfile://tomatoes_960_640.png")

node = fabricClient.DependencyGraph.createNode("node")
node.setDependency(rlnode, "loadnode")
node.bindings.append(binding)

if len( node.getErrors() ) > 0:
  print(node.getErrors())
node.evaluate()

fabricClient.close()

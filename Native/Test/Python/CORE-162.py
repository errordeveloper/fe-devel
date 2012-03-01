#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

class ShaderSource():
  def __init__( self, code = None, type = None ):
    self.code = '' if code is None else code
    self.type = 0 if type is None else type
    
fabricClient.RegisteredTypesManager.registerType( 
	'ShaderSource', 
	{
		'members': [ 
			{ 'code':'String' }, { 'type':'Integer' }
		],
		'constructor': ShaderSource
	} 
)

klCode = ""
klCode += "operator bar( io ShaderSource shaders[] ) {\n"
klCode += "\tfor( Size i = 0; i < shaders.size(); i++ ) {\n"
klCode += "\t\treport( \"Type: \" + shaders[i].type );\n"
klCode += "\t\treport( \"Code: \" + shaders[i].code );\n"
klCode += "\t}\n"
klCode += "}\n"

print( klCode )

sources = []
sources.append( ShaderSource( "Zero", 0 ) )
sources.append( ShaderSource( "One", 1 ) )

node = fabricClient.DependencyGraph.createNode("foo")
node.addMember( 'shaderSources', 'ShaderSource[]' )
node.setData( 'shaderSources', sources )

op = fabricClient.DependencyGraph.createOperator("bar")
op.setEntryFunctionName("bar")
op.setSourceCode(klCode)

binding = fabricClient.DependencyGraph.createBinding()
binding.setOperator( op )
binding.setParameterLayout( ['self.shaderSources' ] )

node.bindings.append( binding )
errors = node.getErrors()
if len( errors ) > 0:
  for i in errors:
    print( errors[i] )
else:
  node.evaluate()

fabricClient.close()

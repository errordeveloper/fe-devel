#
#  Copyright 2010-2012 Fabric Engine Inc. All rights reserved.
#

import fabric
fabricClient = fabric.createClient()

def printErrors( obj ):
  if len( obj.getErrors() ) > 0:
    print obj.getName() + ': errors: ' + fabric.stringify( obj.getErrors() )

parentOp = fabricClient.DependencyGraph.createOperator( "parentOp" )
parentOp.setEntryPoint('entry')
parentOp.setSourceCode("operator entry( io Scalar input, io Scalar output ) { output = 2 * input; }")

parentBinding = fabricClient.DependencyGraph.createBinding()
parentBinding.setOperator( parentOp )
parentBinding.setParameterLayout( [ "self.input", "self.output" ] )

parentNode = fabricClient.DependencyGraph.createNode( "parent" )
parentNode.addMember( "input", "Scalar" )
parentNode.addMember( "output", "Scalar" )
parentNode.resize( 2 )
parentNode.setData( 'input', 0, 3 )
parentNode.setData( 'input', 1, 7 )
parentNode.bindings.append( parentBinding )

childOp = fabricClient.DependencyGraph.createOperator( "childOp" )
childOp.setEntryPoint('entry')
childOp.setSourceCode("operator entry( io Scalar input<>, Size index, io Scalar output ) { output = 2 * input[index]; }")

printErrors( childOp )

childBinding = fabricClient.DependencyGraph.createBinding()
childBinding.setOperator( childOp )
childBinding.setParameterLayout( [ "parent.output<>", "self.index", "self.output" ] )

childNode = fabricClient.DependencyGraph.createNode( "child" )
childNode.setDependency( parentNode, "parent" )
childNode.addMember( "output", "Scalar" )
childNode.resize( 2 )
childNode.bindings.append( childBinding )

printErrors( childNode )

parentEHPreOp = fabricClient.DependencyGraph.createOperator( "parentEHPreOp" )
parentEHPreOp.setEntryPoint('entry')
parentEHPreOp.setSourceCode("operator entry( Size index, io Scalar output ) { report('parentEHPreOp begin'); report('  Index: ' + index); report('  Output: ' +output); report('parentEHPreOp end'); }")

printErrors( parentEHPreOp )

parentEHPreBinding = fabricClient.DependencyGraph.createBinding()
parentEHPreBinding.setOperator( parentEHPreOp )
parentEHPreBinding.setParameterLayout( [ "node.index", "node.output" ] )

parentEHPostOp = fabricClient.DependencyGraph.createOperator( "parentEHPostOp" )
parentEHPostOp.setEntryPoint('entry')
parentEHPostOp.setSourceCode("operator entry( Size index, io Scalar output ) { report('parentEHPostOp begin'); report('  Index: ' + index); report('  Output: ' +output); report('parentEHPostOp end'); }")

printErrors( parentEHPostOp )

parentEHPostBinding = fabricClient.DependencyGraph.createBinding()
parentEHPostBinding.setOperator( parentEHPostOp )
parentEHPostBinding.setParameterLayout( [ "node.index", "node.output" ] )

parentEH = fabricClient.DependencyGraph.createEventHandler( "parentEH" )
parentEH.setScope( 'node', parentNode )
parentEH.preDescendBindings.append( parentEHPreBinding )
parentEH.postDescendBindings.append( parentEHPostBinding )

childEHPreOp = fabricClient.DependencyGraph.createOperator( "childEHPreOp" )
childEHPreOp.setEntryPoint('entry')
childEHPreOp.setSourceCode("operator entry( Size childIndex, io Scalar childOutput) { report('childEHPreOp begin'); report('  Child Index: ' + childIndex); report('  Child Output: ' + childOutput); report('childEHPreOp end'); }")

childEHPreBinding = fabricClient.DependencyGraph.createBinding()
childEHPreBinding.setOperator( childEHPreOp )
childEHPreBinding.setParameterLayout( [ "child.index", "child.output" ] )

childEHPostOp = fabricClient.DependencyGraph.createOperator( "childEHPostOp" )
childEHPostOp.setEntryPoint('entry')
childEHPostOp.setSourceCode("operator entry( Size childIndex, io Scalar childOutput ) { report('childEHPostOp begin'); report('  Child Index: ' + childIndex); report('  Child Output: ' + childOutput); report('childEHPostOp end'); }")

printErrors( childEHPostOp )

childEHPostBinding = fabricClient.DependencyGraph.createBinding()
childEHPostBinding.setOperator( childEHPostOp )
childEHPostBinding.setParameterLayout( [ "child.index", "child.output" ] )

childEH = fabricClient.DependencyGraph.createEventHandler( "childEH" )
childEH.appendChildEventHandler( parentEH )
childEH.setScope( 'child', childNode )
childEH.preDescendBindings.append( childEHPreBinding )
childEH.postDescendBindings.append( childEHPostBinding )

event = fabricClient.DependencyGraph.createEvent( "event" )
event.appendEventHandler( childEH )
event.fire()

fabricClient.close()

import json
import fabric
fabricClient = fabric.createClient()

node = fabricClient.DependencyGraph.createNode("foo")
node.addMember( "foo", "Integer", 17 )
node.addMember( "bar", "String", "red" )
node.addMember( "baz", "Scalar[]", [3.141] )
print(node.getBulkDataJSON())
node.setBulkDataJSON(json.dumps({'foo':[42],'bar':["fred"],'baz':[[4.5,3.6]]}))
print(node.getBulkDataJSON())
node.setCount(2)
print(node.getBulkDataJSON())

fabricClient.close()

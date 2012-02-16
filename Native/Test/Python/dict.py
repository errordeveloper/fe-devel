import fabric
F = fabric.createClient()
n = F.DG.createNode("n")

n.addMember("foo","String[String]")
a = n.getData("foo", 0)
print(fabric.stringify(a))
a['x'] = "bar"
n.setData("foo",0,a)
a = n.getData("foo", 0)
print(fabric.stringify(a))
a['x'] = "baz"
n.setData("foo",0,a)
a = n.getData("foo", 0)
print(fabric.stringify(a))

n.addMember("bar","Scalar[Scalar]")
a = n.getData("bar", 0)
print(fabric.stringify(a))
a[2.4] = 3.14
a[7.9] = 2.71
n.setData("bar", 0, a )
a = n.getData("bar", 0)
print(fabric.stringify(a))

a = {}
for i in range( 0, 4096 ):
  a["key "+str(i)] = "value "+str(i)
n.setData("foo",0,a)
a = n.getData("foo", 0)
print(fabric.stringify(a))

n.addMember("baz","Scalar[String][String]")
a = {
 'foo': {
   'a': 5.6,
   'b': 7.8
 },
 'bar': {
   'a': 3.1,
   'b': -2.1
 }
}
n.setData("baz", 0, a)
a = n.getData("baz", 0)
print(fabric.stringify(a))

F.close()

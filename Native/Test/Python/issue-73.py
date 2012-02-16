# test that setting slice count to 'None' gives exception
# and not a crash
import fabric
F = fabric.createClient()
node = F.DG.createNode("foo")
node.setCount(None)
node.evaluate()
F.close()

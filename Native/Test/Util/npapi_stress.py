from selenium import webdriver
from selenium.webdriver.support.ui import WebDriverWait
import psutil

baseUrl = "http://localhost/Fabric/Apps/Sample/"

def getMemMB( mem ):
  return str( round( mem / 1024.0 / 1024.0, 2 ) ) + " MB"

def printMem( p ):
  print str( p.name ) + ": " + getMemMB( p.get_memory_info().vms )
  for c in p.get_children():
    print "  " + str( c.name ) + ": " + getMemMB( c.get_memory_info().vms )

def testDemo( driver, drivername, testname, url, p ):
  driver.get( baseUrl + url )

  # wait for Fabric to say the scene is loaded
  WebDriverWait(driver, 10).until(lambda driver : driver.execute_script(" return typeof(fabricDemoSceneLoaded) != 'undefined' && fabricDemoSceneLoaded "))

  print "creating..."
  loop_js = "\
    c = FABRIC.createContext();\
    for (var i=0; i<100000; i++) {\
      c.DG.createNode('foo'+i);\
    }\
  "
  driver.execute_script( loop_js )

  print "flushing..."
  flush_js = "\
    c.flush();\
  "
  driver.execute_script( flush_js )

def basicDemosTest( driver, drivername ):
  # open a fixed-size window for testing
  driver.execute_script( "window.open('','testwindow','width=1024,height=768')" )
  driver.close()
  driver.switch_to_window( "testwindow" )

  p = psutil.Process( driver.binary.process.pid )

  for i in range(0, 20):
    printMem( p )
    print "looping  " + str( i ) + "..."
    testDemo( driver, drivername, "Skinning", "BasicDemos/CharacterSkinning.html", p )

  driver.quit()

basicDemosTest( webdriver.Firefox(), "Firefox" )
# basicDemosTest( webdriver.Chrome(), "Chrome" )


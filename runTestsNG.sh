(cd test/testng; sh build.sh)
java -cp 'project/jpype_java/dist/org.jpype.jar:project/epype_java/dist/epype_java.jar:lib/*:test/testng/classes'  org.testng.TestNG test/testng/testng.xml

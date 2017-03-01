
del *.cxx
del ..\..\java\src\main\java\com\example\swigtest\core\*.java

swig -c++ -java -package com.example.swigtest.core  -outdir ../../java/src/main/java/com/example/swigtest/core swig_test.i
pause

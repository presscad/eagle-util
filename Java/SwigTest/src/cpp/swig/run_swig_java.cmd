
del *.cxx
del ..\..\java\src\main\java\com\example\swigtest\*.java

swig -c++ -java -package com.example.swigtest  -outdir ../../java/src/main/java/com/example/swigtest swig_test.i
pause

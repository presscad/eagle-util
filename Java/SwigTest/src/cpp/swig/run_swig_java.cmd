
del *.java
del *.cxx

swig -c++ -java -package com.example.swigtest  -outdir ../../java/main/java/com/example/swigtest swig_test.i
pause

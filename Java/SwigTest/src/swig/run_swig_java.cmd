
del *.java
del *.cxx

swig -c++ -java -package com.example.swigtest  -outdir ../com/example/swigtest swig_test.i
pause

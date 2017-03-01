/* File : swig_test.i */
%module swig_test

%include "std_string.i"
%include "std_vector.i"

%{
#include "swig_test_core.h"
%}

%include "swig_test_core.h"

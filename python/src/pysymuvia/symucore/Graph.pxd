# distutils: language = c++

from libcpp.string cimport string
from libcpp cimport bool


cdef extern from "symucore/Graph/Model/Graph.h" namespace "SymuCore":
    cdef cppclass Graph:
        Graph() except +
        bool hasOriginOrDownstreamInterface(const string & strNodeName)

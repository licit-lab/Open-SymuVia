# distutils: language = c++

from pysymuvia.symucore.Graph cimport Graph

cdef class PyGraph:
    cdef Graph c_graph

    def hasOriginOrDownstreamInterface(self, nodename):
        return self.c_graph.hasOriginOrDownstreamInterface(bytes(nodename, 'UTF-8'))

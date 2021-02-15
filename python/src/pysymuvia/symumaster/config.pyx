# distutils: language = c++

from pysymuvia.symumaster.config cimport SimulationConfiguration

cdef class PySimulationConfiguration:
    cdef SimulationConfiguration *c_symconf

    def __cinit__(self):
        self.c_symconf = new SimulationConfiguration()

    def loadFromXML(self, filename):
        self.c_symconf.LoadFromXML(bytes(filename, 'UTF-8'))

    def getLoadedConfigurationFilePath(self):
      return self.c_symconf.GetLoadedConfigurationFilePath().decode("utf-8")

    def getStrOutpuDirectoryPath(self):
      return self.c_symconf.GetStrOutpuDirectoryPath().decode("utf-8")

    def getStrOutputPrefix(self):
      return self.c_symconf.GetStrOutputPrefix().decode("utf-8")

# distutils: language = c++

from cython.operator cimport dereference as deref

from pysymuvia.symumaster.runner cimport SimulationRunner
from pysymuvia.symumaster.config cimport SimulationConfiguration
from pysymuvia.symumaster.config import PySimulationConfiguration


cdef class PySimulationRunner:
    cdef SimulationRunner *c_runner
    cdef bool bEnd
    _conf = None
    _is_initialize = False

    def __cinit__(self, conf=None):
      self.c_runner = NULL
      if conf is not None:
        self.setConfig(conf)

    def __dealloc__(self):
      if self.c_runner is not NULL:
        del self.c_runner

    def run(self):
      return self.c_runner.Run()

    def step(self):
      if not self._is_initialize:
        self.init()
        
      if self.bEnd:
        self.bEnd = self.c_runner.RunNextAssignmentPeriod(self.bEnd)

    def init(self):
      self.bEnd = self.c_runner.Initialize()

    def setConfig(self, object conf):
      self._conf = conf
      new_conf = new SimulationConfiguration()
      filename = conf.getLoadedConfigurationFilePath()
      new_conf.LoadFromXML(bytes(filename, 'UTF-8'))
      self.c_runner = new SimulationRunner(deref(new_conf))

    @property
    def config(self):
      return self._config

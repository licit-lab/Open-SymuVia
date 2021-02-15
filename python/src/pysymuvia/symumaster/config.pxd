# distutils: language = c++

from libcpp.string cimport string
from libcpp cimport bool


cdef extern from "symumaster/Simulation/Config/SimulationConfiguration.h" namespace "SymuMaster":
    cdef cppclass SimulationConfiguration:
        SimulationConfiguration() except +
        bool LoadFromXML(const string & xmlConfigFile)
        const string & GetLoadedConfigurationFilePath() const
        const string GetStrOutpuDirectoryPath() const
        const string GetStrOutputPrefix() const

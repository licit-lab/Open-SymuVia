# distutils: language = c++

from libcpp.string cimport string
from libcpp cimport bool

from pysymuvia.symumaster.config cimport SimulationConfiguration

cdef extern from "symumaster/Simulation/SimulationRunner.h" namespace "SymuMaster":
    cdef cppclass SimulationRunner:
        SimulationRunner(SimulationConfiguration conf) except +
        bool Run();
        bool Initialize();
        bool RunNextAssignmentPeriod(bool & bEnd);

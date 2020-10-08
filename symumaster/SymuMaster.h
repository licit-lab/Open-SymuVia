#if defined SYMUMASTER_EXPORTS
#ifdef WIN32
#define DECLDIR __declspec(dllexport)
#else
#define DECLDIR __attribute__ ((visibility ("default")))
#endif
#else
#ifdef WIN32
#define DECLDIR __declspec(dllimport)
#else
#define DECLDIR __attribute__ ((visibility ("hidden")))
#endif
#endif

extern "C"
{
	// Simulation loop control methods
	DECLDIR bool LoadSimulation(const char * configFilePath);
	DECLDIR bool RunNextAssignmentPeriod(bool * bEnd);
	DECLDIR bool DropSimulation();

	// Available actions on the simulation :
	DECLDIR bool ForbidSymuviaLinks(char ** linkNames);
}

namespace SymuMaster {
	class SimulationRunner;
	class SimulationConfiguration;
	class SymuMasterCWrapper {
	public:
		SymuMasterCWrapper();
		~SymuMasterCWrapper();

		void Clean();
		bool Load(const char * configFile);
		bool RunNextAssignmentPeriod(bool * bEnd);
		bool DropSimulation();

		bool ForbidSymuviaLinks(char ** linkNames);

		SimulationConfiguration * m_pConfig;
		SimulationRunner * m_pSimulationRunner;
	};
}

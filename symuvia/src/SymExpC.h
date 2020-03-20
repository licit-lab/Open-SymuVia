#if defined DLL_EXPORT
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
	DECLDIR int SymRunEx( char* sFile );				// Run a full simulation
														// sFile : full path of SymuVia input file

	DECLDIR int SymLoadNetworkEx(char* sFile);			// Load SymuVia input file
														// sFile : full path of SymuVia input file

	DECLDIR bool SymRunNextStepEx(char *sOutputXmlFlow, bool bTrace, bool *bNEnd);		// Run a simulation time step
																						// sOutputXmlFlow : Output flow xml
																						// bTrace : tell if results are written in xml file
																						// bNEnd (output) : tell if simulation is finished

	DECLDIR int SymCreateVehicleEx(						// Create a vehicle 
		char*			sType,							// sType : type of the vehicle
		char*			sOrigin,						// sOrigin: ID of the node origin
		char*			sDestination,					// sDestination : ID of the node destination
		int				nLane,							// nLane :  lane number where the vehicle is created (1 is the most right lane)
		double			dbt								// Time of creation during the time step
	);												

	DECLDIR double SymDriveVehicleEx(						// Drive a vehicle
		int				nID,							// nID : vehicle ID
		char			*sLink,							// SLink : Link ID where locate vehicle (at the end of current time step)
		int				nVoie,							// nLane : lane number where locate vehicle (at the end of current time step)
		double			dbPos,
		bool			bForce
	);												

	DECLDIR int SymComputeRoutesEx(
		char* originId,
		char* destinationId,
		char* typeVeh,
		double dbTime,
		int nbShortestPaths,
		char ***pathLinks[],
		double* pathCosts[]
	);												// Compute routes
}


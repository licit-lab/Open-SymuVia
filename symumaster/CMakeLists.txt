project (SymuMaster VERSION 1.0.0)

# Set processor flags
add_definitions(-DUSE_SYMUCORE)

file(GLOB SymuMaster_src
    "SymuMasterExports.h"
    "Simulation/SimulationRunner.cpp"
    "Simulation/SimulationRunner.h"
    "Simulation/Simulators/TraficSimulatorHandler.cpp"
    "Simulation/Simulators/TraficSimulatorHandler.h"
    "Simulation/Simulators/TraficSimulatorHandlerFactory.cpp"
    "Simulation/Simulators/TraficSimulatorHandlerFactory.h"
	"Simulation/Simulators/TraficSimulatorInstance.cpp"
    "Simulation/Simulators/TraficSimulatorInstance.h"
    "Simulation/Simulators/SimulationDescriptor.cpp"
    "Simulation/Simulators/SimulationDescriptor.h"
	"Simulation/Simulators/SimulatorsInterface.cpp"
    "Simulation/Simulators/SimulatorsInterface.h"
    "Simulation/Simulators/TraficSimulators.cpp"
    "Simulation/Simulators/TraficSimulators.h"
    "Simulation/Assignment/AssignmentModel.cpp"
    "Simulation/Assignment/AssignmentModel.h"
    "Simulation/Assignment/AssignmentModelFactory.cpp"
    "Simulation/Assignment/AssignmentModelFactory.h"
    "Simulation/Assignment/MSADualLoop.cpp"
    "Simulation/Assignment/MSADualLoop.h"
    "Simulation/Assignment/HeuristicDualLoop.cpp"
    "Simulation/Assignment/HeuristicDualLoop.h"
    "Simulation/Simulators/SymuVia/SymuViaInstance.cpp"
    "Simulation/Simulators/SymuVia/SymuViaInstance.h"
	"Simulation/Simulators/SymuVia/SymuViaInterface.cpp"
    "Simulation/Simulators/SymuVia/SymuViaInterface.h"
    "Simulation/Simulators/SymuVia/SymuViaConfig.cpp"
    "Simulation/Simulators/SymuVia/SymuViaConfig.h"
    "Simulation/Simulators/Walk/WalkSimulator.cpp"
    "Simulation/Simulators/Walk/WalkSimulator.h"
    "Simulation/Users/UserEngine.cpp"
    "Simulation/Users/UserEngine.h"
	"Simulation/Users/UserEngines.cpp"
    "Simulation/Users/UserEngines.h"
    "Simulation/Users/TrackedUser.cpp"
    "Simulation/Users/TrackedUser.h"
    "Simulation/Users/PPath.cpp"
    "Simulation/Users/PPath.h"
    "Simulation/Users/UserEngineSnapshot.cpp"
    "Simulation/Users/UserEngineSnapshot.h"
    "Simulation/Users/UserPathWriting.cpp"
    "Simulation/Users/UserPathWriting.h"
	"Simulation/Users/UserPathWritings.cpp"
    "Simulation/Users/UserPathWritings.h"
    "Simulation/Config/SimulationConfiguration.cpp"
    "Simulation/Config/SimulationConfiguration.h"
    "Simulation/Config/SimulatorConfiguration.cpp"
    "Simulation/Config/SimulatorConfiguration.h"
    "Simulation/Config/SimulatorConfigurationFactory.cpp"
    "Simulation/Config/SimulatorConfigurationFactory.h"
	"IO/CostsWriting.cpp"
    "IO/CostsWriting.h"
    "IO/PredefinedPathFileHandler.cpp"
    "IO/PredefinedPathFileHandler.h"
    "IO/DemandFileHandler.cpp"
    "IO/DemandFileHandler.h"
	"IO/NetworkInterfacesFileHandler.h"
	"IO/NetworkInterfacesFileHandler.cpp"
    "IO/XMLErrorHandler.cpp"
    "IO/XMLErrorHandler.h"
    "IO/XMLLoader.cpp"
    "IO/XMLLoader.h"
    "IO/XMLStringUtils.cpp"
    "IO/XMLStringUtils.h"
    "IO/CSVStringUtils.cpp"
    "IO/CSVStringUtils.h"
    "Utils/ContainerForPopulationOriginDestination.h"
    "Utils/SymuMasterConstants.cpp"
    "Utils/SymuMasterConstants.h"
    "Utils/SystemUtils.cpp"
    "Utils/SystemUtils.h"
	"SymuMaster.cpp"
	"SymuMaster.h"
 )

# Files in project
print_files(SymuMaster_src)

# Define SymuMaster target: library
add_library(SymuMaster SHARED ${SymuMaster_src})

# Link dependencies
target_link_libraries(SymuMaster SymuCore)
target_link_libraries(SymuMaster SymuVia)

# Visible
target_include_directories(SymuMaster PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

# Définition de l'exécutable SymuMasterLauncher
add_executable(SymuMasterLauncher Launcher/SymuMasterLauncher.cpp Launcher/ColouredConsoleLogger.cpp Launcher/ColouredConsoleLogger.h)
target_link_libraries(SymuMasterLauncher SymuMaster)
target_link_libraries(SymuMasterLauncher ${Boost_LIBRARIES})

# file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/Resources/xsd/SymuMasterConfig.xsd DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Resources/xsd/SymuMasterConfig.xsd DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

install(TARGETS SymuMaster DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(TARGETS SymuMasterLauncher DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
# install(FILES ${SymuMaster_src} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/symumaster)

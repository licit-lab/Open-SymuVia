project (SymuCore VERSION 1.0.0)

file(GLOB SymuCore_src
    "src/Graph/Model/Cost.cpp"
    "src/Graph/Model/Cost.h"
    "src/Graph/Model/Penalty.cpp"
    "src/Graph/Model/Penalty.h"
    "src/Graph/Model/Graph.cpp"
    "src/Graph/Model/Graph.h"
    "src/Graph/Model/Link.cpp"
    "src/Graph/Model/Link.h"
    "src/Graph/Model/ListTimeFrame.cpp"
    "src/Graph/Model/ListTimeFrame.h"
    "src/Graph/Model/MultiLayersGraph.cpp"
    "src/Graph/Model/MultiLayersGraph.h"
    "src/Graph/Model/Node.cpp"
    "src/Graph/Model/Node.h"
    "src/Graph/Model/NullPattern.cpp"
    "src/Graph/Model/NullPattern.h"
    "src/Graph/Model/NullPenalty.cpp"
    "src/Graph/Model/NullPenalty.h"
    "src/Graph/Model/Pattern.cpp"
    "src/Graph/Model/Pattern.h"
    "src/Graph/Model/PatternsSwitch.cpp"
    "src/Graph/Model/PatternsSwitch.h"
    "src/Graph/Model/Driving/DrivingPattern.cpp"
    "src/Graph/Model/Driving/DrivingPattern.h"
    "src/Graph/Model/Driving/DrivingPenalty.cpp"
    "src/Graph/Model/Driving/DrivingPenalty.h"
    "src/Graph/Model/PublicTransport/PublicTransportLine.cpp"
    "src/Graph/Model/PublicTransport/PublicTransportLine.h"
    "src/Graph/Model/PublicTransport/PublicTransportPattern.cpp"
    "src/Graph/Model/PublicTransport/PublicTransportPattern.h"
	"src/Graph/Model/PublicTransport/WalkingPattern.cpp"
    "src/Graph/Model/PublicTransport/WalkingPattern.h"
    "src/Graph/Model/AbstractPenalty.cpp"
    "src/Graph/Model/AbstractPenalty.h"
    "src/Graph/Algorithms/Dijkstra.cpp"
    "src/Graph/Algorithms/Dijkstra.h"
    "src/Graph/Algorithms/KShortestPaths.cpp"
    "src/Graph/Algorithms/KShortestPaths.h"
    "src/Graph/Algorithms/ShortestPathsComputer.cpp"
    "src/Graph/Algorithms/ShortestPathsComputer.h"
    "src/Demand/DefaultMacroType.cpp"
    "src/Demand/DefaultMacroType.h"
    "src/Demand/DefaultPopulation.cpp"
    "src/Demand/DefaultPopulation.h"
    "src/Demand/Destination.cpp"
    "src/Demand/Destination.h"
    "src/Demand/MacroType.cpp"
    "src/Demand/MacroType.h"
    "src/Demand/Motive.cpp"
    "src/Demand/Motive.h"
    "src/Demand/Origin.cpp"
    "src/Demand/Origin.h"
    "src/Demand/Path.cpp"
    "src/Demand/Path.h"
    "src/Demand/Population.cpp"
    "src/Demand/Population.h"
    "src/Demand/Populations.cpp"
    "src/Demand/Populations.h"
	"src/Demand/PredefinedPath.cpp"
    "src/Demand/PredefinedPath.h"
    "src/Demand/Trip.cpp"
    "src/Demand/Trip.h"
    "src/Demand/SubPopulation.cpp"
    "src/Demand/SubPopulation.h"
    "src/Demand/ValuetedPath.cpp"
    "src/Demand/ValuetedPath.h"
    "src/Demand/VehicleType.cpp"
    "src/Demand/VehicleType.h"
	"src/Users/IUserHandler.cpp"
	"src/Users/IUserHandler.h"
	"src/Utils/EmissionUtils.cpp"
	"src/Utils/EmissionUtils.h"
    "src/Utils/Point.cpp"
    "src/Utils/Point.h"
	"src/Utils/RobustTravelIndicatorsHelper.cpp"
	"src/Utils/RobustTravelIndicatorsHelper.h"
	"src/Utils/SpatialMarginalsHelper.cpp"
	"src/Utils/SpatialMarginalsHelper.h"
    "src/Utils/SymuCoreConstants.cpp"
    "src/Utils/SymuCoreConstants.h"
	"src/Utils/TravelTimeUtils.cpp"
	"src/Utils/TravelTimeUtils.h"
	"src/Utils/SpatialMarginalsHelper.cpp"
	"src/Utils/SpatialMarginalsHelper.h"
	"src/Utils/RobustTravelIndicatorsHelper.cpp"
	"src/Utils/RobustTravelIndicatorsHelper.h"
	"src/Utils/TravelIndicatorClass.cpp"
	"src/Utils/TravelIndicatorClass.h"
    "src/SymuCoreExports.h"
 )

# Files in project
# print_files(SymuCore_src)

# Define SymuCore target: library
add_library(SymuCore SHARED ${SymuCore_src})

# Link dependencies
target_link_libraries(SymuCore ${Boost_DATE_TIME_LIBRARY} ${Boost_LOCALE_LIBRARY})

# Visible
target_include_directories(SymuCore PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

# Set processor flags
add_definitions(-DSYMUCORE_EXPORTS)

install(TARGETS SymuCore DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
# install(FILES ${SymuCore_src} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/symucore)

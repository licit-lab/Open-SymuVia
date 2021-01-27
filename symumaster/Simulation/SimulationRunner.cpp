#include "SimulationRunner.h"

#include "Users/UserEngine.h"
#include "Config/SimulationConfiguration.h"
#include "Simulators/TraficSimulatorHandlerFactory.h"
#include "Simulators/SimulationDescriptor.h"
#include "Simulators/TraficSimulatorInstance.h"
#include "Assignment/MSADualLoop.h"
#include "IO/DemandFileHandler.h"
#include "IO/PredefinedPathFileHandler.h"
#include "IO/CostsWriting.h"
#include "Users/UserPathWritings.h"
#include "Users/UserPathWriting.h"
#include "Users/UserEngine.h"

#include "Simulation/Simulators/SymuVia/SymuViaInstance.h"


#include <Demand/Populations.h>
#include <Demand/DefaultPopulation.h>
#include <Demand/Population.h>
#include <Demand/Trip.h>

#include <Graph/Model/MultiLayersGraph.h>
#include <Graph/Algorithms/Dijkstra.h>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/trivial.hpp>

#include <boost/thread.hpp>

#include <cassert>
#include <set>

using namespace SymuMaster;

SimulationRunner::SimulationRunner(const SimulationConfiguration & config) :
    m_Config(config)
{
    m_pAssignmentModel = config.GetAssignmentModel();
    m_pAssignmentModel->SetSimulationConfiguration(&config);
    m_Simulators.SetSimulationConfig(&config);
	m_pCostsWriting = nullptr;
	m_pTripOutputWritings = nullptr;
}

SimulationRunner::~SimulationRunner()
{
	if (m_pCostsWriting)
	{
		delete m_pCostsWriting;
	}
	if (m_pTripOutputWritings)
	{
		delete m_pTripOutputWritings;
	}
}

bool SimulationRunner::Terminate()
{
	return m_Simulators.Terminate();
}

bool SimulationRunner::Run()
{
    clock_t start_s = clock();

    bool bOk = Initialize();
    if(bOk)
    {
        bOk = Execute();
    }

	// Calling terminate no matter what
	bool bTerminateOk = Terminate();
	if (!bTerminateOk)
	{
		bOk = false;
	}

    clock_t stop_s = clock();
    BOOST_LOG_TRIVIAL(info) << "Computation Time in second: " << (stop_s - start_s) / double(CLOCKS_PER_SEC);

    return bOk;
}

bool SimulationRunner::Initialize()
{
    assert(m_Simulators.Empty());

	m_iAssignmentPeriodIndex = 0;

    int nbSimulatorInstances = m_pAssignmentModel->NeedsParallelSimulationInstances() ? m_Config.GetMaxParallelSimulationsNb() : 1;
    nbSimulatorInstances = std::min<int>(nbSimulatorInstances, m_pAssignmentModel->GetMaxSimulationInstancesNumber());

    // Simulators creation
    for (size_t iSimConfig = 0; iSimConfig < m_Config.GetLstSimulatorConfigs().size(); iSimConfig++)
    {
        SimulatorConfiguration * pSimConfig = m_Config.GetLstSimulatorConfigs()[iSimConfig];
        TraficSimulatorHandler * pSimhandler = TraficSimulatorHandlerFactory::CreateTraficSimulatorHandler(pSimConfig, &m_Simulators, nbSimulatorInstances);
        m_Simulators.AddSimulatorHandler(pSimhandler);
    }

    m_UserEngines.Initialize(nbSimulatorInstances, m_Config, m_Simulators.GetSimulators());

    bool bOk = m_Simulators.Initialize(m_Config, m_UserEngines);

	DemandFileHandler demandFileHandler;
	if (bOk)
	{
		bOk = LoadDemand(demandFileHandler);

		// after origin and destination creation, merge suborigins and subdestinations into main origins and destinations
		// if the option to aggregate at parent level is checked
		if (bOk && m_Config.DoWorkAtParentODLevel())
		{
			m_Simulators.MergeRelatedODs();
		}
	}
	if (bOk)
	{
		bOk = BuildSimulationGraph();
	}
	if (bOk)
	{
		bOk = demandFileHandler.CheckOriginAndDestinations(m_Simulators.GetSimulationDescriptor().GetGraph());
	}
	if (bOk)
	{
		bOk = demandFileHandler.AddUsersPredefinedPaths(m_Simulators.GetSimulationDescriptor().GetGraph(), (int)m_UserEngines.GetUserEngines().size());
		if (bOk && !m_Config.GetPredefinedPathsFilePath().empty())
		{
			PredefinedPathFileHandler predefinedPathFileHandler;
			bOk = predefinedPathFileHandler.FillPredefinedPaths(m_Config.GetPredefinedPathsFilePath(), m_Simulators.GetSimulationDescriptor().GetGraph(), m_Simulators.GetSimulationDescriptor().GetPopulations(), m_pAssignmentModel->GetListPredefinedPaths());
		}
	}

	if (bOk)
	{
		if (!m_pAssignmentModel)
		{
			BOOST_LOG_TRIVIAL(error) << "No assignement defined";
			return false;
		}

		// For now, we just run the simulation from startTime to endTime
		BOOST_LOG_TRIVIAL(info) << "Simulation from time " << m_Simulators.GetSimulationDescriptor().GetStartTime() << " to time " << m_Simulators.GetSimulationDescriptor().GetEndTime();
		BOOST_LOG_TRIVIAL(info) << "Using prediction duration : " << m_Config.GetPredictionPeriod();
		BOOST_LOG_TRIVIAL(info) << "Using assignment period duration : " << m_Config.GetAssignmentPeriod();
		BOOST_LOG_TRIVIAL(info) << "Using travel times update period : " << m_Config.GetTravelTimesUpdatePeriod();


		m_nbAssignmentPeriod = (int)ceil((double)(m_Simulators.GetSimulationDescriptor().GetEndTime() - m_Simulators.GetSimulationDescriptor().GetStartTime()).ticks() / (double)m_Config.GetAssignmentPeriod().ticks());
		m_nbTravelTimeUpdatePeriodsByPeriod = (int)ceil((double)m_Config.GetPredictionPeriod().ticks() / (double)m_Config.GetTravelTimesUpdatePeriod().ticks());

		BOOST_LOG_TRIVIAL(info) << "Number of assignment / prediction periods : " << m_nbAssignmentPeriod;
		BOOST_LOG_TRIVIAL(info) << "Number of travel time update periods by prediction period : " << m_nbTravelTimeUpdatePeriodsByPeriod;

		m_bRedoLastIteration = ((m_Config.DoExtraIterationToProduceOutputs() && m_Simulators.IsExtraIterationToProduceOutputsRelevant()) || m_pAssignmentModel->NeedsParallelSimulationInstances());

		std::vector<SymuCore::Trip*> listTripsNoOptimizer;
		SortTrip(m_Simulators.GetPopulations(), m_Simulators.GetSimulationDescriptor().GetStartTime(), m_Config.GetAssignmentPeriod(), m_nbAssignmentPeriod, m_Config.GetPredictionPeriod(), m_listSortedTrips, listTripsNoOptimizer);

		m_pAssignmentModel->Initialize(m_Simulators.GetSimulationDescriptor().GetStartTime(), m_Simulators.GetSimulationDescriptor().GetEndTime(), m_Simulators.GetPopulations().getListPopulations(), m_mapCostFunctions,
			m_pAssignmentModel->GetSimulationConfiguration()->GetDayToDayAssignement(), m_pAssignmentModel->GetSimulationConfiguration()->UseCostsForTemporalShortestPath());

		assert(m_pTripOutputWritings == nullptr);
		assert(m_pCostsWriting == nullptr);

		m_pTripOutputWritings = new UserPathWritings(m_Config, m_UserEngines);
		m_pCostsWriting = m_Config.bWriteCosts() ? new CostsWriting(m_Config, m_Simulators.GetSimulationDescriptor()) : NULL;

		// Initial (and final) assignment for users ignored by the optimizer (all or nothing)
		if (!listTripsNoOptimizer.empty()) {
			bOk = m_Simulators.FillEstimatedCostsForNewPeriod(m_Simulators.GetSimulationDescriptor().GetStartTime(),
				m_Simulators.GetSimulationDescriptor().GetStartTime(),
				m_Simulators.GetSimulationDescriptor().GetEndTime(),
				m_Config.GetTravelTimesUpdatePeriod(), m_mapCostFunctions, 1);
			if (bOk)
			{
				bOk = AssignAllToShortestPath(listTripsNoOptimizer, (int)m_UserEngines.GetUserEngines().size());
			}
		}

		m_currentTime = m_Simulators.GetSimulationDescriptor().GetStartTime();
	}

	return bOk;
}

bool SimulationRunner::LoadDemand(DemandFileHandler& demandFileHandler)
{
    bool bOk = true;

    // cloning input populations definition into the simulation descriptor.
    m_Simulators.GetSimulationDescriptor().GetPopulations().DeepCopy(*m_Config.GetPopulations(), m_Simulators.GetSimulationDescriptor().GetGraph());

    if(m_Config.GetDemandFilePath().empty()) //No Demand file defined
    {
        // if no populations definition, we use a global default population :
        if (m_Simulators.GetSimulationDescriptor().GetPopulations().getListPopulations().empty())
        {
            SymuCore::DefaultPopulation* newDefaultPopulation = new SymuCore::DefaultPopulation();
            m_Simulators.GetSimulationDescriptor().GetPopulations().getListPopulations().push_back(newDefaultPopulation);
            m_Simulators.GetSimulationDescriptor().GetGraph()->AddMacroType(newDefaultPopulation->GetMacroType());
        }

        // load trips for the simulation's period
        BOOST_LOG_TRIVIAL(info) << "Generating demand data from simulators scenarios...";
        std::vector<SymuCore::Trip> placeHolder;
        bOk = m_Simulators.FillTripsDescriptor(m_Simulators.GetSimulationDescriptor().GetGraph(), m_Simulators.GetSimulationDescriptor().GetPopulations(),
                                               m_Simulators.GetSimulationDescriptor().GetStartTime(), m_Simulators.GetSimulationDescriptor().GetEndTime(),
                                               m_Config.DoWorkAtParentODLevel(), placeHolder);

        if (bOk)
        {
            BOOST_LOG_TRIVIAL(info) << "Demand data successfuly generated";

            // write demand file as CSV for future use instead of recomputing it every time
            demandFileHandler.Write(m_Config, m_Simulators.GetSimulationDescriptor());
        }
    }
    else
    {

        BOOST_LOG_TRIVIAL(info) << "Loading demande file : '" << m_Config.GetDemandFilePath() << "'";
        bOk = demandFileHandler.FillDemands(m_Config.GetDemandFilePath(), m_Simulators.GetSimulationDescriptor().GetStartTime(), m_Simulators.GetSimulationDescriptor().GetEndTime(),
                                            m_Simulators.GetSimulationDescriptor().GetPopulations(), m_Simulators.GetSimulationDescriptor().GetGraph());
    }

	if (bOk)
	{
		m_Simulators.PostFillDemand();

		BOOST_LOG_TRIVIAL(info) << "Demand file successfuly loaded";
	}

    return bOk;
}

bool SimulationRunner::BuildSimulationGraph()
{
    // build the aggregated simulation descriptor
    return m_Simulators.BuildSimulationGraph(NULL, NULL); //no multiLayersGraph
}


// Thread class used to do simulations in parallel
class SimulationThread {
public:
    SimulationThread(TraficSimulators & traficSimulators,
        int iInstance,
        UserEngine * pUserEngine,
        const boost::posix_time::ptime & startPeriodTime,
        const boost::posix_time::ptime & endPeriodTime,
        const boost::posix_time::time_duration & travelTimesUpdatePeriod,
        const boost::posix_time::ptime & endAssignmentPeriodTime,
        int nbTravelTimeUpdatePeriods,
        const std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> & mapCostFunctions) :
        m_TraficSimulators(traficSimulators),
        m_iInstance(iInstance),
        m_pUserEngine(pUserEngine),
        startPeriodTime(startPeriodTime),
        endPeriodTime(endPeriodTime),
        travelTimesUpdatePeriod(travelTimesUpdatePeriod),
        endAssignmentPeriodTime(endAssignmentPeriodTime),
        nbTravelTimeUpdatePeriods(nbTravelTimeUpdatePeriods),
        mapCostFunctions(mapCostFunctions),
        m_Thread(&SimulationThread::run, this)
    {
    }

    void run() {
        currentTime = startPeriodTime;

        bAssignmentPeriodEnded = false;

        bOk = true;
        for (int iTravelTimesUpdatePeriod = 0; iTravelTimesUpdatePeriod < nbTravelTimeUpdatePeriods && bOk; iTravelTimesUpdatePeriod++)
        {
            boost::posix_time::ptime startTravelTimesPeriodTime = currentTime;
            boost::posix_time::ptime endTravelTimesPeriodTime = std::min<boost::posix_time::ptime>(startTravelTimesPeriodTime + travelTimesUpdatePeriod, endPeriodTime);

            BOOST_LOG_TRIVIAL(info) << "Starting travel time update period " << (iTravelTimesUpdatePeriod + 1) << ", " << startTravelTimesPeriodTime << " -> " << endTravelTimesPeriodTime;

            while (currentTime < endTravelTimesPeriodTime && bOk)
            {
                // If this is the last timestep We take a snapshot of the end of the assignmentPeriod before continuing the prediction period.
                if (!bAssignmentPeriodEnded && currentTime + m_TraficSimulators.GetSimulationDescriptor().GetTimeStepDuration() > endAssignmentPeriodTime)
                {
                    BOOST_LOG_TRIVIAL(info) << "Assignment period ended : taking simulation snapshot...";

                    bAssignmentPeriodEnded = true;
                    bOk = m_TraficSimulators.AssignmentPeriodEnded(m_iInstance);
                    if (!bOk)
                    {
                        break;
                    }
                    m_pUserEngine->AssignmentPeriodEnded();
                    m_pUserEngine->GetUserPathWriter()->AssignmentPeriodEnded();
                }

                bOk = m_TraficSimulators.Step(currentTime, m_TraficSimulators.GetSimulationDescriptor().GetTimeStepDuration(), m_iInstance, m_pUserEngine);
                currentTime += m_TraficSimulators.GetSimulationDescriptor().GetTimeStepDuration();
            }

            if (bOk)
            {
                bOk = m_TraficSimulators.ComputeCosts(iTravelTimesUpdatePeriod, startTravelTimesPeriodTime, endTravelTimesPeriodTime, mapCostFunctions, m_iInstance);
            }
        }

        // do the post processing of the cost computations (for spatial marginals for exemple) :
        if (bOk)
        {
            m_TraficSimulators.PostProcessCosts(m_iInstance, mapCostFunctions);
        }
    }

    void join() {
        m_Thread.join();
    }

    bool IsOK() {
        return bOk;
    }

    const boost::posix_time::ptime & GetCurrentTime() {
        return currentTime;
    }

    bool GetAssignmentPeriodEnded() {
        return bAssignmentPeriodEnded;
    }

private:
    TraficSimulators & m_TraficSimulators;
    int m_iInstance;
    UserEngine * m_pUserEngine;
    const boost::posix_time::ptime & startPeriodTime;
    const boost::posix_time::ptime & endPeriodTime;
    const boost::posix_time::time_duration & travelTimesUpdatePeriod;
    const boost::posix_time::ptime & endAssignmentPeriodTime;
    int nbTravelTimeUpdatePeriods;
    std::map<SymuCore::SubPopulation*, SymuCore::CostFunction> mapCostFunctions;
    boost::posix_time::ptime currentTime;
    bool bOk;
    bool bAssignmentPeriodEnded;

    boost::thread m_Thread;
};


bool SimulationRunner::Execute()
{
	bool bOk = true;
	bool bEnd = false;
	while (bOk && !bEnd)
    {
		bOk = RunNextAssignmentPeriod(bEnd);
    }

    return bOk;
}

bool SimulationRunner::IsEndReached()
{
	return m_iAssignmentPeriodIndex >= m_nbAssignmentPeriod;
}

bool SimulationRunner::RunNextAssignmentPeriod(bool &bEnd)
{
	bool bOk = true;
	bool bPeriodTerminated = false;

	bool bIsRedoingLastIteration = false;
	int iCurrentPeriodIteration = 0;
	while (bOk && !bPeriodTerminated)
	{
		if (bIsRedoingLastIteration)
		{
			BOOST_LOG_TRIVIAL(info) << "Redoing assignment period " << (m_iAssignmentPeriodIndex + 1) << " of " << m_nbAssignmentPeriod << " for writing simulators results";
		}
		else
		{
			BOOST_LOG_TRIVIAL(info) << (iCurrentPeriodIteration > 0 ? "Restarting" : "Starting") << " assignment period " << (m_iAssignmentPeriodIndex + 1) << " of " << m_nbAssignmentPeriod;
		}

		boost::posix_time::ptime startPeriodTime = m_currentTime;
		boost::posix_time::ptime endPeriodTime = std::min<boost::posix_time::ptime>(startPeriodTime + m_Config.GetPredictionPeriod(), m_Simulators.GetSimulationDescriptor().GetEndTime());
		boost::posix_time::ptime endAssignmentPeriodTime = std::min<boost::posix_time::ptime>(startPeriodTime + m_Config.GetAssignmentPeriod(), m_Simulators.GetSimulationDescriptor().GetEndTime());

		boost::posix_time::time_duration realAssignmentPeriod = endAssignmentPeriodTime - startPeriodTime;
		boost::posix_time::time_duration realPredictionPeriod = endPeriodTime - startPeriodTime;

		int nbTravelTimeUpdatePeriods = std::min<int>(m_nbTravelTimeUpdatePeriodsByPeriod, (int)ceil((double)(endPeriodTime - startPeriodTime).ticks() / (double)m_Config.GetTravelTimesUpdatePeriod().ticks()));

		// Ask the assignment model for the number of simulation instances it needs for the current period
		int nbSimulationInstances;
		int nbMaxSimulationInstancesForPeriod = std::min<int>(m_pAssignmentModel->GetMaxSimulationInstancesNumber(), m_Config.GetMaxParallelSimulationsNb());
		if (bIsRedoingLastIteration)
		{
			// for the resync iteration, we need to do it on all simulator instances
			nbSimulationInstances = m_pAssignmentModel->GetMaxSimulationInstancesNumber();
		}
		else
		{
			bOk = m_pAssignmentModel->GetNeededSimulationInstancesNumber(m_listSortedTrips[m_iAssignmentPeriodIndex], startPeriodTime, realPredictionPeriod, m_Config.GetTravelTimesUpdatePeriod(), iCurrentPeriodIteration, m_iAssignmentPeriodIndex, nbSimulationInstances);
			if (!bOk) {
				break;
			}
		}

		assert(nbSimulationInstances <= m_pAssignmentModel->GetMaxSimulationInstancesNumber());

		bOk = m_Simulators.AssignmentPeriodStart(m_iAssignmentPeriodIndex, m_bRedoLastIteration && !bIsRedoingLastIteration, nbSimulationInstances);
		m_UserEngines.AssignmentPeriodStart(nbSimulationInstances);
		m_pTripOutputWritings->AssignmentPeriodStart(nbSimulationInstances);

		if (!bOk) {
			break;
		}

		int nbBatches = (int)ceil((double)nbSimulationInstances / m_Config.GetMaxParallelSimulationsNb());
		int nbSimulationInstanceForLastBatch = nbSimulationInstances % m_Config.GetMaxParallelSimulationsNb();
		if (nbSimulationInstanceForLastBatch == 0)
		{
			nbSimulationInstanceForLastBatch = m_Config.GetMaxParallelSimulationsNb();
		}
		int nbParallelSimulationMaxNumber = nbBatches > 1 ? m_Config.GetMaxParallelSimulationsNb() : nbSimulationInstanceForLastBatch;

		bool bGlobalAssignmentPeriodEnded = false;

		int iCurrentSimulationInstanceOffset = 0;
		for (int iBatch = 0; iBatch < nbBatches && bOk; iBatch++)
		{
			int nbSimulationInstancesForThisBatch = (iBatch == nbBatches - 1) ? nbSimulationInstanceForLastBatch : nbParallelSimulationMaxNumber;

			if (iBatch != 0)
			{
				// we have to rollback between each batch.
				m_Simulators.AssignmentPeriodRollback(nbSimulationInstances);
				m_UserEngines.AssignmentPeriodRollback(nbSimulationInstances);
				m_pTripOutputWritings->AssignmentPeriodRollback(nbSimulationInstances);
			}

			if (iCurrentPeriodIteration == 0)
			{
				// patterns and switching costs generation for the started period
				bOk = m_Simulators.FillEstimatedCostsForNewPeriod(m_Simulators.GetSimulationDescriptor().GetStartTime(),
					startPeriodTime, endPeriodTime, m_Config.GetTravelTimesUpdatePeriod(), m_mapCostFunctions, nbMaxSimulationInstancesForPeriod);

				if (!bOk) {
					break;
				}
			}

			// get the assignment for each simulation instance of the batch
			for (int iSimulationInstanceForBatch = 0; iSimulationInstanceForBatch < nbSimulationInstancesForThisBatch && bOk; iSimulationInstanceForBatch++)
			{
				// paths determination by the assignment model for this new iteration :
				if (!bIsRedoingLastIteration || m_pAssignmentModel->NeedsParallelSimulationInstances()) // when using parallel simulations, we have to assign every time
				{
					bOk = m_pAssignmentModel->AssignPathsToTrips(m_listSortedTrips[m_iAssignmentPeriodIndex], startPeriodTime, realPredictionPeriod,
						m_Config.GetTravelTimesUpdatePeriod(), iCurrentPeriodIteration, m_iAssignmentPeriodIndex, iCurrentSimulationInstanceOffset + iSimulationInstanceForBatch, iSimulationInstanceForBatch, bIsRedoingLastIteration);
				}

				if (bOk)
				{
					// provide simulators the path assignment for the period for some special cases (some non trip based simulators may need to know
					// the demand per route before the trips activations). We have to do it event when AssignPathsToTrips has not been recalled (MFD demands
					// have to be re-set from the assignment since the snapshot reload lost them)
					bOk = m_Simulators.ProcessPathsAssignment(m_listSortedTrips[m_iAssignmentPeriodIndex], m_Config.GetPredictionPeriod(), startPeriodTime, endPeriodTime, iSimulationInstanceForBatch);
				}
			}

			if (!bOk) {
				break;
			}

			std::vector<SimulationThread*> threads;
			for (int iThread = 0; iThread < nbSimulationInstancesForThisBatch; iThread++)
			{
				threads.push_back(new SimulationThread(m_Simulators,
					iThread,
					m_UserEngines.GetUserEngines().at(iThread),
					startPeriodTime, endPeriodTime, m_Config.GetTravelTimesUpdatePeriod(), endAssignmentPeriodTime, nbTravelTimeUpdatePeriods, m_mapCostFunctions));
			}

			for (int iThread = 0; iThread < nbSimulationInstancesForThisBatch; iThread++)
			{
				SimulationThread * pThread = threads.at(iThread);

				pThread->join();

				bOk = bOk && pThread->IsOK();

				if (bOk)
				{
					if (iThread > 0)
					{
						m_Simulators.MergeCostsForSecondaryInstance(iThread);
					}

					// write costs and travel times if asked
					if (m_pCostsWriting && !bIsRedoingLastIteration && m_Config.bWriteCostsForEveryIteration())
					{
						for (int iTTUpdatePeriod = 0; iTTUpdatePeriod < nbTravelTimeUpdatePeriods && bOk; iTTUpdatePeriod++)
						{
							boost::posix_time::ptime startTravelTimesPeriodTime = startPeriodTime + m_Config.GetTravelTimesUpdatePeriod() * iTTUpdatePeriod;
							boost::posix_time::ptime endTravelTimesPeriodTime = std::min<boost::posix_time::ptime>(startTravelTimesPeriodTime + m_Config.GetTravelTimesUpdatePeriod(), endPeriodTime);

							bOk = m_pCostsWriting->WriteCosts(m_iAssignmentPeriodIndex, iCurrentPeriodIteration, iCurrentSimulationInstanceOffset + iThread, iThread,
								startTravelTimesPeriodTime, endTravelTimesPeriodTime, ((double)((startTravelTimesPeriodTime + (endTravelTimesPeriodTime - startTravelTimesPeriodTime) / 2) - m_Simulators.GetSimulationDescriptor().GetStartTime()).total_microseconds()) / 1000000,
								m_Simulators.GetSimulationDescriptor().GetPopulations(), m_Simulators.GetSimulationDescriptor().GetGraph());
						}
					}

					if (bOk && m_Config.bWritePpathAtEachIteration() && !bIsRedoingLastIteration)
					{
						bOk = m_pTripOutputWritings->WriteUserPaths(m_iAssignmentPeriodIndex, iCurrentPeriodIteration, iCurrentSimulationInstanceOffset,
							m_Simulators.GetSimulationDescriptor().GetStartTime(), startPeriodTime, realAssignmentPeriod, realPredictionPeriod, false);
					}

					// entry point for the assignment model to use the graph measured costs for the instance before the next batch overwrites it.
					bOk = bOk && m_pAssignmentModel->onSimulationInstanceDone(m_listSortedTrips[m_iAssignmentPeriodIndex], endPeriodTime, iCurrentSimulationInstanceOffset + iThread, iThread, iCurrentPeriodIteration);

					// get updated global values (could be done just once)
					m_currentTime = pThread->GetCurrentTime();
					if (pThread->GetAssignmentPeriodEnded())
					{
						bGlobalAssignmentPeriodEnded = true;
					}
				}

				delete pThread;
			}

			iCurrentSimulationInstanceOffset += nbSimulationInstancesForThisBatch;

			// just do one batch if redoing assignment to keep to resync all simulator instances
			if (bIsRedoingLastIteration) break;

		} // end of batch loop

		if (bOk)
		{
			// End of assignment period :

			// check if rollback or commit :
			if (!bIsRedoingLastIteration && m_pAssignmentModel->IsRollbackNeeded(m_listSortedTrips[m_iAssignmentPeriodIndex], startPeriodTime, realPredictionPeriod, iCurrentPeriodIteration, nbParallelSimulationMaxNumber, bOk))
			{
				if (bOk)
				{
					BOOST_LOG_TRIVIAL(info) << "Rollback needed for current assignment period " << (m_iAssignmentPeriodIndex + 1) << " of " << m_nbAssignmentPeriod;
					bOk = m_Simulators.AssignmentPeriodRollback(nbSimulationInstances);
					m_UserEngines.AssignmentPeriodRollback(nbSimulationInstances);
					m_pTripOutputWritings->AssignmentPeriodRollback(nbSimulationInstances);
					m_currentTime = startPeriodTime;
					iCurrentPeriodIteration++;
				}
			}
			else
			{
				if (bOk)
				{
					if (m_bRedoLastIteration && !bIsRedoingLastIteration)
					{
						bIsRedoingLastIteration = true;
						bOk = m_Simulators.AssignmentPeriodRollback(nbSimulationInstances);
						m_UserEngines.AssignmentPeriodRollback(nbSimulationInstances);
						m_pTripOutputWritings->AssignmentPeriodRollback(nbSimulationInstances);
						m_currentTime = startPeriodTime;
					}
					else
					{
						bIsRedoingLastIteration = false;

						if (bOk && (m_iAssignmentPeriodIndex + 1) == m_nbAssignmentPeriod)
							bOk = m_pTripOutputWritings->WriteUserPaths(m_iAssignmentPeriodIndex, iCurrentPeriodIteration, 0, 
								m_Simulators.GetSimulationDescriptor().GetStartTime(), startPeriodTime, realAssignmentPeriod, realPredictionPeriod, true); // write final paths for trips

						// write costs and travel times if asked and if not already written
						if (bOk && m_pCostsWriting && !m_Config.bWriteCostsForEveryIteration())
						{
							for (int iTTUpdatePeriod = 0; iTTUpdatePeriod < nbTravelTimeUpdatePeriods && bOk; iTTUpdatePeriod++)
							{
								boost::posix_time::ptime startTravelTimesPeriodTime = startPeriodTime + m_Config.GetTravelTimesUpdatePeriod() * iTTUpdatePeriod;
								boost::posix_time::ptime endTravelTimesPeriodTime = std::min<boost::posix_time::ptime>(startTravelTimesPeriodTime + m_Config.GetTravelTimesUpdatePeriod(), endPeriodTime);

								bOk = m_pCostsWriting->WriteCosts(m_iAssignmentPeriodIndex, iCurrentPeriodIteration, 0, 0, startTravelTimesPeriodTime, endTravelTimesPeriodTime,
									((double)((startTravelTimesPeriodTime + (endTravelTimesPeriodTime - startTravelTimesPeriodTime) / 2) - m_Simulators.GetSimulationDescriptor().GetStartTime()).total_microseconds()) / 1000000,
									m_Simulators.GetSimulationDescriptor().GetPopulations(), m_Simulators.GetSimulationDescriptor().GetGraph());
							}
						}

						if (bOk)
							bOk = m_Simulators.AssignmentPeriodCommit(nbSimulationInstances, bGlobalAssignmentPeriodEnded);
						m_UserEngines.AssignmentPeriodCommit();
						m_pTripOutputWritings->AssignmentPeriodCommit();

						bPeriodTerminated = true;

						m_currentTime = endAssignmentPeriodTime;
						m_iAssignmentPeriodIndex++;
					}
				}
			}
		}
	}

	bEnd = bOk && IsEndReached();

	return bOk;
}

void SimulationRunner::SortTrip(const SymuCore::Populations &listPopulations, boost::posix_time::ptime startTime, boost::posix_time::time_duration assignmentPeriod,
    int nbPeriods, boost::posix_time::time_duration predictionPeriod,
    std::vector<std::vector<SymuCore::Trip*> > & listResultTrips,
    std::vector<SymuCore::Trip*> & listResultTripsNoOptimizer)
{
    struct TripCompare
    {
        bool operator()(const SymuCore::Trip* s1, const SymuCore::Trip* s2) const
        {
            if(s1->GetDepartureTime() == s2->GetDepartureTime())
                return s1->GetID() < s2->GetID();
            return s1->GetDepartureTime() < s2->GetDepartureTime();
        }
    };

    std::vector<std::multiset<SymuCore::Trip*, TripCompare> > listSortedTrip(nbPeriods);
	std::multiset<SymuCore::Trip*, TripCompare> sortTripForPopulation;
    listResultTrips.resize(nbPeriods);
    SymuCore::Population* currentPopulation;
    SymuCore::Trip* currentTrip;
    int indexAssignmentPeriod;

    for (size_t iPopulation = 0; iPopulation < listPopulations.getListPopulations().size(); iPopulation++)
    {
        currentPopulation = listPopulations.getListPopulations()[iPopulation];
		sortTripForPopulation.clear();
        for(size_t iTrip = 0; iTrip < currentPopulation->GetListTrips().size(); iTrip++)
        {
            currentTrip = currentPopulation->GetListTrips().at(iTrip);

			if (currentTrip->IsDeactivated())
				continue;

            sortTripForPopulation.insert(currentTrip);

            //check if this trip hasn't already a predefined path
            if(currentTrip->isPredefinedPath() && !m_pAssignmentModel->GetSimulationConfiguration()->GetDayToDayAssignement())
            {
                continue;
            }

            assert(!currentTrip->GetDepartureTime().is_special());
            // The trip is part of an assignment period :
            indexAssignmentPeriod = std::min(nbPeriods - 1, (int)floor((double)(currentTrip->GetDepartureTime() - startTime).ticks() / (double)assignmentPeriod.ticks()));

            // special handling of users of populations with no optimization
            if (currentPopulation->GetDisableOptimization())
            {
                listResultTripsNoOptimizer.push_back(currentTrip);
            }
            else
            {
                listSortedTrip[indexAssignmentPeriod].insert(currentTrip);

                // The trip might also be part of previous prediction periods :
                int iPreviousPeriod = indexAssignmentPeriod - 1;
                while (iPreviousPeriod >= 0 && currentTrip->GetDepartureTime() < startTime + assignmentPeriod*iPreviousPeriod + predictionPeriod)
                {
                    listSortedTrip[iPreviousPeriod].insert(currentTrip);
                    iPreviousPeriod--;
                }
            }
        }
        currentPopulation->SetListTrips(std::vector<SymuCore::Trip*>(sortTripForPopulation.begin(), sortTripForPopulation.end()));
    }

    for(size_t i=0; i<listSortedTrip.size(); i++)
    {
        listResultTrips[i] = std::vector<SymuCore::Trip*>(listSortedTrip[i].begin(), listSortedTrip[i].end());
    }
}

bool SimulationRunner::AssignAllToShortestPath(const std::vector<SymuCore::Trip*> & trips, int nbInstances) const
{
    // sort trips by subpopulation, origin and destination
    std::map<SymuCore::SubPopulation*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, std::vector<SymuCore::Trip*> > > > mapSortedTrips;
    for (size_t i = 0; i < trips.size(); i++)
    {
        SymuCore::Trip * pTrip = trips.at(i);
        mapSortedTrips[pTrip->GetSubPopulation()][pTrip->GetOrigin()][pTrip->GetDestination()].push_back(pTrip);
    }

    SymuCore::Dijkstra dijkstra(true);
    for (std::map<SymuCore::SubPopulation*, std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, std::vector<SymuCore::Trip*> > > >::const_iterator iterSubPop = mapSortedTrips.begin(); iterSubPop != mapSortedTrips.end(); ++iterSubPop)
    {
        for (std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, std::vector<SymuCore::Trip*> > >::const_iterator iterOrigin = iterSubPop->second.begin(); iterOrigin != iterSubPop->second.end(); ++iterOrigin)
        {
            std::vector<SymuCore::Origin*> origin(1, iterOrigin->first);
            std::vector<SymuCore::Destination*> destinations;
            for (std::map<SymuCore::Destination*, std::vector<SymuCore::Trip*> >::const_iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                destinations.push_back(iterDestination->first);
            }
            std::map<SymuCore::Origin*, std::map<SymuCore::Destination*, std::vector<SymuCore::ValuetedPath*> > > result = dijkstra.getShortestPaths(0, origin, destinations, iterSubPop->first, 0);
            const std::map<SymuCore::Destination*, std::vector<SymuCore::ValuetedPath*> > & resultForOrigin = result[iterOrigin->first];
            for (std::map<SymuCore::Destination*, std::vector<SymuCore::Trip*> >::const_iterator iterDestination = iterOrigin->second.begin(); iterDestination != iterOrigin->second.end(); ++iterDestination)
            {
                const std::vector<SymuCore::ValuetedPath*> & resultForDestination = resultForOrigin.at(iterDestination->first);

                if (resultForDestination.empty())
                {
                    BOOST_LOG_TRIVIAL(error) << "No path available from origin '" << iterOrigin->first->getStrNodeName()  << "' to destination '" << iterDestination->first->getStrNodeName()
                        << "' for subpopulation '" << iterSubPop->first->GetStrName() << "' !";
                    return false;
                }
                else
                {
                    const SymuCore::Path & path = resultForDestination.front()->GetPath();
                    for (size_t i = 0; i < iterDestination->second.size(); i++)
                    {
                        for (int iInstance = 0; iInstance < nbInstances; iInstance++)
                        {
                            iterDestination->second.at(i)->SetPath(iInstance, path);
                        }
                    }
                }
            }

            // cleanup
            for (std::map<SymuCore::Destination*, std::vector<SymuCore::ValuetedPath*> >::const_iterator iterDest = resultForOrigin.begin(); iterDest != resultForOrigin.end(); ++iterDest)
            {
                for (size_t iPath = 0; iPath < iterDest->second.size(); iPath++)
                {
                    delete iterDest->second.at(iPath);
                }
            }
        }
    }
    
    return true;
}

void SimulationRunner::SetAssignmentModel(AssignmentModel *pAssignmentModel)
{
    m_pAssignmentModel = pAssignmentModel;
}

bool SimulationRunner::ForbidSymuviaLinks(const std::vector<std::string> & linkNames)
{
	bool bOk = true;
	for (size_t iSim = 0; iSim < m_Simulators.GetSimulators().size() && bOk; iSim++)
	{
		TraficSimulatorHandler * pSimulator = m_Simulators.GetSimulators()[iSim];
		if (pSimulator->GetSimulatorConfiguration()->GetSimulatorName() == ST_SymuVia)
		{
			for (size_t iInstance = 0; iInstance < pSimulator->GetSimulatorInstances().size() && bOk; iInstance++)
			{
				bOk = static_cast<SymuViaInstance*>(pSimulator->GetSimulatorInstances()[iInstance])->ForbidLinks(linkNames);
			}
		}
	}
	return bOk;
}



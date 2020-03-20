#include "stdafx.h"
#include "TrafficState.h"

using namespace std;
using namespace eveShared;

void SymuviaNetwork::Empty(SymuviaNetwork * pTS)
{
    map<std::string,Troncon*>::iterator iter;
    for(iter = pTS->troncons.begin(); iter != pTS->troncons.end(); iter++)
    {
        list<LevelCrossing*>::iterator lvlIter;
        for(lvlIter = iter->second->LevelCrossings.begin(); lvlIter != iter->second->LevelCrossings.end(); lvlIter++)
        {
            delete *lvlIter;
        }
        iter->second->LevelCrossings.clear();

        for(size_t iPt = 0; iPt < iter->second->point_internes.size(); iPt++)
        {
            delete iter->second->point_internes[iPt];
        }
        iter->second->point_internes.clear();

        delete iter->second;
    }
    pTS->troncons.clear();
}

SymuviaNetwork::~SymuviaNetwork()
{
	Empty(this);
}

void TrafficState::Empty(TrafficState * pTS)
{
	pTS->inst = 0.0f;
	for( std::list<Segment*>::iterator itSeg = pTS->Segments.begin(); pTS->Segments.end()!= itSeg ; itSeg++)
	{ 
		delete (*itSeg); 
	} 
	pTS->Segments.clear();
	for( std::list<TrafficLight*>::iterator itLight = pTS->TrafficLights.begin(); pTS->TrafficLights.end()!= itLight ; itLight++)
	{ 
		delete (*itLight); 
	} 
	pTS->TrafficLights.clear();
	for( std::list<Cell*>::iterator itCell = pTS->Cells.begin(); pTS->Cells.end()!= itCell ; itCell++)
	{ 
		delete (*itCell); 
	} 
	pTS->Cells.clear();
	for( std::list<Source*>::iterator itSource = pTS->Sources.begin(); pTS->Sources.end()!= itSource ; itSource++)
	{ 
		delete (*itSource); 
	} 
	pTS->Sources.clear();
	for( std::list<Sensor*>::iterator itSensor = pTS->Sensors.begin(); pTS->Sensors.end()!= itSensor ; itSensor++)
	{ 
		delete (*itSensor); 
	} 
	pTS->Sensors.clear();
	for( std::map<int,Trajectory*>::iterator itTraj = pTS->Trajectories.begin(); pTS->Trajectories.end()!= itTraj ; itTraj++)
	{ 
		delete (itTraj)->second; 
	} 
    pTS->Trajectories.clear();

    for( std::map<int, Stream*>::iterator itStream = pTS->Streams.begin(); itStream != pTS->Streams.end(); pTS++)
    {
        delete (itStream)->second;
    }
    pTS->Streams.clear();

}

TrafficState::~TrafficState()
{
	Empty(this);
}

void TrafficState::Copy(const TrafficState * pTSSrc, TrafficState * pTSDest)
{
	pTSDest->inst = pTSSrc->inst;
	for( std::list<Segment*>::const_iterator itSeg = pTSSrc->Segments.begin(); pTSSrc->Segments.end()!= itSeg ; itSeg++)
	{ 
		Segment * pSeg = new Segment();
		*pSeg = *(*itSeg);
		pTSDest->Segments.push_back(pSeg); 
	} 
	for( std::list<TrafficLight*>::const_iterator itLight = pTSSrc->TrafficLights.begin(); pTSSrc->TrafficLights.end()!= itLight ; itLight++)
	{ 
		TrafficLight * pTL = new TrafficLight();
		*pTL = *(*itLight);
		pTSDest->TrafficLights.push_back(pTL); 
	} 
	for( std::list<Cell*>::const_iterator itCell = pTSSrc->Cells.begin(); pTSSrc->Cells.end()!= itCell ; itCell++)
	{ 
		Cell * pCell = new Cell();
		*pCell = *(*itCell);
		pTSDest->Cells.push_back(pCell); 
	} 
	for( std::list<Source*>::const_iterator itSource = pTSSrc->Sources.begin(); pTSSrc->Sources.end()!= itSource ; itSource++)
	{ 
		Source * pSrc = new Source;
		*pSrc = *(*itSource);
		pTSDest->Sources.push_back(pSrc); 
	} 
	for( std::list<Sensor*>::const_iterator itSensor = pTSSrc->Sensors.begin(); pTSSrc->Sensors.end()!= itSensor ; itSensor++)
	{ 
		Sensor * pSensor = new Sensor();
		*pSensor = *(*itSensor);
		pTSDest->Sensors.push_back(pSensor);
	} 
	for( std::map<int,Trajectory*>::const_iterator itTraj = pTSSrc->Trajectories.begin(); pTSSrc->Trajectories.end()!= itTraj ; itTraj++)
	{ 
		Trajectory * pTraj = new Trajectory();
		*pTraj = *(itTraj)->second;
		pTSDest->Trajectories[itTraj->first] = pTraj;
	}

    for( std::map<int,Stream*>::const_iterator itStream = pTSSrc->Streams.begin(); pTSSrc->Streams.end()!= itStream ; itStream++)
	{ 
		Stream * pStream = new Stream();
		*pStream = *(itStream)->second;
		pTSDest->Streams[itStream->first] = pStream;
	}
}

TrafficState * TrafficState::Clone()
{
	TrafficState * pTS = new TrafficState();
	Copy(this, pTS);
	return pTS;
}

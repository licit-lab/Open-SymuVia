#include "stdafx.h"
#include "GTFSExporter.h"

#include "reseau.h"
#include "tuyau.h"
#include "arret.h"
#include "usage/PublicTransportFleet.h"
#include "usage/Trip.h"
#include "usage/TripLeg.h"
#include "usage/Schedule.h"
#include "usage/PublicTransportScheduleParameters.h"

#include <iostream>

#include "SystemUtil.h"

#define SYMUVIA_GTFS_SERVICE "SymuViaService"

using namespace std;

GTFSExporter::GTFSExporter()
{
}

GTFSExporter::~GTFSExporter()
{
}

bool GTFSExporter::write(Reseau * pNetwork, const std::map<std::string, std::pair<int,int> > & lstTPTypes)
{
    bool bOk = true;

    std::string outDir = pNetwork->GetOutputDir();
    writeAgency(outDir);
    bOk = bOk && writeStops(outDir, pNetwork);
    writeCalendar(outDir);
    writeCalendarDates(outDir);
    writeRoutes(outDir, pNetwork, lstTPTypes);
    writeTrips(outDir, pNetwork);
    writeStopTimes(outDir, pNetwork);
    writeFrequencies(outDir, pNetwork);

    return bOk;
}

// ecriture du fichier agency.txt
void GTFSExporter::writeAgency(const std::string & outDir)
{
    const std::string & sFile(outDir+"/agency.txt");
    std::ofstream ofs(sFile.c_str(), std::ios::out);
    
    ofs.setf(std::ios::fixed);
    ofs << "agency_id,agency_name,agency_url,agency_timezone,agency_lang" << std::endl;
    ofs << "\"1\",\"TCL\",\"http://www.tcl.fr\",\"Europe/Paris\",fr" << std::endl;
    ofs.close();
}

// �criture du fichier stops.txt
bool GTFSExporter::writeStops(const std::string & outDir, Reseau * pNetwork)
{
    const std::string & sFile(outDir+"/stops.txt");
    std::ofstream ofs(sFile.c_str(), std::ios::out);

    ofs.setf(std::ios::fixed);
    ofs << "stop_id,stop_name,stop_desc,stop_lat,stop_lon,zone_id,stop_url,location_type,parent_station" << std::endl;

    // Le format GTFS attend des coordonn�es en lat long WGS84.
    // On suppose ici que le syst�me d'entr�e est Lambert93 (2154), mais il fvaudrait mieux se baser sur le champ EPSGinput du fichier XML d'entr�e ? le probl�me est qu'il
    // n'est souvent pas renseign�...
    OGRErr ogrErrI, ogErrO;

  	OGRSpatialReference* inputRef = new OGRSpatialReference();
	ogrErrI = inputRef->importFromEPSG(2154);

	OGRSpatialReference* outputRef = new OGRSpatialReference();
	ogErrO = outputRef->importFromEPSG(4326);	

	OGRCoordinateTransformation * pCoordTransf = NULL;
	if( ogrErrI==0 &&  ogErrO==0)
		pCoordTransf = OGRCreateCoordinateTransformation( inputRef, outputRef );

	if(!pCoordTransf)
    {
        cerr << "Unable to create the requested OGR transformation..." << endl;
        ofs.close();
	    return false;
    }

    PublicTransportFleet * publicTransportFleet = pNetwork->GetPublicTransportFleet();
    const std::vector<TripNode*> & stops = publicTransportFleet->GetTripNodes();
    for(size_t iStop = 0; iStop < stops.size(); iStop++)
    {
        TripNode * pStop = stops[iStop];

        Point pt;
        CalcCoordFromPos(pStop->GetInputPosition().GetLink(), pStop->GetInputPosition().GetPosition(), pt);

        pCoordTransf->Transform(1, &pt.dbX, &pt.dbY); 

        ofs << pStop->GetID() << ",\"" << pStop->GetID() << "\",\"\"," << pt.dbY << "," << pt.dbX << ",\"\",\"\",0,0" << std::endl;
    }
    ofs.close();

    delete pCoordTransf;

    return true;
}

// ecriture du fichier calendar.txt
void GTFSExporter::writeCalendar(const std::string & outDir)
{
    const std::string & sFile(outDir+"/calendar.txt");
    std::ofstream ofs(sFile.c_str(), std::ios::out);
    
    ofs.setf(std::ios::fixed);
    ofs << "service_id,monday,tuesday,wednesday,thursday,friday,saturday,sunday,start_date,end_date" << std::endl;
    // rmq. : les champs date de d�but et de fin sont optionnels dans le standard, mais tempus veut des valeurs. Du coup on met une plage bidon tr�s large.
    // Peut-�tre faudrait-il mettre uniquement la date de la simulation (le probl�me est que cette date est facultative dans le format SymuVia...) ?
    ofs << SYMUVIA_GTFS_SERVICE << ",1,1,1,1,1,1,1,20000101,20200101" << std::endl;
    ofs.close();
}


// ecriture du fichier calendar_dates.txt (a priori optionnel mais le loader tempus le consid�re obligatoire ...)
void GTFSExporter::writeCalendarDates(const std::string & outDir)
{
    const std::string & sFile(outDir+"/calendar_dates.txt");
    std::ofstream ofs(sFile.c_str(), std::ios::out);
    
    ofs.setf(std::ios::fixed);
    ofs << "service_id,date,exception_type" << std::endl;
    ofs.close();
}

// �criture du fichier routes.txt
void GTFSExporter::writeRoutes(const std::string & outDir, Reseau * pNetwork, const std::map<std::string, std::pair<int,int> > & lstTPTypes)
{
    const std::string & sFile(outDir+"/routes.txt");
    std::ofstream ofs(sFile.c_str(), std::ios::out);
    
    ofs.setf(std::ios::fixed);
    ofs << "route_id,route_short_name,route_long_name,route_desc,route_type" << std::endl;
    for(size_t iLine = 0; iLine < pNetwork->GetPublicTransportFleet()->GetTrips().size(); iLine++)
    {
        Trip * pLine = pNetwork->GetPublicTransportFleet()->GetTrips()[iLine];

        ofs << pLine->GetID() << "," << pLine->GetID() << "," << pLine->GetID() << ",," << lstTPTypes.at(pNetwork->GetPublicTransportFleet()->GetTypeVehicule(pLine)->GetLabel()).second << std::endl;
    }
    ofs.close();
}

// �criture du fichier trips.txt
void GTFSExporter::writeTrips(const std::string & outDir, Reseau * pNetwork)
{
    const std::string & sFile(outDir+"/trips.txt");
    std::ofstream ofs(sFile.c_str(), std::ios::out);
    
    ofs.setf(std::ios::fixed);
    ofs << "route_id,service_id,trip_id,trip_headsign,direction_id,block_id,shape_id" << std::endl;
    for(size_t iLine = 0; iLine < pNetwork->GetPublicTransportFleet()->GetTrips().size(); iLine++)
    {
        Trip * pLine = pNetwork->GetPublicTransportFleet()->GetTrips()[iLine];

        ofs << pLine->GetID() << "," << SYMUVIA_GTFS_SERVICE << "," << pLine->GetID() << ",,,," << std::endl;
    }
    ofs.close();
}

// �criture du fichier stop_times.txt
void GTFSExporter::writeStopTimes(const std::string & outDir, Reseau * pNetwork)
{
    STime tSimulationStart = pNetwork->GetSimuStartTime();

    const std::string & sFile(outDir+"/stop_times.txt");
    std::ofstream ofs(sFile.c_str(), std::ios::out);
    
    ofs.setf(std::ios::fixed);
    ofs << "trip_id,arrival_time,departure_time,stop_id,stop_sequence,pickup_type,drop_off_type" << std::endl;
    const std::vector<Trip*> & lstLines = pNetwork->GetPublicTransportFleet()->GetTrips();
    for(size_t iLine = 0; iLine < lstLines.size(); iLine++)
    {
        Trip * pLine = lstLines[iLine];

        // Calcul du temps de parcours � vide entre chaque arr�t pour avoir une estimation des horaires de bus � tous les arr�ts de la ligne
        std::vector<double> dbDriveTimesBetweenStops;
        TripNode * pLastStop = pLine->GetOrigin();
        for(size_t iLeg = 0; iLeg < pLine->GetLegs().size(); iLeg++)
        {
            TripLeg * pLeg = pLine->GetLegs()[iLeg];

            double dbLegTime = 0;
            for(size_t iLink = 0; iLink < pLeg->GetPath().size(); iLink++)
            {
                Tuyau * pLink = pLeg->GetPath()[iLink];
                // Vitesse sur le tron�on
                double dbSpeedOnLink = pNetwork->GetPublicTransportFleet()->GetTypeVehicule(pLine)->GetVx();
                if(pLink->GetVitReg() != DBL_MAX)
                {
                    dbSpeedOnLink = std::min<double>(pLink->GetVitReg(), dbSpeedOnLink);
                }

                double dbLengthOnLink = pLink->GetLength();
                if(iLink == 0)
                {
                    dbLengthOnLink -= pLastStop->GetOutputPosition().GetPosition();
                }
                if(iLink == pLeg->GetPath().size()-1)
                {
                    dbLengthOnLink -= (pLink->GetLength() - pLeg->GetDestination()->GetInputPosition().GetPosition());
                }

                dbLegTime += dbLengthOnLink / dbSpeedOnLink;
            }

            dbDriveTimesBetweenStops.push_back(dbLegTime);

            pLastStop = pLeg->GetDestination();
        }

        Schedule * pLineSchedule = pNetwork->GetPublicTransportFleet()->GetSchedule(pLine);

        // Constitution de la liste des instants de d�part
        std::vector<double> lstDepartureTimes;
        std::vector<ScheduleElement*> lstDepartureScheduleElement;
        ScheduleFrequency * pFirstFrequency = NULL;
        for(size_t iElem = 0; iElem < pLineSchedule->GetLstElements().size(); iElem++)
        {
            ScheduleElement * pElem = pLineSchedule->GetLstElements()[iElem];
            if(dynamic_cast<ScheduleFrequency*>(pElem))
            {
                // On ignore les fr�quences (mais on garde la premi�re pour ajouter l'instant de d�marrage s'il n'y a que des fr�quences dans le calendrier)
                pFirstFrequency = (ScheduleFrequency*)pElem;
            }
            else
            {
                lstDepartureTimes.push_back(pElem->GetStartTime());
                lstDepartureScheduleElement.push_back(pElem);
            }
        }
        // Si que des fr�quences, on met l'instant de d�marrage de la premi�re fr�quence
        if(lstDepartureTimes.empty() && pFirstFrequency)
        {
            lstDepartureTimes.push_back(pFirstFrequency->GetStartTime());
            lstDepartureScheduleElement.push_back(pFirstFrequency);
        }

        // Pour chaque horaire de d�part
        for(size_t iDeparture = 0; iDeparture < lstDepartureTimes.size(); iDeparture++)
        {
            STimeSpan ts(0, 0, 0, (int)(lstDepartureTimes[iDeparture]+0.5));
            STimeSpan::RegulariseSomme(ts);
            STime tDepartureTime = tSimulationStart + ts;
            STime stopTime = tDepartureTime;

            // Heure de d�part
            // Remarque : on ignore cette ligne s'il s'agit d'une heure de d�part � une entr�e et pas au premier arr�t (puisque la colonne stop_id doit correspondre � un nom d'arr�t)
            if(dynamic_cast<Arret*>(pLine->GetOrigin()))
            {
                ofs << pLine->GetID() << "," << tDepartureTime.ToString() << "," << tDepartureTime.ToString() << "," << pLine->GetOrigin()->GetID() << ",1,0,0" << std::endl;
            }

            for(size_t iLeg = 0; iLeg < pLine->GetLegs().size(); iLeg++)
            {
                TripLeg * pLeg = pLine->GetLegs()[iLeg];

                Arret * pStop = dynamic_cast<Arret*>(pLeg->GetDestination());
                if(pStop)
                {
                    double dbStopTime = pStop->getTempsArret();

                    // Si une dur�e d'arr�t est d�finie en particulier pour ce d�part, on l'utilise � la place
                    std::map<ScheduleElement*, ScheduleParameters*>::const_iterator iterParams = pNetwork->GetPublicTransportFleet()->GetScheduleParameters().find(lstDepartureScheduleElement[iDeparture]);
                    if(iterParams != pNetwork->GetPublicTransportFleet()->GetScheduleParameters().end())
                    {
                        PublicTransportScheduleParameters* pParams = (PublicTransportScheduleParameters*)iterParams->second;
                        std::map<Arret*, double>::const_iterator iter = pParams->GetStopTimes().find(pStop);
                        if(iter != pParams->GetStopTimes().end())
                        {
                            dbStopTime = iter->second;
                        }
                    }

                    // Prise en compte de la dur�e de d�placement
                    STimeSpan ts2(0, 0, 0, (int)(dbDriveTimesBetweenStops[iLeg]+0.5));
                    STimeSpan::RegulariseSomme(ts2);
                    stopTime = tDepartureTime + ts2;

                    // Dans le cas particulier du dernier arr�t; on ne compte pas le temps d'arr�t
                    if(iLeg == pLine->GetLegs().size()-1)
                    {
                        tDepartureTime = stopTime;
                    }
                    else
                    {
                        // Prise en compte de la dur�e de l'arr�t
                        ts2 = STimeSpan(0, 0, 0, (int)(dbStopTime+0.5));
                        STimeSpan::RegulariseSomme(ts2);
                        tDepartureTime = stopTime + ts2;
                    }

                    ofs << pLine->GetID() << "," << stopTime.ToString() << "," << tDepartureTime.ToString() << "," << pLeg->GetDestination()->GetID() << "," << (iLeg+2) << ",0,0" << std::endl;
                }
            }
        }
    }
    ofs.close();
}

// �criture du fichier frequencies.txt
void GTFSExporter::writeFrequencies(const std::string & outDir, Reseau * pNetwork)
{
    bool bHasFrequencies = false;
    for(size_t iLine = 0; iLine < pNetwork->GetPublicTransportFleet()->GetTrips().size() && !bHasFrequencies; iLine++)
    {
        Trip * pLine = pNetwork->GetPublicTransportFleet()->GetTrips()[iLine];
        Schedule * pSchedule = pNetwork->GetPublicTransportFleet()->GetSchedule(pLine);

        for(size_t iElem = 0; iElem < pSchedule->GetLstElements().size() && !bHasFrequencies; iElem++)
        {
            ScheduleFrequency * pFreq = dynamic_cast<ScheduleFrequency*>(pSchedule->GetLstElements()[iElem]);
            if(pFreq)
            {
                bHasFrequencies = true;
            }
        }
    }

    if(bHasFrequencies)
    {
        STime tSimulationStart = pNetwork->GetSimuStartTime();
        
        const std::string & sFile(outDir+"/frequencies.txt");
        std::ofstream ofs(sFile.c_str(), std::ios::out);
        
        ofs.setf(std::ios::fixed);
        ofs << "trip_id,start_time,end_time,headway_secs" << std::endl;

        for(size_t iLine = 0; iLine < pNetwork->GetPublicTransportFleet()->GetTrips().size(); iLine++)
        {
            Trip * pLine = pNetwork->GetPublicTransportFleet()->GetTrips()[iLine];
            Schedule * pSchedule = pNetwork->GetPublicTransportFleet()->GetSchedule(pLine);

            for(size_t iElem = 0; iElem < pSchedule->GetLstElements().size(); iElem++)
            {
                ScheduleFrequency * pFreq = dynamic_cast<ScheduleFrequency*>(pSchedule->GetLstElements()[iElem]);
                if(pFreq)
                {
                    STimeSpan tsDebut(0, 0, 0, (int)(pFreq->GetStartTime()+0.5));
                    STimeSpan::RegulariseSomme(tsDebut);
                    STime tDebut = tSimulationStart + tsDebut;
                    STimeSpan tsFin(0, 0, 0, (int)(pFreq->GetStopTime()+0.5));
                    STimeSpan::RegulariseSomme(tsFin);
                    STime tFin = tSimulationStart + tsFin;

                    ofs << pLine->GetID() << "," << tDebut.ToString() << "," << tFin.ToString() << "," << (int)(pFreq->GetFrequency()+0.5) << std::endl;
                }
            }
        }
        ofs.close();
    }
}

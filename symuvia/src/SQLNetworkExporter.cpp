#include "stdafx.h"
#include "SQLNetworkExporter.h"

#include "SystemUtil.h"
#include "reseau.h"
#include "tuyau.h"
#include "repartiteur.h"
#include "usage/PublicTransportFleet.h"
#include "usage/Trip.h"

#include <iostream>
#include <algorithm>

// To fix an incompatibility of boost.Python headers and locale header on Mac OS X
#ifdef __APPLE__
#undef tolower
#endif // __APPLE__

SQLNetworkExporter::SQLNetworkExporter()
{
}

SQLNetworkExporter::~SQLNetworkExporter()
{
}

bool lower_test (char l, char r) {
  return (std::tolower(l, std::locale()) == std::tolower(r, std::locale()));
}

bool SQLNetworkExporter::writeSQL(Reseau * pNetwork)
{
    bool bOk = true;

    // Construction du chemin de sortie
    std::ofstream ofs((pNetwork->GetPrefixOutputFiles() + "_tempus.sql").c_str(), std::ios::out);
    ofs.setf(std::ios::fixed);

    ofs << "set schema 'tempus';" << std::endl << std::endl;

    // Nettoyage des tables
    ofs << "DELETE FROM poi;" << std::endl;
    ofs << "DELETE FROM road_restriction;" << std::endl;
    ofs << "DELETE FROM road_restriction_time_Penalty;" << std::endl;
    ofs << "DELETE FROM road_restriction_toll;" << std::endl;
    ofs << "DELETE FROM road_daily_profile;" << std::endl;
    ofs << "DELETE FROM road_section_speed;" << std::endl;
    ofs << "DELETE FROM road_validity_period;" << std::endl;
    ofs << "DELETE FROM pt_stop;" << std::endl;
    ofs << "DELETE FROM road_section;" << std::endl;
    ofs << "DELETE FROM road_node;" << std::endl << std::endl;
    ofs << "DELETE FROM view_section_by_network;" << std::endl;
    ofs << "DELETE FROM view_stop_by_network;" << std::endl;
    ofs << "DELETE FROM pt_frequency;" << std::endl;
    ofs << "DELETE FROM pt_trip;" << std::endl;
    ofs << "DELETE FROM pt_agency;" << std::endl;
    ofs << "DELETE FROM pt_calendar;" << std::endl;
    ofs << "DELETE FROM pt_calendar_date;" << std::endl;
    ofs << "DELETE FROM pt_fare_attribute;" << std::endl;
    ofs << "DELETE FROM pt_fare_rule;" << std::endl;
    ofs << "DELETE FROM pt_network;" << std::endl;
    ofs << "DELETE FROM pt_route;" << std::endl;
    ofs << "DELETE FROM pt_section;" << std::endl;
    ofs << "DELETE FROM pt_service;" << std::endl;
    ofs << "DELETE FROM pt_stop_time;" << std::endl;
    ofs << "DELETE FROM pt_zone;" << std::endl;

    // rmq. on garde les quatre premiers types (ils sont en dur dans TEMPUS donc pas le choix, ils sont réservés).
    // On a ensuite un numéro à partir de 5 pour chaque type de transport en commun
    ofs << "DELETE FROM transport_mode WHERE id > 4;" << std::endl << std::endl;

    // Définition de la plage temporelle globale de la simulation (nécessaire pour les mouvements interdits)
    ofs << "INSERT INTO road_validity_period (id,name,monday,tuesday,wednesday,thursday,friday,saturday,sunday,bank_holiday,day_before_bank_holiday,holidays,day_before_holidays) "
        << "VALUES (0,'Always',TRUE,TRUE,TRUE,TRUE,TRUE,TRUE,TRUE,TRUE,TRUE,TRUE,TRUE);" << std::endl << std::endl;

    // Recherche des types de transports guidés
    m_LstTPTypes.clear();
    static const std::string tramName = "tram";
    static const std::string taxiName = "taxi";
    int iTransportId = 5;
    for(size_t iLine = 0; iLine < pNetwork->GetPublicTransportFleet()->GetTrips().size(); iLine++)
    {
        Trip * pLine = pNetwork->GetPublicTransportFleet()->GetTrips()[iLine];
        std::string tpName = pNetwork->GetPublicTransportFleet()->GetTypeVehicule(pLine)->GetLabel();

        int tpType = 3; // type 3 = bus
        
        // Par défaut, on met bus. Si on trouve tram dans le nom (en ignorant la casse), on met tram
        std::string::iterator fpos = std::search(tpName.begin(), tpName.end(), tramName.begin(), tramName.end(), lower_test);
        if (fpos != tpName.end())
        {
            tpType = 0; // type 0 = tramway
        }

        if(m_LstTPTypes.find(tpName) == m_LstTPTypes.end())
        {
            m_LstTPTypes[tpName] = std::make_pair(iTransportId, tpType);
            // rmq. : visiblement la creation de cette ligne a lieu a l'import des données GTFS : pas la peine de le faire !
            //ofs << "INSERT INTO transport_mode (id,name,public_transport,gtfs_route_type) VALUES (" << iTransportId << ", '" << tpName << "', TRUE, " << tpType << ");" << std::endl;
            iTransportId++;
        }
    }

    ofs << std::endl;

    // Ecriture des nodes
    std::map<std::string, int> mapNodeIDs;
    const std::deque<Tuyau*> & lstTuyaux = pNetwork->m_LstTuyaux;
    int iNode = 1;
    for(size_t i = 0; i < lstTuyaux.size(); i++)
    {
	    Tuyau * pTuy = lstTuyaux[i];

        // rmq. : on ne sort pas les tronçons internes
        if( !pTuy->GetBriqueParente() && pTuy->GetCnxAssAm() && pTuy->GetCnxAssAv() )
	    {
            // Ajout du noeud amont si pas déjà traité
            if(mapNodeIDs.find(pTuy->GetCnxAssAm()->GetID()) == mapNodeIDs.end())
            {
                ofs << "INSERT INTO road_node VALUES (" << iNode << ", FALSE, public.ST_GeomFromText('" << GetWKTFromConnection(pTuy->GetCnxAssAm(), pNetwork) << "',2154));" << std::endl;
                mapNodeIDs[pTuy->GetCnxAssAm()->GetID()] = iNode;
                iNode++;
            }

            // Ajout du noeud aval si pas déjà traité
            if(mapNodeIDs.find(pTuy->GetCnxAssAv()->GetID()) == mapNodeIDs.end())
            {
                ofs << "INSERT INTO road_node VALUES (" << iNode << ", FALSE, public.ST_GeomFromText('" << GetWKTFromConnection(pTuy->GetCnxAssAv(), pNetwork) << "',2154));" << std::endl;
                mapNodeIDs[pTuy->GetCnxAssAv()->GetID()] = iNode;
                iNode++;
            }
        }
    }

    ofs << std::endl << std::endl;

    // Ecriture des sections

    int iLink = 1;
    std::map<std::string,int> mapLinks;
    for(size_t i = 0; i < lstTuyaux.size(); i++)
    {
	    Tuyau * pTuy = lstTuyaux[i];

        // rmq. : on ne sort pas les tronçons internes
        if( !pTuy->GetBriqueParente() && pTuy->GetCnxAssAm() && pTuy->GetCnxAssAv() )
	    {
		    size_t nbPoints = pTuy->GetLstPtsInternes().size()+2;
            std::vector<Point> lstPoints(nbPoints);

	        // coordonnées des points
	        for(size_t iPoint = 0; iPoint < nbPoints; iPoint++)
	        {
		        Point * pPoint;
		        if(iPoint == 0)
		        {
			        pPoint = pTuy->GetExtAmont();
		        }
		        else if(iPoint == nbPoints-1)
		        {
			        pPoint = pTuy->GetExtAval();
		        }
		        else
		        {
			        pPoint = pTuy->GetLstPtsInternes()[iPoint-1];
		        }

		        if(pNetwork->m_pCoordTransf)
			        pNetwork->m_pCoordTransf->Transform(1, &pPoint->dbX, &pPoint->dbY); 

                lstPoints[iPoint] = *pPoint;
	        }

            int trafficRules = 1 + 2; // de base, les piétons et vélos peuvent emprunter tous les tronçons ...
            // Pour chaque type de véhicule existant ...
            bool bDrivingsAuthorized = false;
            bool bTaxiAuthorized = false;
            for(size_t iTV = 0; iTV < pNetwork->m_LstTypesVehicule.size(); iTV++)
            {
                TypeVehicule * pTV = pNetwork->m_LstTypesVehicule[iTV];
                if(!pTuy->IsInterdit(pTV))
                {
                    if(m_LstTPTypes.find(pTV->GetLabel()) != m_LstTPTypes.end())
                    {
                        // Le mode de transport en commun associé est autorisé à circuler sur le tronçon
                        trafficRules += (int)pow(2.0, m_LstTPTypes[pTV->GetLabel()].first-1);
                    }
                    else
                    {
                        std::string strTypeVeh = pTV->GetLabel();
                        std::string::iterator fpos = std::search(strTypeVeh.begin(), strTypeVeh.end(), taxiName.begin(), taxiName.end(), lower_test);
                        if (fpos != strTypeVeh.end())
                        {
                            if(!bTaxiAuthorized)
                            {
                                trafficRules += 8;
                                bTaxiAuthorized = true;
                            }
                        }
                        else
                        {
                            if(!bDrivingsAuthorized)
                            {
                                trafficRules += 4;
                                bDrivingsAuthorized = true;
                            }
                        }
                    }
                }
            }
            
            // OTK - TOTO - NEXT - vérifier qu'on peut mettre des NULL aux vitesses réglementaires ?? si on met nul de partout, ca ne devrait pas empecher le calcul des itinéraires de se faire, si ?
            ofs << "INSERT INTO road_section (id, road_type, node_from, node_to, traffic_rules_ft, traffic_rules_tf, length, car_speed_limit, road_name, lane, roundabout, bridge, tunnel, ramp, tollway, geom) "
                << "VALUES (" << iLink << ", 5, " << mapNodeIDs[pTuy->GetCnxAssAm()->GetID()] << ", " << mapNodeIDs[pTuy->GetCnxAssAv()->GetID()] << ", " << trafficRules << ", 0, " << pTuy->GetLength() << ", ";
            if(pTuy->GetVitReg() >= DBL_MAX)
            {
                // si on laisse null pour la vitesse reglementaire, vu le code de tempus, les tronçons seront considérés comme interdits (ou comportement indéfini ?)
                // Du coup, on met la vitesse max en fonction des types de véhicules définis.
                ofs << pNetwork->GetMaxVitMax()*3.6;
            }
            else
            {
                ofs << (pTuy->GetVitReg()*3.6);
            }
            ofs << ", '" << pTuy->GetLabel() << "', " << pTuy->getNb_voies() << ",FALSE,FALSE,FALSE,FALSE,FALSE," << "public.ST_GeomFromText('" << GetWKTFromPoints(lstPoints) << "',2154));" << std::endl;

            mapLinks[pTuy->GetLabel()] = iLink;
            iLink++;
	    }
    }


    // Gestion des mouvements interdits
    int iRoadRestriction = 1;
    for(size_t i = 0; i < lstTuyaux.size(); i++)
    {
	    Tuyau * pTuy = lstTuyaux[i];

        // rmq. : on ne sort pas les tronçons internes
        if( !pTuy->GetBriqueParente() && pTuy->GetCnxAssAm() && pTuy->GetCnxAssAv() )
	    {
            // test de chaque mouvement possible. Si le mouvement est interdit pour un type de véhicule ou un autre,
            // on ajoute la restriction correspondante
            for(size_t iTuyAval = 0; iTuyAval < pTuy->GetCnxAssAv()->m_LstTuyAssAv.size(); iTuyAval++)
            {
                Tuyau * pTuyAval = pTuy->GetCnxAssAv()->m_LstTuyAssAv[iTuyAval];

                // Calcul de la valeur de trafficRules correspondant au mouvement
                int nbAuthorizedVehicles = 4 + (int)m_LstTPTypes.size(); // par défaut, pietons et vélos
                int trafficRules = 0;

                bool bDrivingsRestricted = false;
                bool bTaxisRestricted = false;
                for(size_t iTV = 0; iTV < pNetwork->m_LstTypesVehicule.size(); iTV++)
                {
                    TypeVehicule * pTV = pNetwork->m_LstTypesVehicule[iTV];
                    if(!pTuy->GetCnxAssAv()->IsMouvementAutorise(pTuy, pTuyAval, pTV, NULL))
                    {
                        if(m_LstTPTypes.find(pTV->GetLabel()) != m_LstTPTypes.end())
                        {
                            // Le mode de transport en commun associé est autorisé à circuler sur le tronçon
                            trafficRules += (int)pow(2.0, m_LstTPTypes[pTV->GetLabel()].first-1);
                            nbAuthorizedVehicles--;
                        }
                        else
                        {
                            std::string strTypeVeh = pTV->GetLabel();
                            std::string::iterator fpos = std::search(strTypeVeh.begin(), strTypeVeh.end(), taxiName.begin(), taxiName.end(), lower_test);
                            if (fpos != strTypeVeh.end())
                            {
                                // Cas des taxis
                                if(!bTaxisRestricted)
                                {
                                    trafficRules += 8;
                                    bTaxisRestricted = true;
                                }
                            }
                            else
                            {
                                // Cas des private cars
                                if(!bDrivingsRestricted)
                                {
                                    trafficRules += 4;
                                    bDrivingsRestricted = true;
                                }

                            }
                        }
                    }
                }

                // S'il y a un mouvement interdit pour au moins un type de véhicule ...
                if(trafficRules != 0)
                {
                    ofs << "INSERT INTO road_restriction (id,sections) VALUES (" << iRoadRestriction << ", array[" << mapLinks[pTuy->GetLabel()] << "," << mapLinks[pTuyAval->GetLabel()] << "]);" << std::endl;
                    ofs << "INSERT INTO road_restriction_time_Penalty VALUES (" << iRoadRestriction << ", 0, " << trafficRules << ", '+infinity');" << std::endl;
                    iRoadRestriction++;
                }
            }
        }
    }

    ofs.close();

    return bOk;
}

const std::map<std::string, std::pair<int,int> > & SQLNetworkExporter::getTPTypes()
{
    return m_LstTPTypes;
}

std::string SQLNetworkExporter::GetWKTFromPoints(const std::vector<Point> & lstPoints)
{
    std::ostringstream oss;
    oss.setf(std::ios::fixed);

    oss << "LINESTRING(";
    for(size_t iPt = 0; iPt < lstPoints.size(); iPt++)
    {
        if(iPt > 0)
        {
            oss << ",";
        }
        oss << lstPoints[iPt].dbX << " " << lstPoints[iPt].dbY << " " << lstPoints[iPt].dbZ;
    }
    oss << ")";
    return oss.str();
}

std::string SQLNetworkExporter::GetWKTFromConnection(Connexion * pConnexion, Reseau * pNetwork)
{
    Point pt;
    BriqueDeConnexion* pBrique = dynamic_cast<BriqueDeConnexion*>(pConnexion);
    if(pBrique)
    {
        Point * ptTmp = pBrique->CalculBarycentre();
        pt = *ptTmp;
        delete ptTmp;
    }
    else
    {
        Repartiteur * pRep = dynamic_cast<Repartiteur*>(pConnexion);
        if(pRep)
        {
            pt.dbX = pRep->GetAbscisse();
            pt.dbY = pRep->GetOrdonnee();
            pt.dbZ = 0;

            if(pNetwork->m_pCoordTransf)
			    pNetwork->m_pCoordTransf->Transform(1, &pt.dbX, &pt.dbY); 
        }
        else if(pConnexion->m_LstTuyAssAm.size() == 1)
        {
            pt = *pConnexion->m_LstTuyAssAm.front()->GetExtAval();
        }
        else if(pConnexion->m_LstTuyAssAv.size() == 1)
        {
            pt = *pConnexion->m_LstTuyAssAv.front()->GetExtAmont();
        }
        else
        {
            // connexion sans aucun tuyau : impossible vu le code d'appel (on boucle sur les tuyaux et on regarde les connexions amont et aval)
            assert(false);
        }
    }
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss << "POINT(" << pt.dbX << " " << pt.dbY << " " << pt.dbZ << ")";
    return oss.str();
}

#include "stdafx.h"
#include "ParkingParameters.h"

#include "reseau.h"
#include "Logger.h"
#include "Xerces/XMLUtil.h"

#include <xercesc/dom/DOMNode.hpp>
#include <xercesc/dom/DOMNodeList.hpp>

XERCES_CPP_NAMESPACE_USE

ParkingParameters::ParkingParameters()
{
    m_bParkingManagement = false;
    m_bCreateVehiclesWithoutStock = false;
    m_nMaxParkingSpotsWhenNoMaximumParkingCapacity = -1;
}

ParkingParameters::~ParkingParameters()
{
}

bool ParkingParameters::DoCreateVehiclesWithoutParkingStock() const
{
    return m_bCreateVehiclesWithoutStock;
}

int ParkingParameters::GetMaxParkingSpotsWhenNoMaximumParkingCapacity() const
{
    return m_nMaxParkingSpotsWhenNoMaximumParkingCapacity;
}

bool ParkingParameters::LoadFromXML(Reseau * pNetwork, DOMNode * pParkingNode, ParkingParameters * pParentParameters, Logger * pLogger)
{
    if (pParentParameters == NULL)
    {
        // ces attributs ne sont présents que pour le noeud de paramètrage général (donc sans parent).
        GetXmlAttributeValue(pParkingNode, "gestion_stationnement", m_bParkingManagement, pLogger);
        GetXmlAttributeValue(pParkingNode, "creation_vehicules_sans_stock", m_bCreateVehiclesWithoutStock, pLogger);
        GetXmlAttributeValue(pParkingNode, "nb_places_max_par_parking", m_nMaxParkingSpotsWhenNoMaximumParkingCapacity, pLogger);
    }
    else
    {
        // du coup, dans le cas des enfants, on recopie la valeur du parent pour ne pas s'embêter à faire des tests à l'utilisation :
        m_bParkingManagement = pParentParameters->m_bParkingManagement;
        m_bCreateVehiclesWithoutStock = pParentParameters->m_bCreateVehiclesWithoutStock;
        m_nMaxParkingSpotsWhenNoMaximumParkingCapacity = pParentParameters->m_nMaxParkingSpotsWhenNoMaximumParkingCapacity;
    }

    // Lecture des variations temporelles des paramètres :
    double dbDuree;
    XMLSize_t countj = pParkingNode->getChildNodes()->getLength();
    for (XMLSize_t j = 0; j<countj; j++)
    {
        // Chargement de la variante temporelle des paramètres de stationnement
        DOMNode * xmlChildj = pParkingNode->getChildNodes()->item(j);
        if (xmlChildj->getNodeType() != DOMNode::ELEMENT_NODE) continue;

        std::vector<PlageTemporelle*> plages;
        if (!GetXmlDuree(xmlChildj, pNetwork, dbDuree, plages, pLogger))
        {
            return false;
        }

        boost::shared_ptr<ParkingParametersVariation> pParkingParamsVariation = boost::make_shared<ParkingParametersVariation>();
        pParkingParamsVariation->LoadFromXML(xmlChildj, m_bParkingManagement, pLogger);

        if (plages.size() > 0)
        {
            for (size_t iPlage = 0; iPlage < plages.size(); iPlage++)
            {
                m_LstVariations.AddVariation(plages[iPlage], pParkingParamsVariation);
            }
        }
        else
        {
            m_LstVariations.AddVariation(dbDuree, pParkingParamsVariation);
        }
    }

    // vérification de la couverture des plages temporelles
    if (!m_LstVariations.CheckPlagesTemporelles(pNetwork->GetDureeSimu()))
    {
        *pLogger << Logger::Error << "ERROR : The time frames defined for parking parameters don't cover the whole simulation duration !" << std::endl;
        *pLogger << Logger::Info; // rebascule en mode INFO pour ne pas avoir à reprendre tous les appels aux log en précisant que c'est des INFO. à supprimer si on reprend tous les appels au log.
        return false;
    }

    return true;
}

void ParkingParameters::AddVariation(boost::shared_ptr<ParkingParametersVariation> var, double dbDuree)
{
    m_LstVariations.AddVariation(dbDuree, var);
}

ParkingParametersVariation * ParkingParameters::GetVariation(double dbInstant)
{
    return m_LstVariations.GetVariationEx(dbInstant);
}

ParkingParametersVariation::ParkingParametersVariation()
{
    m_dbProportionResidentialOrigin = 1;
    m_dbProportionResidentialDestination = 1;
    m_dbProportionSurfacicOrigin = -1;
    m_dbProportionSurfacicDestination = -1;
    m_bUseCosts = false;
    m_dbSurfacicCost = -1;
    m_dbHSCost = -1;
    m_dbCostRatio = -1;
    m_dbMaxSurfacicSearchDuration = -1;
    m_dbReroutingPeriod = -1;
    m_dbWideningDelay = -1;
    m_bAutoGenerateParkingDemand = false;
    m_dbParkingMeanTime = -1;
    m_dbParkingTimeSigma = -1;
    m_bUseSimplifiedSurcaficMode = false;
    m_dbUpdateGoalDistancePeriod = -1;
    m_dbLengthBetweenTwoParkBlocks = -1;
    m_dbLengthBetweenTwoParkSpots = -1;
    m_dbNbOfSpotsPerBlock = -1;
}

ParkingParametersVariation::~ParkingParametersVariation()
{

}

double ParkingParametersVariation::GetResidentialRatioOrigin() const
{
    return m_dbProportionResidentialOrigin;
}

double ParkingParametersVariation::GetResidentialRatioDestination() const
{
    return m_dbProportionResidentialDestination;
}

double ParkingParametersVariation::GetSurfacicProportionOrigin() const
{
    return m_dbProportionSurfacicOrigin;
}

double ParkingParametersVariation::GetSurfacicProportionDestination() const
{
    return m_dbProportionSurfacicDestination;
}

bool ParkingParametersVariation::UseCosts() const
{
    return m_bUseCosts;
}

double ParkingParametersVariation::GetSurfacicCost() const
{
    return m_dbSurfacicCost;
}

double ParkingParametersVariation::GetHSCost() const
{
    return m_dbHSCost;
}

double ParkingParametersVariation::GetCostRatio() const
{
    return m_dbCostRatio;
}

double ParkingParametersVariation::GetMaxSurfacicSearchDuration() const
{
    return m_dbMaxSurfacicSearchDuration;
}

double ParkingParametersVariation::GetReroutingPeriod() const
{
    return m_dbReroutingPeriod;
}

double ParkingParametersVariation::GetWideningDelay() const
{
    return m_dbWideningDelay;
}

bool ParkingParametersVariation::AutoGenerateParkingDemand() const
{
    return m_bAutoGenerateParkingDemand;
}

double ParkingParametersVariation::GetParkingMeanTime() const
{
    return m_dbParkingMeanTime;
}

double ParkingParametersVariation::GetParkingTimeSigma() const
{
    return m_dbParkingTimeSigma;
}

bool ParkingParametersVariation::UseSimplifiedSurcaficMode() const
{
    return m_bUseSimplifiedSurcaficMode;
}

double ParkingParametersVariation::GetUpdateGoalDistancePeriod() const
{
    return m_dbUpdateGoalDistancePeriod;
}

double ParkingParametersVariation::GetLengthBetweenTwoParkBlocks() const
{
    return m_dbLengthBetweenTwoParkBlocks;
}

double ParkingParametersVariation::GetLengthBetweenTwoParkSpots() const
{
    return m_dbLengthBetweenTwoParkSpots;
}

double ParkingParametersVariation::GetNbOfSpotsPerBlock() const
{
    return m_dbNbOfSpotsPerBlock;
}



bool ParkingParametersVariation::LoadFromXML(XERCES_CPP_NAMESPACE::DOMNode * pParkingParametersNode, bool bGestionStationnement, Logger * pLogger)
{
    if (bGestionStationnement)
    {
        GetXmlAttributeValue(pParkingParametersNode, "proportion_stationnement_residentiel_origine", m_dbProportionResidentialOrigin, pLogger);
        GetXmlAttributeValue(pParkingParametersNode, "proportion_stationnement_residentiel_destination", m_dbProportionResidentialDestination, pLogger);
    }
    else
    {
        m_dbProportionResidentialOrigin = 1.0;
        m_dbProportionResidentialDestination = 1.0;
    }

    GetXmlAttributeValue(pParkingParametersNode, "proportion_stationnement_surfacique_origine", m_dbProportionSurfacicOrigin, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "proportion_stationnement_surfacique_destination", m_dbProportionSurfacicDestination, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "prise_en_compte_couts", m_bUseCosts, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "cout_stationnement_surfacique", m_dbSurfacicCost, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "cout_stationnement_parking", m_dbHSCost, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "cout_stationnement_ratio", m_dbCostRatio, pLogger);
    
    GetXmlAttributeValue(pParkingParametersNode, "duree_recherche_surfacique_max", m_dbMaxSurfacicSearchDuration, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "frequence_rearbitrage", m_dbReroutingPeriod, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "delai_elargissement_recherche", m_dbWideningDelay, pLogger);

    GetXmlAttributeValue(pParkingParametersNode, "generation_demande_stationnement", m_bAutoGenerateParkingDemand, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "duree_stationnement_moyenne", m_dbParkingMeanTime, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "duree_stationnement_ecart_type", m_dbParkingTimeSigma, pLogger);

    GetXmlAttributeValue(pParkingParametersNode, "mode_surfacique_simplifie", m_bUseSimplifiedSurcaficMode, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "frequence_maj_distance_recherche", m_dbUpdateGoalDistancePeriod, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "distance_entre_blocs", m_dbLengthBetweenTwoParkBlocks, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "distance_entre_places", m_dbLengthBetweenTwoParkSpots, pLogger);
    GetXmlAttributeValue(pParkingParametersNode, "nb_places_par_bloc", m_dbNbOfSpotsPerBlock, pLogger);

    return true;
}

template void ParkingParameters::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ParkingParameters::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ParkingParameters::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_bParkingManagement);
    ar & BOOST_SERIALIZATION_NVP(m_bCreateVehiclesWithoutStock);
    ar & BOOST_SERIALIZATION_NVP(m_nMaxParkingSpotsWhenNoMaximumParkingCapacity);
}

template void ParkingParametersVariation::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void ParkingParametersVariation::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void ParkingParametersVariation::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_dbProportionResidentialOrigin);
    ar & BOOST_SERIALIZATION_NVP(m_dbProportionResidentialDestination);
    ar & BOOST_SERIALIZATION_NVP(m_dbProportionSurfacicOrigin);
    ar & BOOST_SERIALIZATION_NVP(m_dbProportionSurfacicDestination);
    ar & BOOST_SERIALIZATION_NVP(m_bUseCosts);
    ar & BOOST_SERIALIZATION_NVP(m_dbSurfacicCost);
    ar & BOOST_SERIALIZATION_NVP(m_dbHSCost);
    ar & BOOST_SERIALIZATION_NVP(m_dbCostRatio);
    ar & BOOST_SERIALIZATION_NVP(m_dbMaxSurfacicSearchDuration);
    ar & BOOST_SERIALIZATION_NVP(m_dbReroutingPeriod);
    ar & BOOST_SERIALIZATION_NVP(m_dbWideningDelay);
    ar & BOOST_SERIALIZATION_NVP(m_bAutoGenerateParkingDemand);
    ar & BOOST_SERIALIZATION_NVP(m_dbParkingMeanTime);
    ar & BOOST_SERIALIZATION_NVP(m_dbParkingTimeSigma);
    ar & BOOST_SERIALIZATION_NVP(m_bUseSimplifiedSurcaficMode);
    ar & BOOST_SERIALIZATION_NVP(m_dbUpdateGoalDistancePeriod);
    ar & BOOST_SERIALIZATION_NVP(m_dbLengthBetweenTwoParkBlocks);
    ar & BOOST_SERIALIZATION_NVP(m_dbLengthBetweenTwoParkSpots);
    ar & BOOST_SERIALIZATION_NVP(m_dbNbOfSpotsPerBlock);
}


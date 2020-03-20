#ifndef _PARKING_PARAMETERS_H__
#define _PARKING_PARAMETERS_H__

#include "tools.h"

class ParkingParametersVariation;

//! Classe définissant un ensemble de paramètres pour le stationnement en zone
class ParkingParameters
{
public:
    //! Default constructor
    ParkingParameters(void);
    //! Destructor
    virtual ~ParkingParameters(void);

    bool DoCreateVehiclesWithoutParkingStock() const;
    int GetMaxParkingSpotsWhenNoMaximumParkingCapacity() const;

    // Chargement à partir d'un noeud XML
    bool LoadFromXML(Reseau * pNetwork, XERCES_CPP_NAMESPACE::DOMNode * pParkingParametersNode, ParkingParameters * pParentParameters, Logger * pLogger);

    void AddVariation(boost::shared_ptr<ParkingParametersVariation> var, double dbDuree);
    ParkingParametersVariation * GetVariation(double dbInstant);

private:

    bool m_bParkingManagement;
    bool m_bCreateVehiclesWithoutStock;
    int  m_nMaxParkingSpotsWhenNoMaximumParkingCapacity;

    // variations temporelles des paramètres
    ListOfTimeVariation<ParkingParametersVariation> m_LstVariations;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sérialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

//! Classe définissant une variante temporelle des paramètres pour le stationnement en zone
class ParkingParametersVariation
{

public:
    //! Default constructor
    ParkingParametersVariation(void);
    //! Destructor
    virtual ~ParkingParametersVariation(void);

    // Chargement à partir d'un noeud XML
    bool LoadFromXML(XERCES_CPP_NAMESPACE::DOMNode * pParkingParametersNode, bool bGestionStationnement, Logger * pLogger);

    double GetResidentialRatioOrigin() const;
    double GetResidentialRatioDestination() const;
    double GetSurfacicProportionOrigin() const;
    double GetSurfacicProportionDestination() const;
    bool UseCosts() const;
    double GetSurfacicCost() const;
    double GetHSCost() const;
    double GetCostRatio() const;
    double GetMaxSurfacicSearchDuration() const;
    double GetReroutingPeriod() const;
    double GetWideningDelay() const;
    bool AutoGenerateParkingDemand() const;
    double GetParkingMeanTime() const;
    double GetParkingTimeSigma() const;
    bool UseSimplifiedSurcaficMode() const;
    double GetUpdateGoalDistancePeriod() const;
    double GetLengthBetweenTwoParkBlocks() const;
    double GetLengthBetweenTwoParkSpots() const;
    double GetNbOfSpotsPerBlock() const;

private:

    double m_dbProportionResidentialOrigin;
    double m_dbProportionResidentialDestination;
    double m_dbProportionSurfacicOrigin;
    double m_dbProportionSurfacicDestination;
    bool   m_bUseCosts;
    double m_dbSurfacicCost;
    double m_dbHSCost;
    double m_dbCostRatio;
    double m_dbMaxSurfacicSearchDuration;
    double m_dbReroutingPeriod;
    double m_dbWideningDelay;
    bool   m_bAutoGenerateParkingDemand;
    double m_dbParkingMeanTime;
    double m_dbParkingTimeSigma;
    bool   m_bUseSimplifiedSurcaficMode;
    double m_dbUpdateGoalDistancePeriod;
    double m_dbLengthBetweenTwoParkBlocks;
    double m_dbLengthBetweenTwoParkSpots;
    double m_dbNbOfSpotsPerBlock;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Sérialisation
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:

    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version);
};

#endif // _PARKING_PARAMETERS_H__
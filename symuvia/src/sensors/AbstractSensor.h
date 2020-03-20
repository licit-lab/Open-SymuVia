#pragma once
#ifndef AbstractSensorH
#define AbstractSensorH

#include <string>
#include <deque>

#include <boost/shared_ptr.hpp>
#include <boost/serialization/assume_abstract.hpp>

#include <float.h>

namespace boost {
    namespace serialization {
        class access;
    }
}

class DocTrafic;
class TraceDocTrafic;
class CSVOutputWriter;
class Reseau;
class Vehicule;
class Tuyau;

class AbstractSensorSnapshot {

public:
    virtual ~AbstractSensorSnapshot();
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(AbstractSensorSnapshot)

class AbstractSensor
{

public:
    AbstractSensor();
    AbstractSensor(const std::string & strNom);
    virtual ~AbstractSensor();

    virtual std::string GetSensorPeriodXMLNodeName() const = 0;

    virtual void ResetData();

    virtual void CalculInfoCapteur(Reseau * pNetwork, double dbInstant, bool bNewPeriod, double dbInstNewPeriode, boost::shared_ptr<Vehicule> pVeh) = 0;

    //! ajout aux données capteurs d'un véhicule sortant d'un tronçon meso à la position curviligne dbLengthInLink
    virtual void AddMesoVehicle(double dbInstant, Vehicule * pVeh, Tuyau * pLink, Tuyau * pDownstreamLink, double dbLengthInLink = DBL_MAX) {}

    virtual void WriteDef(DocTrafic* pDocTrafic) = 0;
    virtual void Write(double dbInstant, Reseau * pReseau, double dbPeriodAgregation, double dbDebut, double dbFin, const std::deque<TraceDocTrafic* > & docTrafics, CSVOutputWriter * pCSVOutput) = 0;

    virtual void PrepareNextPeriod() = 0;

	std::string GetUnifiedID();

    // méthode pour la sauvegarde et restitution de l'état des capteurs (affectation dynamique convergente)
    virtual AbstractSensorSnapshot * TakeSnapshot() = 0;
    virtual void Restore(Reseau * pNetwork, AbstractSensorSnapshot * backup) = 0;

protected:

    std::string     m_strNom;    

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // AbstractSensorH

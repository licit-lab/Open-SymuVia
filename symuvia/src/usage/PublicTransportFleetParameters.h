#pragma once
#ifndef PublicTransportFleetParametersH
#define PublicTransportFleetParametersH

#include "AbstractFleetParameters.h"


class PublicTransportFleetParameters : public AbstractFleetParameters
{

public:
    PublicTransportFleetParameters();
    virtual ~PublicTransportFleetParameters();

    void    SetLastNbMontees(int lastNbMontees);
    int     GetLastNbMontees();

    void    SetLastNbDescentes(int lastNbDescentes);
    int     GetLastNbDescentes();

    virtual AbstractFleetParameters * Clone();
    
protected:

    // Nombre de piétons qui sont montés au dernier arrêt (pour sortie dans le doc de trafic XML)
    int     m_nLastNbMontees;   
    int     m_nLastNbDescentes;   


////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // PublicTransportFleetParametersH

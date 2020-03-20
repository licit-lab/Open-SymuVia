#pragma once
#ifndef CarFollowingFactoryH
#define CarFollowingFactoryH

#include <xercesc/util/XercesVersion.hpp>

#include <boost/shared_ptr.hpp>

#include <vector>

namespace XERCES_CPP_NAMESPACE {
    class DOMNode;
}

namespace boost {
    namespace serialization {
        class access;
    }
}

class AbstractCarFollowing;
class Reseau;
class Vehicule;
class ScriptedCarFollowing;

class CarFollowingFactory
{
public:
    CarFollowingFactory();
    CarFollowingFactory(Reseau * pNetwork);
    virtual ~CarFollowingFactory();

    // Ajoute un modèle de loi de poursuite
    void addCarFollowing(XERCES_CPP_NAMESPACE::DOMNode * pNodeCarFollowing);

    AbstractCarFollowing * buildCarFollowing(Vehicule * pVehicle);

    AbstractCarFollowing * copyCarFollowing(Vehicule * pVehicle, AbstractCarFollowing * pOriginalCarFollowing);

    ScriptedCarFollowing* GetScriptedCarFollowingFromID(const std::string & strLawID);

private:
    Reseau * m_pNetwork;

    std::vector<ScriptedCarFollowing*> m_LstScriptedLaws;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);

};

#endif // CarFollowingFactoryH

#include "stdafx.h"
#include "CarFollowingFactory.h"

#include "NewellCarFollowing.h"
#include "GippsCarFollowing.h"
#include "IDMCarFollowing.h"
#include "CarFollowingContext.h"
#include "ScriptedCarFollowing.h"
#include "../reseau.h"
#include "../vehicule.h"
#include "../RandManager.h"
#include "../SystemUtil.h"
#include "../Xerces/XMLUtil.h"
#include "../regulation/PythonUtils.h"

#include <xercesc/dom/DOMNode.hpp>

#include <boost/serialization/vector.hpp>

XERCES_CPP_NAMESPACE_USE

CarFollowingFactory::CarFollowingFactory() :
m_pNetwork(NULL)
{
}


CarFollowingFactory::CarFollowingFactory(Reseau * pNetwork) :
    m_pNetwork(pNetwork)
{
}

CarFollowingFactory::~CarFollowingFactory()
{
    if(m_LstScriptedLaws.size() > 0)
    {
        m_pNetwork->GetPythonUtils()->lockChildInterpreter();
        for(size_t i = 0; i < m_LstScriptedLaws.size(); i++)
        {
            delete m_LstScriptedLaws[i];
        }
        m_pNetwork->GetPythonUtils()->unlockChildInterpreter();
    }
}

void CarFollowingFactory::addCarFollowing(DOMNode * pNodeCarFollowing)
{
    // Récupération de l'ID de la loi de poursuite
    std::string strTmp;
    GetXmlAttributeValue(pNodeCarFollowing, "id", strTmp, m_pNetwork->GetLogger());

    ScriptedCarFollowing * pScriptedCarFollowing = new ScriptedCarFollowing();
    pScriptedCarFollowing->SetID(strTmp);

    // Récupération de la variable d'état considérée
    GetXmlAttributeValue(pNodeCarFollowing, "variable_etat", strTmp, m_pNetwork->GetLogger());
    AbstractCarFollowing::EStateVariable var;
    if(strTmp == "acceleration")
    {
        var = AbstractCarFollowing::STATE_VARIABLE_ACCELERATION;
    }
    else if(strTmp == "vitesse")
    {
        var = AbstractCarFollowing::STATE_VARIABLE_SPEED;
    }
    else
    {
        var = AbstractCarFollowing::STATE_VARIABLE_POSITION;
    }
    pScriptedCarFollowing->SetStateVariable(var);

    // Récupération des noms de fonctions et du chemin du module python à utiliser
    DOMNode * pNodeFunc = m_pNetwork->GetXMLUtil()->SelectSingleNode( "./FONCTIONS", pNodeCarFollowing->getOwnerDocument(), (DOMElement*)pNodeCarFollowing);    
    assert(pNodeFunc);
    if (GetXmlAttributeValue(pNodeFunc, "loi", strTmp, m_pNetwork->GetLogger())) pScriptedCarFollowing->SetInternalComputeFuncName(strTmp);
    if (GetXmlAttributeValue(pNodeFunc, "distanceInfluenceMax", strTmp, m_pNetwork->GetLogger())) pScriptedCarFollowing->SetMaxInfluenceDistanceFuncName(strTmp);
    if (GetXmlAttributeValue(pNodeFunc, "longueurVehicule", strTmp, m_pNetwork->GetLogger())) pScriptedCarFollowing->SetVehicleLengthFuncName(strTmp);
    if (GetXmlAttributeValue(pNodeFunc, "vitesseApprochee", strTmp, m_pNetwork->GetLogger())) pScriptedCarFollowing->SetApproxSpeedFuncName(strTmp);
    if (GetXmlAttributeValue(pNodeFunc, "agressivite", strTmp, m_pNetwork->GetLogger())) pScriptedCarFollowing->SetAggressivityFuncName(strTmp);
    if (GetXmlAttributeValue(pNodeFunc, "testChangementVoie", strTmp, m_pNetwork->GetLogger())) pScriptedCarFollowing->SetTestLaneChangeFuncName(strTmp);
    if (GetXmlAttributeValue(pNodeFunc, "appliqueChangementVoie", strTmp, m_pNetwork->GetLogger())) pScriptedCarFollowing->SetApplyLaneChangeFuncName(strTmp);
    if (GetXmlAttributeValue(pNodeFunc, "calculVitesseVoie", strTmp, m_pNetwork->GetLogger())) pScriptedCarFollowing->SetLaneSpeedFuncName(strTmp);
    if (GetXmlAttributeValue(pNodeFunc, "testInsertionConvergent", strTmp, m_pNetwork->GetLogger())) pScriptedCarFollowing->SetTestConvergentInsertionFuncName(strTmp);

    if (GetXmlAttributeValue(pNodeFunc, "module", strTmp, m_pNetwork->GetLogger())) pScriptedCarFollowing->SetModuleName(strTmp, m_pNetwork);

    m_LstScriptedLaws.push_back(pScriptedCarFollowing);
}

AbstractCarFollowing * CarFollowingFactory::buildCarFollowing( Vehicule * pVehicle)
{
    AbstractCarFollowing *pCarFollowing = NULL;

    bool bScriptedLaw = false;
    std::string strLawId;
    if(!pVehicle->GetType()->GetScriptedLawsCoeffs().empty())
    {
        double dbSum = 0;
        double dbRand = m_pNetwork->GetRandManager()->myRand() / (double)MAXIMUM_RANDOM_NUMBER;
        std::map<std::string, double>::const_iterator iter;
        for(iter = pVehicle->GetType()->GetScriptedLawsCoeffs().begin(); iter != pVehicle->GetType()->GetScriptedLawsCoeffs().end(); ++iter)
        {
            dbSum += iter->second;

            if( dbSum >= dbRand )
            {
                strLawId = iter->first;
                bScriptedLaw = true;
                break;
            }
        }
    }

    if(bScriptedLaw)
    {
		//Those 3 are not exactly scripted law, but the specific law is defined the same way
		if (strLawId == "Newell"){
			pCarFollowing = new NewellCarFollowing();
		}
		else if (strLawId == "IDM"){
			pCarFollowing = new IDMCarFollowing(false);
		}
		else if (strLawId == "Gipps"){
			pCarFollowing = new GippsCarFollowing();
		}
		else
		{
			ScriptedCarFollowing * pScriptedCarFollowing = GetScriptedCarFollowingFromID(strLawId);
			pScriptedCarFollowing->GetParameters() = pVehicle->GetType()->GetScriptedLawsParams()[strLawId];
			pCarFollowing = copyCarFollowing(pVehicle, pScriptedCarFollowing);
		}
    }
	// By default, we use Newell
    else
    {
        // rmq. : Pour l'instant je laisse les autres lois de poursuite en version C++ pour faire des tests de comparaison avec leur version python
        pCarFollowing = new NewellCarFollowing();
        //pCarFollowing = new GippsCarFollowing();
        //pCarFollowing = new IDMCarFollowing(true);
    }

    pCarFollowing->Initialize(m_pNetwork, pVehicle);
    return pCarFollowing;
}

AbstractCarFollowing * CarFollowingFactory::copyCarFollowing(Vehicule * pVehicle, AbstractCarFollowing * pOriginalCarFollowing)
{
    AbstractCarFollowing * pResult = NULL;

    NewellCarFollowing * pNewell = dynamic_cast<NewellCarFollowing*>(pOriginalCarFollowing);

    if(pNewell)
    {
        pResult = new NewellCarFollowing();
    }
    else
    {
        GippsCarFollowing * pGipps = dynamic_cast<GippsCarFollowing*>(pOriginalCarFollowing);

        if(pGipps)
        {
            pResult = new GippsCarFollowing();
        }
        else
        {
            IDMCarFollowing * pIDM = dynamic_cast<IDMCarFollowing*>(pOriginalCarFollowing);

            if(pIDM)
            {
                pResult = new IDMCarFollowing(pIDM->IsImprovedIDM());
            }
            else
            {
                ScriptedCarFollowing * pScripted = dynamic_cast<ScriptedCarFollowing*>(pOriginalCarFollowing);

                if(pScripted)
                {
                    pResult = new ScriptedCarFollowing();
                    ((ScriptedCarFollowing*)pResult)->Copy(pScripted);
                }
                else
                {
                    // Traiter ici le cas des autres lois de poursuite
                    assert(false);
                }
            }
        }
    }

    pResult->Initialize(m_pNetwork, pVehicle);

    
    // rmq. pas besoin de sauvegarder le PreviousContext (il sera de toute façon supprimé dès le traitement du véhicule pour le pas de temps à venir)
    // par contre, on doit sauvegarder pour le dernier context les infos telles que les leaders, le deltaN (Newell), l'état fluide ou congestionné,...
    // puisqu'il va devenir le PreviousContext utilisé pour le pas de temps à venir.
    if(pOriginalCarFollowing->GetCurrentContext())
    {
        CarFollowingContext * pContext = pOriginalCarFollowing->CreateContext(m_pNetwork->m_dbInstSimu);
        pOriginalCarFollowing->GetCurrentContext()->CopyTo(pContext);
        pResult->SetCurrentContext(pContext);
    }

    return pResult;
}

ScriptedCarFollowing* CarFollowingFactory::GetScriptedCarFollowingFromID(const std::string & strLawID)
{
    ScriptedCarFollowing * pResult = NULL;
    for(size_t i = 0; i < m_LstScriptedLaws.size() && !pResult; i++)
    {
        if(!m_LstScriptedLaws[i]->GetID().compare(strLawID))
        {
            pResult = m_LstScriptedLaws[i];
        }
    }
    //assert(pResult);
    return pResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void CarFollowingFactory::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CarFollowingFactory::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CarFollowingFactory::serialize(Archive& ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(m_pNetwork);
    ar & BOOST_SERIALIZATION_NVP(m_LstScriptedLaws);
}

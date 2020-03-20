#include "stdafx.h"
#include "AffectationSave.h"

#include "tuyau.h"
#include "Xerces/DOMLSSerializerSymu.hpp"
#include "SystemUtil.h"
#include "vehicule.h"
#include "SymuCoreManager.h"

#include "Xerces/XMLUtil.h"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/set.hpp>


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Méthode(s) de la classe "CItineraire" qui gère la sauvegarde des itinéraires prédits
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CItineraire::write(DOMLSSerializerSymu * writer, bool bIsRealized)
{
	std::string elem;
	size_t count = elements.size();
	if (count > 0)
	{
		elem = elements[0];
		for (size_t i=1; i<count; i++)
		{
			elem += " " + elements[i];
		}
	}
    std::string vehicles;
    for(std::set<int>::const_iterator iter = vehicleIDs.begin(); iter != vehicleIDs.end(); ++iter)
    {
        if(iter != vehicleIDs.begin())
        {
            vehicles += " ";
        }
        vehicles += SystemUtil::ToString(*iter);
    }
	writer->writeStartElement(XS("ITINERAIRE"));
	writer->writeAttributeString(XS("elements"), XS(elem.c_str()));
	writer->writeAttributeString(XS("coeffs"), XS(SystemUtil::ToString(3, coeffs).c_str()));
	writer->writeAttributeString(XS("tempsparcours"),XS( SystemUtil::ToString(3, tpsparcours).c_str()));
    writer->writeAttributeString(XS("longueur"),XS(SystemUtil::ToString(3, distance).c_str()));
    writer->writeAttributeString(XS("predefini"),XS(SystemUtil::ToString(predefini).c_str()));
    if(bIsRealized)
    {
        writer->writeAttributeString(XS("vehicules"),XS(vehicles.c_str()));
    }
	writer->writeEndElement();//"ITINERAIRE"
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void CItineraire::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);

template<class Archive>
void CItineraire::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(tuyaux);
    ar & BOOST_SERIALIZATION_NVP(elements);
    ar & BOOST_SERIALIZATION_NVP(vehicleIDs);
    ar & BOOST_SERIALIZATION_NVP(coeffs);
    ar & BOOST_SERIALIZATION_NVP(tpsparcours);
    ar & BOOST_SERIALIZATION_NVP(distance);
    ar & BOOST_SERIALIZATION_NVP(predefini);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Méthode(s) de la classe "CIteration" qui gère la sauvegarde des itérations prédites
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CItineraire * CIteration::addItineraire(const std::vector<Tuyau*> & tuyaux, const std::vector<std::string> & elements, double coeffs, double tempsparcours,
    bool bPredefini, double dbLength)
{
	size_t count;
	CItineraire iti;
	iti.tuyaux = tuyaux;
	iti.elements = elements;
	iti.coeffs = coeffs;
	iti.tpsparcours = tempsparcours;
    iti.predefini = bPredefini;
    iti.distance = dbLength;
	count = itineraires.size();
	itineraires.push_back(iti);
	return &itineraires[count];
}

void CIteration::write(DOMLSSerializerSymu * writer)
{
		writer->writeStartElement(XS("ITERATION"));
		writer->writeAttributeString(XS("num"), XS(SystemUtil::ToString(num).c_str()));
		writer->writeAttributeString(XS("temps_calcul"), XS(SystemUtil::ToString(temps_calcul).c_str()));
		writer->writeAttributeString(XS("indice_proximite"), XS(SystemUtil::ToString(3, indice_proximite).c_str()));
		writer->writeStartElement(XS("ITINERAIRES"));
		
		size_t count = itineraires.size();
		for (size_t i=0; i<count; i++)
		{
			itineraires[i].write(writer, false);
		}

		writer->writeEndElement();//"ITINERAIRES"

		writer->writeEndElement();//"ITERATION"
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void CIteration::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);

template<class Archive>
void CIteration::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(num);
    ar & BOOST_SERIALIZATION_NVP(temps_calcul);
    ar & BOOST_SERIALIZATION_NVP(indice_proximite);
    ar & BOOST_SERIALIZATION_NVP(itineraires);
    ar & BOOST_SERIALIZATION_NVP(temps_debut_calcul);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Méthode(s) de la classe "CCoupleOD" qui gère la sauvegarde des couples Origine/Destination
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CIteration * CCoupleOD::AddPreditIteration(size_t num)
{
	// Calcul de l'instant de début
	STimeSpan TS;
	STime temps_debut_calcul;
	size_t count = predit.size();
	if ( count == 0 )//Liste vide, il faut créer une itération
	{
		CIteration iter;
		// Initialisation calcul de la durée du calcul
		iter.temps_debut_calcul = SDateTime::Now().ToTime();
		iter.temps_calcul = 0;
		// Positionnement de valeurs
		iter.num = num;
		iter.indice_proximite = -1;
		// Ajout de l'itération
		predit.push_back(iter);
		return &predit[count];
	}
	else
	{	// Temps de fin du dernier
		size_t lastNum;
		lastNum = predit[count-1].num;
		if (num <= lastNum) // On est tjs dans la même itération mais avec un autre itinéraire
		{					// Il faut modifier l'itération pour l'ajout d'un itinéraire
							// En fait (num == lastNum)
			CIteration * pIter = &predit[count-1];
			// Initialisation calcul de la durée du calcul
			pIter->temps_debut_calcul = SDateTime::Now().ToTime();
			pIter->temps_calcul = 0;
			// Positionnement de valeurs
			pIter->indice_proximite = -1;
			// Renvoyer l'adresse de l'itération courante
			return pIter;
		}
		else // Il faut effectivement ajouter une itération (nouvelle itération)
		{
			CIteration iter;
			// Initialisation calcul de la durée du calcul
			iter.temps_debut_calcul = SDateTime::Now().ToTime();
			iter.temps_calcul = 0;
			// Positionnement de valeurs
			iter.num = num;
			iter.indice_proximite = -1;
			// Ajout de l'itération
			predit.push_back(iter);
			// Renvoyer l'adresse de la dernière itération de la liste (celle qu'on vient d'ajouter)
			return &predit[count];
		}
	}
}

CItineraire * CCoupleOD::AddRealiseItineraire(const std::vector<Tuyau*> & tuyaux, const std::vector<std::string> & elements, bool bPredefini, int vehID, double dbLength)
{
	CItineraire * pIti = GetRealiseItineraire(elements);
	if (pIti == NULL)
	{
		size_t count;
		CItineraire iti;
		iti.tuyaux = tuyaux;
		iti.elements = elements;
		iti.coeffs = 1;
        iti.predefini = bPredefini;
        iti.vehicleIDs.insert(vehID);
        iti.distance = dbLength;
		count = realise.size();
		realise.push_back(iti);
		return &realise[count];
	}
	else
	{
        pIti->vehicleIDs.insert(vehID);
		pIti->coeffs += 1.0;
		return pIti;
	}
}

CItineraire * CCoupleOD::GetRealiseItineraire(const std::vector<std::string> &elements)
{
	std::deque<CItineraire>::iterator itIti, beginIti, endIti;
	beginIti = realise.begin();
	endIti = realise.end();
	size_t elements_size = elements.size();
	CItineraire * itiRes = NULL;

	for (itIti = beginIti; (itiRes == NULL) && (itIti != endIti); itIti++)
	{
		if (itIti->elements.size() == elements_size)
		{
			bool bEqual = true;
			for (size_t i = 0; bEqual && (i<elements_size); i++)
			{
				bEqual = (itIti->elements[i] == elements[i]);
			}
			if (bEqual)
			{
				itiRes = &(*itIti);
			}
		}
	}

	return itiRes;
}

void CCoupleOD::write(DOMLSSerializerSymu * writer)
{
	writer->writeStartElement(XS("COUPLE_OD"));
	writer->writeAttributeString(XS("origine"), XS(origine.c_str()));
	writer->writeAttributeString(XS("destination"), XS(destination.c_str()));
	writer->writeAttributeString(XS("nb_veh_emis"), XS(SystemUtil::ToString(nb_veh_emis).c_str()));
	writer->writeAttributeString(XS("nb_veh_recus"), XS(SystemUtil::ToString(nb_veh_recus).c_str()));

	writer->writeStartElement(XS("PREDIT"));
	writer->writeStartElement(XS("ITERATIONS"));

	size_t count = predit.size();
	size_t i;
	for (i=0; i<count; i++)
	{
		predit[i].write(writer);
	}

	writer->writeEndElement();//"ITERATIONS"
	writer->writeEndElement();//"PREDIT"

	double sum = 0;
	count = realise.size();
	for (i=0; i<count; i++)
	{
		sum += realise[i].coeffs;
	}

	for (i=0; i<count; i++)
	{
		realise[i].coeffs /= sum;
	}

	writer->writeStartElement(XS("REALISE"));
	writer->writeStartElement(XS("ITINERAIRES"));

	for (i=0; i<count; i++)
	{
		realise[i].write(writer, true);
	}

	writer->writeEndElement();//"ITINERAIRES"
	writer->writeEndElement();//"REALISE"

	writer->writeEndElement();//"COUPLE_OD"
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void CCoupleOD::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);

template<class Archive>
void CCoupleOD::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(origine);
    ar & BOOST_SERIALIZATION_NVP(destination);
    ar & BOOST_SERIALIZATION_NVP(nb_veh_emis);
    ar & BOOST_SERIALIZATION_NVP(nb_veh_recus);
    ar & BOOST_SERIALIZATION_NVP(predit);
    ar & BOOST_SERIALIZATION_NVP(realise);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Méthode(s) de la classe "CPeriode" qui gère la sauvegarde d'une période d'affectation
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//STime CPeriode::temps_debut_calcul;

void CPeriode::InitPeriode()
{
	type.clear();
	debut = 0;
	fin = 0;
	temps_calcul = 0;
	nb_iteration = 0;
	indicateur_proximite = 1.0;
	seuil_tempsparcours = 0;
	temps_debut_calcul = SDateTime::Now().ToTime();
}

void CPeriode::FinPeriode(SymuCoreManager * pSymuScript, const std::string & sType, int nb_iterationA, double indicateur_proximiteA, double finA, double seuil_tempsparcoursA, bool bForceGraphReset)
{
	// Calcul de debut et fin
	debut = fin;
	fin = finA;
	// Calcul du temps de calcul
	STime temps_fin_calcul = SDateTime::Now().ToTime();
	STimeSpan temps_calc = temps_fin_calcul - temps_debut_calcul; 
	temps_calcul = (int)temps_calc.GetTotalSeconds();
	temps_debut_calcul = temps_fin_calcul;
	type = sType;
	// Nombre d'itérations
	nb_iteration = nb_iterationA;
	// Indicateur de proximité
	indicateur_proximite = indicateur_proximiteA;
	seuil_tempsparcours = seuil_tempsparcoursA;

	// Calcul des temps de parcours réalisé	
	std::map<TypeVehicule*, std::map<std::pair<std::string, std::string>, CCoupleOD>>::iterator itTVeh;
    std::deque<CItineraire>::iterator itIti, beginIti, endIti;
	for (itTVeh = periode_type_vehicule.begin(); itTVeh != periode_type_vehicule.end(); ++itTVeh)
	{
		std::map<std::pair<std::string, std::string>, CCoupleOD>::iterator beginCpl, endCpl, itCpl;
		beginCpl = itTVeh->second.begin();
		endCpl = itTVeh->second.end();
		for (itCpl = beginCpl; itCpl != endCpl; ++itCpl)
		{
            beginIti = itCpl->second.realise.begin();
	        endIti = itCpl->second.realise.end();
            for (itIti = beginIti; itIti != endIti; ++itIti)
	        {
                (*itIti).tpsparcours = pSymuScript->ComputeCost(itTVeh->first, (*itIti).tuyaux, bForceGraphReset);
            }
		}
	}
}

void CPeriode::CleanPeriode(bool bClear)
{
	// Nettoyage des coefficients prédits et réalisés
	std::map<TypeVehicule*, std::map<std::pair<std::string, std::string>, CCoupleOD>>::iterator beginTVeh, endTVeh, itTVeh;
	beginTVeh = periode_type_vehicule.begin();
	endTVeh = periode_type_vehicule.end();
	for (itTVeh = beginTVeh; itTVeh != endTVeh; itTVeh++)
	{
		std::map<std::pair<std::string, std::string>, CCoupleOD>::iterator beginCpl, endCpl, itCpl;
		beginCpl = itTVeh->second.begin();
		endCpl = itTVeh->second.end();
		for (itCpl = beginCpl; itCpl != endCpl; itCpl++)
		{
			itCpl->second.nb_veh_emis = 0;
			itCpl->second.nb_veh_recus = 0;

			itCpl->second.realise.clear();
			if( bClear)
			{
				itCpl->second.predit.clear();				
			}
		}
	}
}

CCoupleOD * CPeriode::getCoupleOD(TypeVehicule* pTVeh, const std::string & sOrig, const std::string & sDest)
{
	std::map<std::pair<std::string, std::string>, CCoupleOD> * pListCpl = &periode_type_vehicule[pTVeh];

	std::map<std::pair<std::string, std::string>, CCoupleOD>::iterator itCpl;
	itCpl = pListCpl->find(std::pair<std::string, std::string>(sOrig, sDest));

	if (itCpl == pListCpl->end())
	{
		return NULL;
	}
	else
	{
		return &(itCpl->second);
	}
}

std::map<std::pair<std::string, std::string>, CCoupleOD> * 
	CPeriode::addTypeVehicule(TypeVehicule* pTVeh)
{
	std::map<TypeVehicule*, std::map<std::pair<std::string, std::string>, CCoupleOD>>::iterator it;
	it = periode_type_vehicule.find(pTVeh);
	if (it == periode_type_vehicule.end())
	{
		std::map<std::pair<std::string, std::string>, CCoupleOD> listVide;
		periode_type_vehicule[pTVeh] = listVide;
		return &periode_type_vehicule[pTVeh];
	}
	else
	{
		return &(it->second);
	}
}

CCoupleOD * CPeriode::addCoupleOD(TypeVehicule* pTVeh, const std::string & sOrig, const std::string & sDest)
{
	std::map<std::pair<std::string, std::string>, CCoupleOD> * pListCpl = addTypeVehicule(pTVeh);

	CCoupleOD Cpl(sOrig, sDest);

	{
		(*pListCpl)[std::pair<std::string, std::string>(sOrig, sDest)] = Cpl;

		return &(*pListCpl)[std::pair<std::string, std::string>(sOrig, sDest)];
	}
}

CCoupleOD * CPeriode::addCoupleOD(std::map<std::pair<std::string, std::string>, CCoupleOD> * pListCoupleOD, const std::string & sOrig, const std::string & sDest)
{
	std::map<std::pair<std::string, std::string>, CCoupleOD>::iterator it;

	if (pListCoupleOD == NULL)
	{
		return NULL;
	}

	CCoupleOD Cpl(sOrig, sDest);

	it = pListCoupleOD->find(std::pair<std::string, std::string>(sOrig, sDest));
	if ( it != pListCoupleOD->end())
	{
		return &it->second;
	}
	else
	{
		(*pListCoupleOD)[std::pair<std::string, std::string>(sOrig, sDest)] = Cpl;

		return &(*pListCoupleOD)[std::pair<std::string, std::string>(sOrig, sDest)];
	}
}

void CPeriode::SetLastPreditIterationIndProx(double dbIndProx)
{
	STime temps_fin_calcul = SDateTime::Now().ToTime();
	STimeSpan TS;
	std::map<TypeVehicule*, std::map<std::pair<std::string, std::string>, CCoupleOD>>::iterator it, itEnd;
	itEnd = periode_type_vehicule.end();
	for (it=periode_type_vehicule.begin(); it!=itEnd; it++)//itération sur les types de véhicule
	{
		std::map<std::pair<std::string, std::string>, CCoupleOD>::iterator it2, itEnd2;
		itEnd2 = it->second.end();
		for (it2=it->second.begin(); it2!=itEnd2; it2++)//itération sur les couples OD
		{
			CCoupleOD * pCplOD = &it2->second;
			std::deque<CIteration>::iterator it3, itEnd3;
			itEnd3 = pCplOD->predit.end();
			for (it3 = pCplOD->predit.begin(); it3!=itEnd3; it3++)//itération sur les "CItération"
			{
				if (it3->indice_proximite == -1)
				{
					it3->indice_proximite = dbIndProx;
					TS = temps_fin_calcul - it3->temps_debut_calcul;
					it3->temps_calcul = (int)TS.GetTotalSeconds();
				}
			}
		}
	}
}

void CPeriode::write(DOMLSSerializerSymu * writer)
{
	if (writer == NULL || (fin == 0 && debut == 0))
		return;
	writer->writeStartElement(XS("PERIODE"));
	writer->writeAttributeString(XS("type"), XS(type.c_str()));
	writer->writeAttributeString(XS("debut"), XS(SystemUtil::ToString(0, debut).c_str()));
	writer->writeAttributeString(XS("fin"), XS(SystemUtil::ToString(0, fin).c_str()));
	writer->writeAttributeString(XS("temps_calcul"), XS(SystemUtil::ToString(temps_calcul).c_str()));
	writer->writeAttributeString(XS("nb_iteration"), XS(SystemUtil::ToString(nb_iteration).c_str()));
	writer->writeAttributeString(XS("indicateur_proximite"), XS(SystemUtil::ToString(3, indicateur_proximite).c_str()));	

	std::map<TypeVehicule*, std::map<std::pair<std::string, std::string>, CCoupleOD>>::iterator beginVeh, endVeh, itVeh;
	beginVeh = periode_type_vehicule.begin();
	endVeh = periode_type_vehicule.end();
	for (itVeh = beginVeh; itVeh != endVeh; itVeh++)
	{
		writer->writeStartElement(XS("PERIODE_TYPE_VEHICULE"));
		writer->writeAttributeString(XS("type_vehicule"), XS((*itVeh).first->GetLabel().c_str()));

		writer->writeStartElement(XS("COUPLES_OD"));

		std::map<std::pair<std::string, std::string>, CCoupleOD> * listCoupleOD;

		listCoupleOD = &(*itVeh).second;
		std::map<std::pair<std::string, std::string>, CCoupleOD>::iterator beginCpl, endCpl, itCpl;
		beginCpl = listCoupleOD->begin();
		endCpl = listCoupleOD->end();
		for (itCpl = beginCpl; itCpl != endCpl; itCpl++)
		{
			(*itCpl).second.write(writer);
		}

		writer->writeEndElement();//"COUPLES_OD"
		writer->writeEndElement();//"PERIODE_TYPE_VEHICULE"
	}

	writer->writeEndElement();//"PERIODE"
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template void CPeriode::serialize(boost::archive::xml_woarchive & ar, const unsigned int version);
template void CPeriode::serialize(boost::archive::xml_wiarchive & ar, const unsigned int version);

template<class Archive>
void CPeriode::serialize(Archive & ar, const unsigned int version)
{
    ar & BOOST_SERIALIZATION_NVP(type);
    ar & BOOST_SERIALIZATION_NVP(debut);
    ar & BOOST_SERIALIZATION_NVP(fin);
    ar & BOOST_SERIALIZATION_NVP(temps_calcul);
    ar & BOOST_SERIALIZATION_NVP(nb_iteration);
    ar & BOOST_SERIALIZATION_NVP(indicateur_proximite);
    ar & BOOST_SERIALIZATION_NVP(seuil_tempsparcours);

    ar & BOOST_SERIALIZATION_NVP(periode_type_vehicule);
    ar & BOOST_SERIALIZATION_NVP(temps_debut_calcul);
}


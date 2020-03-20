#pragma once
#ifndef RandManagerH
#define RandManagerH

#include <boost/random.hpp>

// rmq. on utilise la valeur pour Windows de RAND_MAX de stdlib.h. car c'est la valeur utilisée historiquement par SymuVia.
// on n'utilise pas directement RAND_MAX car sa valeur est différente sous Linux.
#define MAXIMUM_RANDOM_NUMBER 0x7fff

/// Classe de gestion de la génération des nombres aléatoires (avec reprise possible de la séquence
/// pseudo-aléatoire à la reprise d'un snapshot)
class RandManager
{
public:

    // Constructeur par défaut
    RandManager();

    /// Initialisation de la séquence aléatoire
    void mySrand(unsigned int seed, unsigned int nbCalls = 0);

    int myRand();

    int myPoissonRand(double mean);

    int myBinomialRand(double probability, int trialNb);

    double myExponentialRand(double rate);

    double myNormalRand(double dbMean, double dbSigma);

    double myNormalRand(double dbMean, double dbSigma, double dbMin, double dbMax);

    unsigned int getCount();

private:
    unsigned int m_Count;
    boost::mt19937 m_RandomGenerator;
    boost::uniform_int<> m_Distribution;
    boost::variate_generator< boost::mt19937&, boost::uniform_int<> > m_LimitedInt;
};

#endif // RandManagerH
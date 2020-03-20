#pragma once
#ifndef symubruitH
#define symubruitH

#include <float.h>
#include <math.h>

// Position indéfinie (le véhicule vient d'être créé et n'est pas sur le réseau)
#define UNDEF_POSITION -DBL_MIN
// Instant indéfini
#define UNDEF_INSTANT -DBL_MIN
// Stock indéfini
#define UNDEF_STOCK -DBL_MIN

#define ZERO_DOUBLE         0.000001    // zero de type double
#define NONVAL_DOUBLE       9999        // Utilisé pour ne pas traiter la valeur

namespace boost {
    namespace serialization {
        class access;
    }
}

//***************************************************************************//
//                          structure Point                                  //
//***************************************************************************//
struct Point
{
    Point(){dbX = 0.0;dbY = 0.0;dbZ = 0.0;}
    Point(double x, double y, double z){dbX = x;dbY = y;dbZ = z;}
    double DistanceTo(const Point& ptOtherPoint){return sqrt((dbX - ptOtherPoint.dbX)*(dbX - ptOtherPoint.dbX) + (dbY - ptOtherPoint.dbY)*(dbY - ptOtherPoint.dbY) + (dbZ - ptOtherPoint.dbZ)*(dbZ - ptOtherPoint.dbZ));}

    double      dbX;
    double      dbY;
    double      dbZ;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version);
};

#endif

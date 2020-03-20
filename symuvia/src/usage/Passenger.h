#pragma once
#ifndef PassengerH
#define PassengerH

namespace boost {
    namespace serialization {
        class access;
    }
}

class TripNode;

class Passenger
{

public:
    Passenger();
    Passenger(int externalID, TripNode * pDestination);
    virtual ~Passenger();

    int GetExternalID() const;
    TripNode * GetDestination() const;

protected:

    int m_ExternalID;
    TripNode * m_pDestination;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sérialisation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive& ar, const unsigned int version);
};

#endif // PassengerH

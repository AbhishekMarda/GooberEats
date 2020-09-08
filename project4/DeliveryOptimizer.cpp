#include "provided.h"
#include <cmath>
#include <vector>
#include <random>

using namespace std;

class DeliveryOptimizerImpl
{
public:
    DeliveryOptimizerImpl(const StreetMap* sm);
    ~DeliveryOptimizerImpl();
    void optimizeDeliveryOrder(
        const GeoCoord& depot,
        vector<DeliveryRequest>& deliveries,
        double& oldCrowDistance,
        double& newCrowDistance) const;
private:
    const StreetMap* m_streetmap;               //maintain a pointer to the map of all the streets
    double calcCurrCrowDistance(const GeoCoord depot, vector<DeliveryRequest>& deliveries) const; //get the crow distance from the depot and back through the deliveries in the given vector
    void putSameLocDeliveriesTogether(vector<DeliveryRequest>& deliveries) const;   //deliveries with same name will be placed together in the vector
    struct DeliveryGroup
    {
        GeoCoord s_avgPoint;
        vector<DeliveryRequest> s_allCoords;
        void setAvgPoint();
    };
    void swap(vector<DeliveryGroup>& groups, const int index1, const int index2) const;
    void swap(vector<DeliveryRequest>& deliveries, const int index1, const int index2) const;
    int randInt(int lowest, int highest) const;     // get a random integer between the supplied bounds
};
inline
int DeliveryOptimizerImpl::randInt(int lowest, int highest) const
{
    if (highest < lowest)
    {
        int temp = lowest;
        lowest=highest;
        highest=temp;
    }
    static random_device rd;
    static default_random_engine generator(rd());
    uniform_int_distribution<> distro(lowest, highest);
    return distro(generator);
}

void DeliveryOptimizerImpl::DeliveryGroup::setAvgPoint()
{
    long double avgLat=0, avgLong=0;
    for (int i=0; i<s_allCoords.size(); i++)
    {
        avgLat+= s_allCoords[i].location.latitude;
        avgLong+= s_allCoords[i].location.longitude;
    }
    
    //get the average coordinates
    avgLat/=s_allCoords.size();
    avgLong/=s_allCoords.size();
    
    //round the coordinates to the 7th decimal point
    avgLat=round(avgLat* pow(10,7))/(pow(10,7));
    avgLong=round(avgLong* pow(10,7))/(pow(10,7));
    
    //since GeoCoord constructor requires a string, convert the long doubles to strings 
    string latStr, longStr;
    latStr = to_string(avgLat);
    longStr = to_string(avgLong);
    
    //set the average point
    s_avgPoint = GeoCoord(latStr, longStr);
}

DeliveryOptimizerImpl::DeliveryOptimizerImpl(const StreetMap* sm)
{
    m_streetmap = sm;
}

DeliveryOptimizerImpl::~DeliveryOptimizerImpl()
{
}

void DeliveryOptimizerImpl::putSameLocDeliveriesTogether(vector<DeliveryRequest> &deliveries) const
{
    for (int i=0; i<deliveries.size(); i++)             //for every delivery
    {
        GeoCoord currLoc = deliveries[i].location;
        int posAfterCurrLoc = i+1;
        for (int j=i+1; j<deliveries.size(); j++)       //for every delivery after the current delivery
            if (deliveries[j].location == currLoc)      //if the name is same
                swap(deliveries, j, posAfterCurrLoc++); //swap with the position after the delivery
    }
}

void DeliveryOptimizerImpl::swap(vector<DeliveryRequest> &deliveries, const int index1, const int index2) const
{
    if (index1<0 || index2<0 || index1>=deliveries.size() || index2>=deliveries.size())
        return;
    DeliveryRequest temp(deliveries[index1]);
    deliveries[index1] = deliveries[index2];
    deliveries[index2] = temp;
}

double DeliveryOptimizerImpl::calcCurrCrowDistance(const GeoCoord depot, vector<DeliveryRequest>& deliveries) const
{
    //this function calculates the distance, for N deliveries, in the path: depot -> delivery1 -> delivery2 -> ... ->deliveryN->depot
    GeoCoord startCoord = depot;
    GeoCoord endCoord;
    double newCrowDis=0;
    for (int i=0; i<deliveries.size(); i++)
    {
        endCoord = deliveries[i].location;
        newCrowDis+= distanceEarthMiles(startCoord, endCoord);
        startCoord = endCoord;
    }
    newCrowDis += distanceEarthMiles(endCoord, depot);
    return newCrowDis;
}

void DeliveryOptimizerImpl::swap(vector<DeliveryGroup>& groups, const int index1, const int index2) const
{
    if (index1<0 || index2<0 || index1>=groups.size() || index2>=groups.size())
        return;
    DeliveryGroup temp(groups[index1]);
    groups[index1] = groups[index2];
    groups[index2] = temp;
}

void DeliveryOptimizerImpl::optimizeDeliveryOrder(
    const GeoCoord& depot,
    vector<DeliveryRequest>& deliveries,
    double& oldCrowDistance,
    double& newCrowDistance) const
{
    if (deliveries.empty())
        return;
    
    oldCrowDistance = calcCurrCrowDistance(depot, deliveries);
    
    //get the average distances from the depot
    double averageDistance=0, totalDistance=0;
    for (int i=0; i<deliveries.size(); i++)
        totalDistance += distanceEarthMiles(depot, deliveries[i].location);
    averageDistance = totalDistance/deliveries.size();
    
    vector<DeliveryRequest> shortestPermutation=deliveries;

    double prevCrowDistance = oldCrowDistance;
    double temperature;
    double coolingRate = 0.99;
    double absoluteTemperature = 0.0001;
    int maxGroupingTrials=5;
    
    
    for (temperature=10000; temperature > absoluteTemperature; temperature*=coolingRate)
    {
        //randomly shuffle the elements
        for (int i=0; i<deliveries.size(); i++)
            swap(deliveries, i, randInt(0, (int)deliveries.size()-1));
        
        //recalculate the crow distance
        newCrowDistance = calcCurrCrowDistance(depot, deliveries);
        
        //check if the random arrangement is somehow better
        if (newCrowDistance<prevCrowDistance)
        {
            shortestPermutation=deliveries;
            prevCrowDistance = newCrowDistance;
        }
        
        //group elements based on different radii for certain numbers of times
        for (int numTrials=0; numTrials<maxGroupingTrials; numTrials++)
        {
            
            int groupingRadius = randInt(0, averageDistance);
            double currDist = 0;
            vector<DeliveryGroup> groups;
            GeoCoord currCoord;
            
            //define different groups based on their proximity from each other
            for (int i=0; i<deliveries.size();)
            {
                currCoord = deliveries[i].location;
                for (int j=i ; j<deliveries.size();)
                {
                    currDist = distanceEarthMiles(currCoord, deliveries[j].location);
                    if (currDist<=groupingRadius)
                    {
                        if (currCoord == deliveries[j].location)
                        {
                            DeliveryGroup dg;
                            groups.push_back(dg);
                        }
                        groups[groups.size()-1].s_allCoords.push_back(deliveries[j]);
                        deliveries[j]=deliveries[deliveries.size()-1];
                        deliveries.pop_back();
                    }
                    else
                        j++;
                    currDist=0;
                }
            }
            //note that at this point deliveries vector will be empty
            
            //now that we have the groups, find the coordinate lying in the middle for each of the groups
            for(int i=0; i<groups.size(); i++)
                groups[i].setAvgPoint();
            
            //order the groups based on their distances from the center
            for (int i=0; i<groups.size(); i++)
            {
                double minDist = distanceEarthMiles(depot, groups[i].s_avgPoint);
                int minIndex = i;
                for (int j=i+1; j<groups.size(); j++)
                {
                    double currDist = distanceEarthMiles(depot, groups[j].s_avgPoint);
                    if (currDist<minDist)
                    {
                        minDist = currDist;
                        minIndex = j;
                    }
                }
                swap(groups, i, minIndex);
            }
            
            //in the current ordering of the groups, push back into the deliveries vector
            for (int i=0; i<groups.size(); i++)
                for (int j=0; j<groups[i].s_allCoords.size(); j++)
                    deliveries.push_back(groups[i].s_allCoords[j]);
            
            //better optimization when deliveries at the same location are together
            putSameLocDeliveriesTogether(deliveries);
            newCrowDistance = calcCurrCrowDistance(depot, deliveries);
            
            //consider if the new arrangement is better than before
            if (newCrowDistance<prevCrowDistance)
            {
                shortestPermutation=deliveries;
                prevCrowDistance = newCrowDistance;
            }
            
        }
    }
    
    deliveries = shortestPermutation;
    
    //get final crow distance 
    newCrowDistance = calcCurrCrowDistance(depot, deliveries);
}

//******************** DeliveryOptimizer functions ****************************

// These functions simply delegate to DeliveryOptimizerImpl's functions.
// You probably don't want to change any of this code.

DeliveryOptimizer::DeliveryOptimizer(const StreetMap* sm)
{
    m_impl = new DeliveryOptimizerImpl(sm);
}

DeliveryOptimizer::~DeliveryOptimizer()
{
    delete m_impl;
}

void DeliveryOptimizer::optimizeDeliveryOrder(
        const GeoCoord& depot,
        vector<DeliveryRequest>& deliveries,
        double& oldCrowDistance,
        double& newCrowDistance) const
{
    return m_impl->optimizeDeliveryOrder(depot, deliveries, oldCrowDistance, newCrowDistance);
}

#include "provided.h"
#include "ExpandableHashMap.h"
#include <list>
#include <queue>
#include <map>
#include <algorithm>
using namespace std;

class PointToPointRouterImpl
{
public:
    PointToPointRouterImpl(const StreetMap* sm);
    ~PointToPointRouterImpl();
    DeliveryResult generatePointToPointRoute(
        const GeoCoord& start,
        const GeoCoord& end,
        list<StreetSegment>& route,
        double& totalDistanceTravelled) const;
    
private:
    const StreetMap* m_streetmap;
};

PointToPointRouterImpl::PointToPointRouterImpl(const StreetMap* sm)
{
    m_streetmap= sm;
}

PointToPointRouterImpl::~PointToPointRouterImpl()
{
}

DeliveryResult PointToPointRouterImpl::generatePointToPointRoute(
        const GeoCoord& start,
        const GeoCoord& end,
        list<StreetSegment>& route,
        double& totalDistanceTravelled) const
{
    while (!route.empty())                                                      //empty the route list if there are any existing values
        route.pop_back();
    
    if (start == end)                                                           //account for when the starting position is the ending position
    {
        totalDistanceTravelled=0;
        return DELIVERY_SUCCESS;
    }
    
    vector<StreetSegment> startingSegs;
    if (m_streetmap->getSegmentsThatStartWith(end, startingSegs)==false      || //if the ending position does not exist in map
        m_streetmap ->getSegmentsThatStartWith(start, startingSegs)==false)     //or the starting position does not exist in map
        return BAD_COORD;                                                       //the coordinates are bad
    
    //note that at this point the vector startingSegs contains all of the street segments that begin with the starting point
    
    ExpandableHashMap<GeoCoord, GeoCoord> locationOfPreviousWayPoint;           //a map that can help backtrack the route from the end position to the start
        
    queue<GeoCoord> coordsToVisit;                                              //the queue enables a breadth first search of the map which
                                                                                //means that we will always find the shortest path
    coordsToVisit.push(start);                                                  //push the starting position onto the queue
    GeoCoord currCoord;
    while (!coordsToVisit.empty())                                              //run until there is no more coordinates left to visit
    {
        currCoord = coordsToVisit.front();
        coordsToVisit.pop();
        
        if (currCoord == end)                                                   //found the end
            break;
        
        m_streetmap->getSegmentsThatStartWith(currCoord, startingSegs);         //get the street segments that start with current coordinate
        for (int i=0; i< startingSegs.size(); i++)                              //for each of those street segments
        {
            if (locationOfPreviousWayPoint.find(startingSegs[i].end)==nullptr)  //if the end of that street segment has not already been visited
            {
                locationOfPreviousWayPoint.associate(startingSegs[i].end, currCoord); //mark off the end of the street segment as visited
                                                                                      //from the current coordinate
                coordsToVisit.push(startingSegs[i].end);                        //enqueue the end of the street segment to be visited
            }
        }
        startingSegs.clear();                                                   //empty the vector to ensure no residual segments in the next iteration
    }

    if (currCoord != end)                                                       //if the end coordinate could not be reached
        return NO_ROUTE;                                                        //there is no route between starting and ending coordinates
    
    //reaching here means that there is a viable route between the starting and ending coordinates
    //important to note that reaching here also means that currCoord is the end coord
    
    //starting back tracking
    GeoCoord prev = * locationOfPreviousWayPoint.find(end);                     //prev variable denotes the GeoCoord from which we came to a
                                                                                //certain coordiante. We start from the end coordinate.
    totalDistanceTravelled=0;
    
    while (currCoord!=start)                                                    //backtrack until the starting point is not reached
    {
        m_streetmap->getSegmentsThatStartWith(currCoord, startingSegs);
        int i;
        for (i=0; i<startingSegs.size(); i++)
            if (startingSegs[i].end == prev)                                    //find the street segment that starts at currCoord and ends at prev
                break;
        route.push_front(startingSegs[i]);                                      //push that street segment into the overall route

        totalDistanceTravelled+=distanceEarthMiles(startingSegs[i].start, startingSegs[i].end);
                                                                                //increase the total distance travelled for that streetsegment
        currCoord = prev;                                                       //set currCoord to the coord that got us to it
        if (locationOfPreviousWayPoint.find(prev) != nullptr)
            prev = * locationOfPreviousWayPoint.find(prev);                     //backtrack with the prev coord
    }
    
    return DELIVERY_SUCCESS;  
}

//******************** PointToPointRouter functions ***************************

// These functions simply delegate to PointToPointRouterImpl's functions.
// You probably don't want to change any of this code.

PointToPointRouter::PointToPointRouter(const StreetMap* sm)
{
    m_impl = new PointToPointRouterImpl(sm);
}

PointToPointRouter::~PointToPointRouter()
{
    delete m_impl;
}

DeliveryResult PointToPointRouter::generatePointToPointRoute(
        const GeoCoord& start,
        const GeoCoord& end,
        list<StreetSegment>& route,
        double& totalDistanceTravelled) const
{
    return m_impl->generatePointToPointRoute(start, end, route, totalDistanceTravelled);
}

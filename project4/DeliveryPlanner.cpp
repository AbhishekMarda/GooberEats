#include "provided.h"
#include <vector>
using namespace std;

class DeliveryPlannerImpl
{
public:
    DeliveryPlannerImpl(const StreetMap* sm);
    ~DeliveryPlannerImpl();
    DeliveryResult generateDeliveryPlan(
        const GeoCoord& depot,
        const vector<DeliveryRequest>& deliveries,
        vector<DeliveryCommand>& commands,
        double& totalDistanceTravelled) const;
private:
    const StreetMap* m_streetmap;
    string getDirName(double angle) const;
    string getTurnDir(double angle) const;
};

string DeliveryPlannerImpl::getDirName(double angle) const
{
    if (0<=angle && angle < 22.5) return "east";
    if (22.5<=angle && angle <67.5) return "northeast";
    if (67.5<=angle && angle <112.5) return "north";
    if (112.5<=angle && angle <157.5) return "northwest";
    if (157.5<=angle && angle <202.5) return "west";
    if (202.5<=angle && angle <247.5) return "southwest";
    if (247.5<=angle && angle <292.5) return "south";
    if (292.5<=angle && angle <337.5) return "southeast";
    return "east";
}

string DeliveryPlannerImpl::getTurnDir(double angle) const
{
    if (angle >=1 && angle< 180) return "left";
    if (angle >=180 && angle <=359) return "right";
    else return "";
}

DeliveryPlannerImpl::DeliveryPlannerImpl(const StreetMap* sm)
{
    m_streetmap = sm;
}

DeliveryPlannerImpl::~DeliveryPlannerImpl()
{
    
}

DeliveryResult DeliveryPlannerImpl::generateDeliveryPlan(
    const GeoCoord& depot,
    const vector<DeliveryRequest>& deliveries,
    vector<DeliveryCommand>& commands,
    double& totalDistanceTravelled) const
{
    {
        vector<StreetSegment> temp;
        if (!m_streetmap->getSegmentsThatStartWith(depot, temp))
            return BAD_COORD;
    }
    
    GeoCoord startCoord = depot;
    GeoCoord endCoord;
    string itemToBeDelivered;
    PointToPointRouter p2p(m_streetmap);
    
    vector<DeliveryRequest> deliverAndReturn = deliveries;                             //this vector will allow changes and can
                                                                                       //allow addition of the depot to the end
    double oldCrowDist, newCrowDist;
    DeliveryOptimizer myDO(m_streetmap);
    myDO.optimizeDeliveryOrder(depot, deliverAndReturn, oldCrowDist, newCrowDist);     //reorder to optimizing the path taken
    cerr<<"Old distance was: "<<oldCrowDist<<endl;
    cerr<<"New distance is: " << newCrowDist<<endl;
    
    deliverAndReturn.push_back(DeliveryRequest("", depot));                            //add the depot to the end of the deliveries
                                                                                       //since we have to return back there
    string currStreetName="";
    for (int i=0; i<deliverAndReturn.size(); i++)                                      //for every delivery
    {
        double dist;
        endCoord = deliverAndReturn[i].location;
        list<StreetSegment> route;
        //get route from previous delivery (or depot if its the first delivery) to the current delivery (or depot if its the last delivery)
        DeliveryResult delRes = p2p.generatePointToPointRoute(startCoord, endCoord, route, dist);
        if (delRes == NO_ROUTE)
            return NO_ROUTE;
        if (delRes == BAD_COORD)
            return BAD_COORD;
        
        itemToBeDelivered = deliverAndReturn[i].item;
        
        auto it = route.begin();
        double currStreetDistance=0;
        double currStreetAngle=0;
        
        for (; ; it++)                                                                  //for every street segment from the depot
        {
            if (currStreetName.empty())                                                 //the street name is empty every time a new street
                                                                                        //begins which allows resetting the angle the current
                                                                                        //street starts with
            {
                currStreetName = it->name;
                currStreetAngle = angleOfLine(*it);
            }
            
            if (++it == route.end())                                                    //if the next street segment is denotes a delivery
            {
                --it;
                if (!currStreetName.empty())
                {
                    currStreetDistance+= distanceEarthMiles(it->start, it->end);        //add the length of the currect street segment
                    DeliveryCommand d1;
                    d1.initAsProceedCommand(getDirName(currStreetAngle), currStreetName, currStreetDistance);
                    commands.push_back(d1);
                    totalDistanceTravelled+= currStreetDistance;                        //increase the total distance with the current
                                                                                        //street's length
                }
                if (i!=deliverAndReturn.size()-1)                                       //don't give a delivery command when we reach back to
                                                                                        //the depot, which is the last element of deliverAndReturn
                {
                    DeliveryCommand d2;
                    d2.initAsDeliverCommand(itemToBeDelivered);
                    commands.push_back(d2);
                }
                currStreetName="";                                                      //reset the name to empty so that a turn command is
                                                                                        //not issued right after a delivery (i.e. the next if
                                                                                        //statement is not entered)
                break;
            }
            --it;
            if (!currStreetName.empty() && currStreetName != it->name)                  //if a new street has begun
            {
                DeliveryCommand d1;
                d1.initAsProceedCommand(getDirName(currStreetAngle), currStreetName, currStreetDistance); //register a proceed command
                commands.push_back(d1);

                StreetSegment s1 = *it;
                it--;
                StreetSegment s2 = *it;
                it++;
                double turnAngle = angleBetween2Lines(s2, s1);                          //calculate the angle between previous and current street
                DeliveryCommand d;
                if (!getTurnDir(turnAngle).empty())                                     //if the car is not to travel straight (i.e. turn
                                                                                        //left or right)
                {
                    d.initAsTurnCommand(getTurnDir(turnAngle), it->name);               //register a turn command
                    commands.push_back(d);
                }
                totalDistanceTravelled+=currStreetDistance;                             //incremeent total distance by previous steet's length
                currStreetDistance=0;                                                   //reset current street distance for the new street
                currStreetAngle = angleOfLine(*it);                                     //reset the starting directin of the current street
                currStreetName = it->name;                                              //reset the name of the street
            }
            
            currStreetDistance+= distanceEarthMiles(it->start, it->end);                //add the distance of the current street segment
        }
        
        startCoord = endCoord;                              //shift to the next street segment
        
    }
    return DELIVERY_SUCCESS;
}

//******************** DeliveryPlanner functions ******************************

// These functions simply delegate to DeliveryPlannerImpl's functions.
// You probably don't want to change any of this code.

DeliveryPlanner::DeliveryPlanner(const StreetMap* sm)
{
    m_impl = new DeliveryPlannerImpl(sm);
}

DeliveryPlanner::~DeliveryPlanner()
{
    delete m_impl;
}

DeliveryResult DeliveryPlanner::generateDeliveryPlan(
    const GeoCoord& depot,
    const vector<DeliveryRequest>& deliveries,
    vector<DeliveryCommand>& commands,
    double& totalDistanceTravelled) const
{
    return m_impl->generateDeliveryPlan(depot, deliveries, commands, totalDistanceTravelled);
}

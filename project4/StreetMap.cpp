#include "provided.h"
#include "ExpandableHashMap.h"
#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

unsigned int hasher(const GeoCoord& g)
{
    return std::hash<string>()(g.latitudeText + g.longitudeText);
}

class StreetMapImpl
{
public:
    StreetMapImpl();
    ~StreetMapImpl();
    bool load(string mapFile);
    bool getSegmentsThatStartWith(const GeoCoord& gc, vector<StreetSegment>& segs) const;
private:
    ExpandableHashMap<GeoCoord, vector<StreetSegment>> m_map;               //maintain a private map that associates a coordinate to a
                                                                            //vector of street segments that start with it
};

StreetMapImpl::StreetMapImpl()
{
}

StreetMapImpl::~StreetMapImpl()
{
}

bool StreetMapImpl::load(string mapFile)
{
    ifstream inf(mapFile);
    if (!inf)
    {
        cerr << "MapFile not read in SteetMap.cpp load function"<<endl;
        return false;                                                       //if there is no file then it can't be loaded
    }
    
    string line;
    bool itsTheLineWithTheStreetName=true;
    bool itsTheLineWithTheCount = false;
    string name;
    int count=0;
    while (getline (inf , line))
    {
        istringstream iss(line);                                            //get the related line
        
        if (itsTheLineWithTheStreetName)                                    //for the line that contains the name of the street
        {
            name=line;                                                      //get the current street name
            itsTheLineWithTheCount=true;                                    //as per the given format, the next line will give the number
                                                                            //of segments that the street contains
            itsTheLineWithTheStreetName=false;
        }
        else if (itsTheLineWithTheCount)                                    //for the line that contains the no. of segments of a street
        {
            iss >> count;                                                   //store the count into the variable
            itsTheLineWithTheCount=false;
        }
        else
        {
            count--;
            string Coord1, Coord2, Coord3,Coord4;
            iss>>Coord1>>Coord2>>Coord3>> Coord4;                           //store the coordinates
            GeoCoord newGCoordS(Coord1, Coord2);                            //get the starting GeoCoord
            GeoCoord newGCoordE(Coord3, Coord4);                            //get the ending GeoCoord
        
            //there have to be two steet segments
            //one that starts from the starting point
            //one that starts from the ending point
            //so that a route can be mapped going along either direction on the street segment
            StreetSegment newStreetSegment1(newGCoordS, newGCoordE, name);
            StreetSegment newStreetSegment2(newGCoordE, newGCoordS, name);
            
            vector<StreetSegment> v;                                        //make a vector that will store the street segments that
                                                                            //start from the starting coordinate
            if (m_map.find(newGCoordS)!=nullptr)                            //if there are already segments that start from the starting coordinate
            {
                v=*m_map.find(newGCoordS);                                  //get the vector that contains those street segments
            }
            v.push_back(newStreetSegment1);                                 //add the street segments starting from the starting coordinate
            m_map.associate(newGCoordS, v);                                 //replace the existing vector with the new vector
            
            vector<StreetSegment> vrev;                                     //this vector will contain the street segments that start from
                                                                            //the ending coordinate
            if (m_map.find(newGCoordE)!=nullptr)                            //if there are already segments that start from the starting coordinate
             {
                 vrev=*m_map.find(newGCoordE);                              //get the vector that contains those street segments
             }
             vrev.push_back(newStreetSegment2);                             //add the street segments starting from the ending coordinate
             m_map.associate(newGCoordE, vrev);                             //replace the existing vector with the new vector
          
            if (count==0)                                                   //when the there are no more segments left for that street
            {
                itsTheLineWithTheStreetName=true;                           //the next line will contain the next street's name
            }
         }
    }
    return true;
}

bool StreetMapImpl::getSegmentsThatStartWith(const GeoCoord& gc, vector<StreetSegment>& segs) const
{
    if (m_map.find(gc) == nullptr)                                          //if there are no street segments that start with the geocoord
        return false;
    vector<StreetSegment> v = *m_map.find(gc);                              //else set segs to the vector that contains the street segments
    segs = v;
    return true;
}

//******************** StreetMap functions ************************************

// These functions simply delegate to StreetMapImpl's functions.
// You probably don't want to change any of this code.

StreetMap::StreetMap()
{
    m_impl = new StreetMapImpl;
}

StreetMap::~StreetMap()
{
    delete m_impl;
}

bool StreetMap::load(string mapFile)
{
    return m_impl->load(mapFile);
}

bool StreetMap::getSegmentsThatStartWith(const GeoCoord& gc, vector<StreetSegment>& segs) const
{
   return m_impl->getSegmentsThatStartWith(gc, segs);
}

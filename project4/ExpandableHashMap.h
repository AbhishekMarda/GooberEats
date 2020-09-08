

#ifndef ExpandableHashMap_h
#define ExpandableHashMap_h
#include <list>
#include <iostream>
template<typename KeyType, typename ValueType>
class ExpandableHashMap
{
public:
    ExpandableHashMap(double maximumLoadFactor = 0.5);
    ~ExpandableHashMap();
    void reset();
    int size() const;
    void associate(const KeyType& key, const ValueType& value);
    const ValueType* find(const KeyType& key) const;
    ValueType* find(const KeyType& key)
    {
        return const_cast<ValueType*>(const_cast<const ExpandableHashMap*>(this)->find(key));
    }
    ExpandableHashMap(const ExpandableHashMap&) = delete;
    ExpandableHashMap& operator=(const ExpandableHashMap&) = delete;

private:
    struct Node
    {
        KeyType k;
        ValueType v;
    };
    
    double m_maxLoadFactor;         //stores the maximum load factor
    int m_nSlots;                   //stores the total buckets available, both filled and unfilled
    std::list<Node>** m_hashTable;  //stores the pointer to the hash table which is an array of buckets of pointers to lists
    int m_filledBuckets;            //stores the number of filled buckets
    int m_associations;             //stores the number of Nodes inserted in the hash table
    
};

template<typename KeyType, typename ValueType>
ExpandableHashMap<KeyType, ValueType>::ExpandableHashMap(double maximumLoadFactor)
{
    if (maximumLoadFactor > 0 && maximumLoadFactor <= 1)
        m_maxLoadFactor = maximumLoadFactor;
    else
        m_maxLoadFactor = 0.5;
    m_nSlots = 8;
    
    m_hashTable = new std::list<Node>*[m_nSlots];
    for (int i = 0; i < m_nSlots; i++)                          //initialize all of the buckets to nullptr
        *(m_hashTable+i) = nullptr;
    
    m_filledBuckets=0;
    m_associations=0;
    
}

template<typename KeyType, typename ValueType>
ExpandableHashMap<KeyType, ValueType>::~ExpandableHashMap()
{
    for (int i=0; i<m_nSlots; i++)
    {
        if (m_hashTable[i] != nullptr)
            delete m_hashTable[i];                              //delete every bucket in the hash table
    }
    delete [] m_hashTable;                                      //then delete the array of buckets
}

template<typename KeyType, typename ValueType>
void ExpandableHashMap<KeyType, ValueType>::reset()
{
    ~ExpandableHashMap();                                       //destroy the table
    m_nSlots = 8;                                               //and create a new one with 8 slots
    m_hashTable = new std::list<Node>*[m_nSlots];
    for (int i = 0; i < m_nSlots; i++)                          //initialize the buckets to nullptr
        *(m_hashTable+i) = nullptr;
    m_filledBuckets=0;
    m_associations=0;
}

template<typename KeyType, typename ValueType>
int ExpandableHashMap<KeyType, ValueType>::size() const
{
    return m_associations;
}

template<typename KeyType, typename ValueType>
void ExpandableHashMap<KeyType, ValueType>::associate(const KeyType& key, const ValueType& value)
{


    ValueType* valptr = find(key);
    if (valptr!=nullptr)                                        //value was found
    {
        *valptr = value;                                        //replace the value
        return;
    }
    unsigned int hasher(const KeyType& key);
    unsigned int index = hasher(key) % m_nSlots;

    //reaching this point means that we would be adding a new association
    Node newBucket;                                             //create a new bucket
    newBucket.k = key;                                          //with the appropriate key
    newBucket.v = value;                                        //and value
    
    //check the load factor after insertion of the Node
    double loadFactorAfterInsertion = ((double) (size())+1)/ (double)m_nSlots;
    
    //if the load factor after insertion is greater than the initial load factor, rehash the has table with double the size
    if (loadFactorAfterInsertion > m_maxLoadFactor)
    {
        int newNSlots = m_nSlots*2;                                                 //double the size
        m_filledBuckets= 0;
        m_associations=0;
        
        std::list<Node>** newhash = new std::list<Node>*[newNSlots];                //create a new hash table with the new size
        
        for (int i=0; i<newNSlots; i++)                                             //initialize buckets to nullptr
            newhash[i] = nullptr;
        
        for (int i=0; i<m_nSlots; i++)                                              //rehash the associations in prev hash table into new hash table
        {
            if (m_hashTable[i]==nullptr)
                continue;
            
            typename std::list<Node>::iterator it;
            for (it= (*m_hashTable[i]).begin(); it != (*m_hashTable[i]).end(); it++)//for ever association in every bucket in the hash table
            {
                Node newRehashbucket = *it;
                unsigned int newIndex = hasher(newRehashbucket.k)%newNSlots;        //get the new index for the association
                //std::cerr<<newIndex<<std::endl;
                if (newhash[newIndex]==nullptr)                                     //if the bucket at the new index is nullptr
                {
                    newhash[newIndex]= new std::list<Node>;                         //make a new list
                    newhash[newIndex]->push_back(newRehashbucket);                  //push back the value into that list
                    m_filledBuckets++;
                    m_associations++;
                }
                else
                {                                                                   //if there is a collision
                    (*newhash[newIndex]).push_back(newRehashbucket);                //push back on the existing list at the bucket
                    m_associations++;
                }
            }
        }
        
                                                                                    //delete the old hash table
        for (int i=0; i<m_nSlots; i++)
        {
            delete m_hashTable[i];
        }
        delete [] m_hashTable;
            
        
                                                                                    //reset the new hash table
        m_hashTable = newhash;
        m_nSlots=newNSlots;
    }
    
    //proceed to add the new association into the hash table
    index = hasher(key) % m_nSlots;                                                 //get an index for the new association
    if (m_hashTable[index]==nullptr)                                                //if bucket at index is a nullptr
    {
        m_hashTable[index]= new std::list<Node>;                                    //make a new list
        m_hashTable[index]->push_back(newBucket);                                   //add association to the list
        m_filledBuckets++;
        m_associations++;

    }
    else
    {
                                                                                    //there is a collision with the new association
        (*m_hashTable[index]).push_back(newBucket);                                 //then push back on the existing list
        m_associations++;
    }
}

template<typename KeyType, typename ValueType>
const ValueType* ExpandableHashMap<KeyType, ValueType>::find(const KeyType& key) const
{
    unsigned int hasher(const KeyType& key);
    unsigned int index = hasher(key) % m_nSlots;                                    //get the index from the hash function for the key
    
    std::list<Node>* concernedList = m_hashTable[index];
    if (concernedList==nullptr)                                                     //if there is no list at the bucket
        return nullptr;                                                             //then there is no association with the key
    
    typename std::list<Node>::iterator it;
    it = (*concernedList).begin();
    while (it != (*concernedList).end())                                            //go through the entire list at the bucket
    {
        if (it->k == key)                                                           //return the pointer to the value if there is a matching key
            return &(it->v);
        it++;
    }
    return nullptr;                                                                 //return nullptr if there is no matching key in the list
}

#endif /* ExpandableHashMap_hpp */

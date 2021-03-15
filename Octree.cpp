#include "Octree.h"
#include "Box.h"

#include <glm/gtc/random.hpp>
#include <iostream>

// calculate bounds of specified quadrant in bounding region
void Octree::calculateBounds(BoundingRegion& out, Octant octant, BoundingRegion parentRegion) {
    // find min and max points of corresponding octant

    glm::vec3 center = parentRegion.calculateCenter();
    if (octant == Octant::O1) {
        out = BoundingRegion(center, parentRegion.max);
    }
    else if (octant == Octant::O2) {
        out = BoundingRegion(glm::vec3(parentRegion.min.x, center.y, center.z), glm::vec3(center.x, parentRegion.max.y, parentRegion.max.z));
    }
    else if (octant == Octant::O3) {
        out = BoundingRegion(glm::vec3(parentRegion.min.x, parentRegion.min.y, center.z), glm::vec3(center.x, center.y, parentRegion.max.z));
    }
    else if (octant == Octant::O4) {
        out = BoundingRegion(glm::vec3(center.x, parentRegion.min.y, center.z), glm::vec3(parentRegion.max.x, center.y, parentRegion.max.z));
    }
    else if (octant == Octant::O5) {
        out = BoundingRegion(glm::vec3(center.x, center.y, parentRegion.min.z), glm::vec3(parentRegion.max.x, parentRegion.max.y, center.z));
    }
    else if (octant == Octant::O6) {
        out = BoundingRegion(glm::vec3(parentRegion.min.x, center.y, parentRegion.min.z), glm::vec3(center.x, parentRegion.max.y, center.z));
    }
    else if (octant == Octant::O7) {
        out = BoundingRegion(parentRegion.min, center);
    }
    else if (octant == Octant::O8) {
        out = BoundingRegion(glm::vec3(center.x, parentRegion.min.y, parentRegion.min.z), glm::vec3(parentRegion.max.x, center.y, center.z));
    }
}


Octree::Node::Node()
    : region(BoundTypes::AABB){}
Octree::Node::Node(BoundingRegion bounds) 
    : region(bounds){}
Octree::Node::Node(BoundingRegion bounds, std::vector<BoundingRegion> objectList) 
    : region(bounds)
{
    objects.insert(objects.end(), objectList.begin(), objectList.end());
}

void Octree::Node::addToPending(BoundingRegion bounds)
{
    pendingQueue.push(bounds); // simple boundsPush for now
    treeBuilt = false;
    treeReady = false;
}

void Octree::Node::Search(BoundingRegion& bounds, std::vector<glm::vec3>& points)
{
    if (!hasChildren && objects.size() == 0)
        return;
    if (!region.intersectsWith(bounds))
        return;
    else
    {
        if (hasChildren)
        {
            for (int i = 0; i < NR_CHILDREN; i++)
            {
                if(children[i] != nullptr)
                   children[i]->Search(bounds, points);
            }
        }

        for (int i = 0; i < objects.size(); i++)
        {
            if (bounds.containsPoint(objects[i].instance->position))
            {
                points.push_back(objects[i].instance->position);
            }
        }
    }
}

void Octree::Node::Build()
{
    BoundingRegion octants[NR_CHILDREN];
    glm::vec3 dimensions = region.calculateDimensions();

    if (objects.size() <= 1)
        goto setVars;
    
    for (int i = 0; i < 3; i++)
        if (dimensions[i] < MIN_BOUNDS)
            goto setVars;

    // create and populate each octant IFF HAS objects
    for (int i = 0; i < NR_CHILDREN; i++)
    {
        // for each sub-octant, calculate its BoundingRegion - store in octants[i]
        calculateBounds(octants[i], (Octant)(1 << i), region);    
        for (int j = objects.size() - 1; j >= 0; j--) 
        {
            BoundingRegion tmp = objects[j];
            tmp.transform();
            if (octants[i].containsRegion(tmp))
            {
                if (children[i] == nullptr)
                {
                    children[i] = new Node(octants[i]);
                    children[i]->region.debugColor = glm::vec3(glm::linearRand(0.1f, 1.0f), glm::linearRand(0.1f, 1.0f), glm::linearRand(0.1f, 1.0f));
                    hasChildren = true;
                }
           
                // handover data from 'this' octant -> sub-octant
                children[i]->objects.push_back(objects[j]);
                objects.erase(objects.begin() + j);

                // mark sub-octant as active and set 'this' as parent
                states::activateIndex(&activeOctants, i);
                children[i]->parent = this;

                // repeat process in sub-octant until termination condtions above are met
                children[i]->Build();
            }
        }
    }

setVars:
    treeBuilt = true;
    treeReady = true;

    for (int i = 0; i < objects.size(); i++) 
        objects[i].octreeNode = this;
}

int Octree::Node::TotalInserted()
{
    int total = objects.size();

    for (int i = 0; i < NR_CHILDREN; i++)
    {
        if (children[i] != nullptr)
            total += children[i]->TotalInserted();
    }

    return total;
}


bool Octree::Node::RemoveStaleBranches()
{
    for (int i = 0; i < NR_CHILDREN; i++)
    {
        if (children[i] != nullptr)
        {
            bool isLeaf = children[i]->RemoveStaleBranches();

            if(isLeaf && children[i]->objects.size() == 0)
            {
                delete children[i];
                children[i] = nullptr;
            }
        }
    }

    for (int i = 0; i < NR_CHILDREN; i++)
    {
        if (children[i] != nullptr)
            return false;
    }
    return true;
}

float random(float min, float max)
{
    assert(max > min);
    float random = ((float)rand()) / (float)RAND_MAX;
    float range = max - min;
    return (random * range) + min;
}


void Octree::Node::Draw(Box& box)
{
    if (treeBuilt && treeReady)
    {
        box.positions.push_back(region.calculateCenter());
        box.sizes.push_back(region.calculateDimensions());

        glm::vec3 color = region.debugColor;

        box.colors.push_back(color);

        bool isLeaf = true;
        for (int i = 0; i < objects.size(); i++)
        {
            objects[i].transform();
            box.positions.push_back(objects[i].calculateCenter());
            box.sizes.push_back(objects[i].calculateDimensions());
            box.colors.push_back(color);
        }

        for (int i = 0; i < NR_CHILDREN; i++)
        {
            if (children[i] != nullptr)
            {
                children[i]->Draw(box);
            }
        }

    }
}

void Octree::Node::ProcessPending()
{
    if (!treeBuilt)
    {
        // add the objects to be sorted into branches when built
        while (pendingQueue.size() != 0)
        {
            objects.push_back(pendingQueue.front());
            pendingQueue.pop();
        }
        Build();
    }
    else
    {
        for (int i = 0, len = pendingQueue.size(); i < len; i++) {
            BoundingRegion br = pendingQueue.front();
            if (region.containsRegion(br)) {
                // insert object immediately
                Insert(br);
            }
            else
            {
                // return to queue
                //br.transform();
                pendingQueue.push(br);
            }
            pendingQueue.pop();
        }
    }
}

void Octree::Node::Update()
{
    if (treeBuilt && treeReady) 
    {
        // update child nodes
        if (children != nullptr) 
        {
            // go through each octant using flags
            for (unsigned char flags = activeOctants, i = 0;
                flags > 0;
                flags >>= 1, i++) {
                if (states::isIndexActive(&flags, 0)) 
                {
                    // active octant
                    if (children[i] != nullptr) 
                    {
                        // child not null
                        children[i]->Update();
                    }
                }
            }
        }

        std::vector<BoundingRegion> toRelocate;
        // handover objects to root node
        for (int i = objects.size() - 1; i >= 0; i--)
        {
            if (states::isActive(&objects[i].instance->state, INSTANCE_MOVED))
            {
                toRelocate.push_back(objects[i]);
                objects.erase(objects.begin() + i);
            }
        }
        
        Node* current = this;
        for (int i = 0; i < toRelocate.size(); i++)
        {
            while (!current->region.containsRegion(toRelocate[i])) 
            {
                if (current->parent != nullptr)
                    current = current->parent;
                else 
                    break; // if root node, the leave
            }
        
            current->Insert(toRelocate[i]);
        }
        toRelocate.clear();
        
        this->RemoveStaleBranches();
    }
    else 
    {
        // process pending results
        if (pendingQueue.size() > 0) 
        {
            ProcessPending();
        }
    }
}

bool Octree::Node::Insert(BoundingRegion obj)
{
    glm::vec3 dimensions = region.calculateDimensions();
    if (!hasChildren && objects.size() == 0)
    {
        obj.octreeNode = this;
        objects.push_back(obj);
        return true;
    }
    
    BoundingRegion tmp = obj;
    tmp.transform();
    // safeguard if object does not fit
    if (!region.containsRegion(tmp))
    {
        if (parent != nullptr)
        {
            parent->objects.push_back(obj);
            return true;
        }

        return parent == nullptr ? false : parent->Insert(obj);
    }
    
    objects.push_back(obj);

    // create regions if not defined
    BoundingRegion octants[NR_CHILDREN];
    for (int i = 0; i < NR_CHILDREN; i++)
    {
        if (children[i] != nullptr)
        {
            octants[i] = children[i]->region;
        }
        else
        {
            calculateBounds(octants[i], (Octant)(1 << i), region);
        }
    }
       
    // determine which octants to put objects in
    std::vector<BoundingRegion> octLists[NR_CHILDREN]; // array of lists of objects in each octant
    for (int i = 0, len = objects.size(); i < len; i++)
    {
        objects[i].octreeNode = this;
        for (int j = 0; j < NR_CHILDREN; j++)
        {
            if (octants[j].containsRegion(objects[i]))
            {
                octLists[j].push_back(objects[i]);
                // remove from objects list
                objects.erase(objects.begin() + i);
                i--;
                len--;
                break;
            }
        }
    }
    
    // populate octants
    for (int i = 0; i < NR_CHILDREN; i++) {
        if (octLists[i].size() != 0) 
        {
            // objects exist in this octant
            if (children[i] != nullptr) 
            {
                for (const BoundingRegion& br : octLists[i]) 
                {
                    children[i]->Insert(br);
                }
            }
            else 
            {
                // create new node
                children[i] = new Node(octants[i], octLists[i]);
                children[i]->region.debugColor = glm::vec3(glm::linearRand(0.1f, 1.0f), glm::linearRand(0.1f, 1.0f), glm::linearRand(0.4f, 1.0f));
                children[i]->parent = this;
                states::activateIndex(&activeOctants, i);
                children[i]->Build();
                hasChildren = true;
            }
        }
    }

    return true;
}
void Octree::Node::Destroy()
{
    // clearing out children
    if (children != nullptr) 
    {
        for (int flags = activeOctants, i = 0;
            flags > 0;
            flags >>= 1, i++) 
        {
            if (states::isIndexActive(&flags, 0)) 
            {
                // active
                if (children[i] != nullptr) {
                    children[i]->Destroy();
                    children[i] = nullptr;
                    delete children[i];
                }
            }
        }
    }

    // clear THIS node
    objects.clear();
    while (pendingQueue.size() != 0)
    {
        pendingQueue.pop(); 
    }
    hasChildren = false;
}
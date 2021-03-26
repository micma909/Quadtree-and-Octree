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
    int uniqueId = bounds.instance->id;
    if (instanceIDs.find(uniqueId) == instanceIDs.end())
    {
        instanceIDs.insert(bounds.instance->id);
        pendingQueue.push(bounds); // simple boundsPush for now
        treeBuilt = false;
        treeReady = false;
    }
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

bool Octree::Node::RemoveStaleBranches()
{
    for (int i = 0; i < NR_CHILDREN; i++)
    {
        if (children[i] != nullptr)
            children[i]->RemoveStaleBranches();
    }

    for (int i = 0; i < NR_CHILDREN; i++)
    {
        if (children[i] != nullptr)
        {
            bool isLeaf = true;

            for (int j = 0; j < NR_CHILDREN; j++)
            {
                //check if leafnode
                if (children[i]->children[j] != nullptr)
                {
                    isLeaf = false;
                    break;
                }
            }
            bool isEmpty = children[i]->objects.size() == 0;

            if (isLeaf && isEmpty)
            {
                states::deactivateIndex(&activeOctants, i);
                delete children[i];
                children[i] = nullptr;
            }
        }
    }
    return true;
}

void Octree::Node::Draw(Box& box)
{
    if (treeBuilt && treeReady)
    {
        box.positions.push_back(region.calculateCenter());
        box.sizes.push_back(region.calculateDimensions());

        glm::vec4 color = region.debugColor;

        if (level)
            color.w = 1.0f - ((float)(10.0f - level)) / 10.0f;
       
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
                if (children[i]->objects.size() == 0 && !children[i]->hasChildren)
                {
                    children[i]->region.debugColor = glm::vec4(1, 0, 0, 1);
                }

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
       //for (int i = 0, len = pendingQueue.size(); i < len; i++) {
       //    BoundingRegion br = pendingQueue.front();
       //    if (region.containsRegion(br)) {
       //        // insert object immediately
       //        Insert(br);
       //    }
       //    else
       //    {
       //        // return to queue
       //        pendingQueue.push(br);
       //    }
       //    pendingQueue.pop();
       //}
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
        {
            if (parent != nullptr)
            {
                for (int j = 0; j < objects.size(); j++)
                    parent->objects.push_back(objects[j]);
                goto setVars;
            }
        }
          
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
                    children[i]->level = this->level + 1;
                    children[i]->region.debugColor = glm::vec4(0, 1, 1, 1); // glm::vec4(glm::linearRand(0.1f, 1.0f), glm::linearRand(0.1f, 1.0f), glm::linearRand(0.1f, 1.0f), 1);
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


void Octree::Node::Update()
{
    if (treeBuilt && treeReady) 
    {
        activeChildren = 0;

        for (int i = 0; i < NR_CHILDREN; i++)
        {
            if (children[i] != nullptr)
                activeChildren++;
        }

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

        // handover objects to root node
        std::vector<BoundingRegion> toRelocate;
        bool shouldDelegate = (hasChildren && objects.size() > 0);

        // simplify from previous frame
        if (activeChildren == 1)
        {
            for (int i = 0; i < NR_CHILDREN; i++)
            {
                if (children[i] != nullptr && !children[i]->hasChildren)
                {
                    for (int j = 0; j < children[i]->objects.size(); j++)
                    {
                        objects.push_back(children[i]->objects[j]);
                        children[i]->objects.erase(children[i]->objects.begin() + j);
                    }
                    states::deactivateIndex(&activeOctants, i);
                    delete children[i];
                    children[i] = nullptr;
                    hasChildren = false;

                    activeChildren--;
                }
            }
        }

        for (int i = objects.size() - 1; i >= 0; i--)
        {
            bool hasMoved = states::isActive(&objects[i].instance->state, INSTANCE_MOVED);
            bool noLongerFits = !region.containsRegion(objects[i]);
            if (hasMoved || shouldDelegate || noLongerFits)
            {
                toRelocate.push_back(objects[i]);
                objects.erase(objects.begin() + i);
            }
        }

        this->RemoveStaleBranches();

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
    BoundingRegion tmp2 = obj;
    tmp2.transform();
    if (level == 0 && !region.containsRegion(tmp2))
    {
        std::set<int>::iterator it = instanceIDs.find(obj.instance->id);
        instanceIDs.erase(it, instanceIDs.end());

        return false; // TODO fix this 
    }


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
            obj.octreeNode = parent;
            return true;
        }
        else
        {
            return false;
        }
    }

    objects.push_back(obj);
    obj.octreeNode = this;

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
                children[i]->level = this->level + 1;
                children[i]->region.debugColor = glm::vec4(0, 1, 1, 1); // glm::vec4(glm::linearRand(0.1f, 1.0f), glm::linearRand(0.1f, 1.0f), glm::linearRand(0.4f, 1.0f), 1);
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
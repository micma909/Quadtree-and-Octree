#include "Octree.h"
#include "Box.h"

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
    // TODO: 
    // Currently will rebuild entire Octree on each drawcall 
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
        for (int objectIndex = objects.size() - 1; objectIndex >= 0; objectIndex--) 
        {
            BoundingRegion br = objects[objectIndex];
            if (octants[i].containsRegion(br))
            {
                if(children[i] == nullptr)
                    children[i] = new Node(octants[i]);

                // handover data from 'this' octant -> sub-octant
                children[i]->objects.push_back(br);
                objects.erase(objects.begin() + objectIndex);

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
void Octree::Node::Update(Box& box)
{
    if (treeBuilt && treeReady) 
    {
        box.positions.push_back(region.calculateCenter());
        box.sizes.push_back(region.calculateDimensions());

        // countdown timer
        if (objects.size() == 0) 
        {
            if (!hasChildren) 
{
                // ensure no child leaves
                if (currentLifespan == -1) 
                {
                    // initial check
                    currentLifespan = maxLifespan;
                }
                else if (currentLifespan > 0) 
                {
                    // decrement
                    currentLifespan--;
                }
            }
        }
        else 
        {
            if (currentLifespan != -1) 
            {
                if (maxLifespan <= 64) 
                {
                    // extend lifespan because "hotspot"
                    maxLifespan <<= 2;
                }
            }
        }

        // remove objects that don't exist anymore
        for (int i = 0, listSize = objects.size(); i < listSize; i++) 
        {
            // remove if kill switch active
            if (states::isActive(&objects[i].state, INSTANCE_DEAD)) 
            {
                objects.erase(objects.begin() + i);
                // offset because removed item from list
                i--;
                listSize--;
            }
        }

        // get moved objects that were in this leaf in previous frame
        std::stack<int> movedObjects;
        for (int i = 0, listSize = objects.size(); i < listSize; i++) 
        {
            if (states::isActive(&objects[i].state, INSTANCE_MOVED)) 
            {
                // if moved switch active, transform region and push to list
                // objects[i].transform();
                movedObjects.push(i);
            }
            box.positions.push_back(objects[i].calculateCenter());
            box.sizes.push_back(objects[i].calculateDimensions());
        }

        // remove dead branches
        unsigned char flags = activeOctants;
        for (int i = 0;
            flags > 0;
            flags >>= 1, i++) 
        {
            if (children[i] != nullptr)
            {
                if (states::isIndexActive(&flags, 0) && children[i]->currentLifespan == 0) {
                    // active and run out of time
                    if (children[i]->objects.size() > 0) {
                        // branch is dead but has children, so reset
                        children[i]->currentLifespan = -1;
                    }
                    else 
                    {
                        // branch is dead
                        children[i] = nullptr;
                        states::deactivateIndex(&activeOctants, i);
                    }
                }
            }
        }

        // update child nodes
        if (children != nullptr) {
            // go through each octant using flags
            for (unsigned char flags = activeOctants, i = 0;
                flags > 0;
                flags >>= 1, i++) {
                if (states::isIndexActive(&flags, 0)) {
                    // active octant
                    if (children[i] != nullptr) {
                        // child not null
                        children[i]->Update(box);
                    }
                }
            }
        }

        // move moved objects into new nodes
        BoundingRegion movedObj; // placeholder
        while (movedObjects.size() != 0) 
        {
            /*
                for each moved object
                - traverse up tree (start with current node) until find a node that completely encloses the object
                - call insert (push object as far down as possible)
            */

            movedObj = objects[movedObjects.top()]; // set to top object in stack
            Node* current = this; // placeholder

            while (!current->region.containsRegion(movedObj)) {
                if (current->parent != nullptr) {
                    // set current to current's parent (recursion)
                    current = current->parent;
                }
                else {
                    break; // if root node, the leave
                }
            }

            /*
                once finished
                - remove from objects list
                - remove from movedObjects stack
                - insert into found region
            */
            objects.erase(objects.begin() + movedObjects.top());
            movedObjects.pop();
            current->Insert(movedObj);

            // collision detection
            // itself
            current = movedObj.octreeNode;
            //current->checkCollisionsSelf(movedObj);
            //
            //// children
            //current->checkCollisionsChildren(movedObj);
            //
            //// parents
            //while (current->parent) {
            //    current = current->parent;
            //    current->checkCollisionsSelf(movedObj);
            //}
        }
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
               // br.transform();
                pendingQueue.push(br);
            }
            pendingQueue.pop();
        }
    }
}
bool Octree::Node::Insert(BoundingRegion obj)
{
    /*
        termination conditions
        - no objects (an empty leaf node)
        - if dimensions are less than MIN_BOUNDS
    */
    glm::vec3 dimensions = region.calculateDimensions();
    if (objects.size() == 0 || (
        dimensions.x < MIN_BOUNDS ||
        dimensions.y < MIN_BOUNDS ||
        dimensions.z < MIN_BOUNDS))
    {
        obj.octreeNode = this;
        objects.push_back(obj);
        return true;
    }
    
    // safeguard if object does not fit
    if (!region.containsRegion(obj))
    {
        return parent == nullptr ? false : parent->Insert(obj);
    }

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

    objects.push_back(obj);

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
        if (octLists[i].size() != 0) {
            // objects exist in this octant
            if (children[i]) {
                for (BoundingRegion br : octLists[i]) {
                    children[i]->Insert(br);
                }
            }
            else 
            {
                // create new node
                children[i] = new Node(octants[i], octLists[i]);
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
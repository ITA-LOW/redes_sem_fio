#include "DynamicMeshingRouting.h"

uint32_t receiverNode = 761895598;

DynamicMeshingRouting::DynamicMeshingRouting() 
    : taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, [this](){ sendMessage(mesh.getNodeId()); }) {}

void DynamicMeshingRouting::setup() {
    Serial.begin(115200);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive([this](uint32_t from, String &msg){ receivedCallback(from, msg); });
    mesh.onNewConnection([this](uint32_t nodeId){ newConnectionCallback(nodeId); });
    mesh.onDroppedConnection([this](uint32_t nodeId){ droppedConnectionCallback(nodeId); });

    userScheduler.addTask(taskSendMessage);
    taskSendMessage.enable();

    if (mesh.getNodeId() != receiverNode) {
        taskSendMessage.enable();  // Only enable sending for non-receiver nodes
    }
}

void DynamicMeshingRouting::loop() {
    mesh.update();
}

void DynamicMeshingRouting::sendMessage(uint32_t nodeId) {
    // Only non-receiver nodes should broadcast messages
    if (nodeId == receiverNode) {
        return;
    }

    String msg = "Hello from Node " + String(nodeId);
    mesh.sendBroadcast(msg);
}


void DynamicMeshingRouting::receivedCallback(uint32_t from, String &msg) {
    Serial.printf("[node %u] %s\n", from, msg.c_str());

    if (mesh.getNodeId() == receiverNode) {
        // If this is the receiver, print the full path
        Serial.println("Path: " + msg.substring(16));
        Serial.println();
    } else {
        // If this is a forwarder, append this node's ID to the path and forward the message
        String newPath = msg + " -> " + String(mesh.getNodeId());
        uint32_t nextNode = findBestPath(mesh.getNodeId(), receiverNode);  // Find next hop to the receiver
        if (nextNode != -1) {
            mesh.sendSingle(nextNode, newPath);}
        /* } else {
            Serial.println("No available path.");
        } */
    }

    //printCostMatrix();

    Serial.println();
}


void DynamicMeshingRouting::newConnectionCallback(uint32_t nodeId) {
    //Serial.printf("New connection with node %u", nodeId);
    initializeNodeCosts(nodeId);

    // Assign the base station role to the receiver node only
    if (nodeId == receiverNode) {
        nodeRoles[nodeId] = "receiver";
    } else {
        nodeRoles[nodeId] = "sender";
    }

    recalculatePaths();  
}

void DynamicMeshingRouting::droppedConnectionCallback(uint32_t nodeId) {
    //Serial.printf("Node %u disconnected\n", nodeId);
    removeNodeCosts(nodeId);

    // Remove node role
    nodeRoles.erase(nodeId);

    recalculatePaths();  
}

uint32_t DynamicMeshingRouting::findBestPath(uint32_t from, uint32_t to) {
    std::map<uint32_t, int> distances;
    std::map<uint32_t, uint32_t> previous;
    std::vector<uint32_t> unvisited;

    for (const auto &node : mesh.getNodeList()) {
        distances[node] = (node == from) ? 0 : INT_MAX;
        unvisited.push_back(node);
    }

    while (!unvisited.empty()) {
        uint32_t currentNode = unvisited.front();
        for (auto node : unvisited) {
            if (distances[node] < distances[currentNode]) {
                currentNode = node;
            }
        }

        unvisited.erase(std::remove(unvisited.begin(), unvisited.end(), currentNode), unvisited.end());

        if (currentNode == to) {
            // Reconstruir o caminho completo de "to" até "from" usando o mapa `previous`
            std::vector<uint32_t> fullPath;
            while (currentNode != from) {
                fullPath.push_back(currentNode);
                currentNode = previous[currentNode];
            }
            fullPath.push_back(from);  // Adicionar o nó de partida
            std::reverse(fullPath.begin(), fullPath.end());  // Inverter para obter a trajetória do início ao fim

            paths[to] = fullPath;  // Armazenar o caminho completo em `paths[to]`
            return to;
        }

        for (const auto &neighbor : costs[currentNode]) {
            int newDist = distances[currentNode] + neighbor.second; 
            if (newDist < distances[neighbor.first]) {
                distances[neighbor.first] = newDist;
                previous[neighbor.first] = currentNode;
            }
        }
    }

    return -1;  // Retorna -1 se não encontrar um caminho
}

/* talvez essa linha consiga imprimir a rota
uint32_t DynamicMeshingRouting::findBestPath(uint32_t from, uint32_t to) {
    std::map<uint32_t, int> distances;
    std::map<uint32_t, uint32_t> previous;
    std::vector<uint32_t> unvisited;

    for (const auto &node : mesh.getNodeList()) {
        distances[node] = (node == from) ? 0 : INT_MAX;
        unvisited.push_back(node);
    }

    while (!unvisited.empty()) {
        uint32_t currentNode = unvisited.front();
        for (auto node : unvisited) {
            if (distances[node] < distances[currentNode]) {
                currentNode = node;
            }
        }

        unvisited.erase(std::remove(unvisited.begin(), unvisited.end(), currentNode), unvisited.end());

        if (currentNode == to) {
            // Reconstruir o caminho completo de "to" até "from" usando o mapa `previous`
            std::vector<uint32_t> fullPath;
            while (currentNode != from) {
                fullPath.push_back(currentNode);
                currentNode = previous[currentNode];
            }
            fullPath.push_back(from);  // Adicionar o nó de partida
            std::reverse(fullPath.begin(), fullPath.end());  // Inverter para obter a trajetória do início ao fim

            paths[to] = fullPath;  // Armazenar o caminho completo em `paths[to]`

            // Imprimir o caminho completo
            Serial.print("Caminho completo de ");
            Serial.print(from);
            Serial.print(" até ");
            Serial.print(to);
            Serial.print(": ");
            for (const auto& node : fullPath) {
                Serial.print(node);
                if (node != to) {
                    Serial.print(" -> ");
                }
            }
            Serial.println();

            return to;
        }

        for (const auto &neighbor : costs[currentNode]) {
            int newDist = distances[currentNode] + neighbor.second; 
            if (newDist < distances[neighbor.first]) {
                distances[neighbor.first] = newDist;
                previous[neighbor.first] = currentNode;
            }
        }
    }

    return -1;  // Retorna -1 se não encontrar um caminho
}

*/


void DynamicMeshingRouting::increaseCost(uint32_t from, uint32_t to) {
    costs[from][to] += 1;
    costs[to][from] = costs[from][to];
}

void DynamicMeshingRouting::initializeNodeCosts(uint32_t newNode) {
    for (auto &entry : costs) {
        costs[newNode][entry.first] = 1;
        costs[entry.first][newNode] = 1;
    }
}

void DynamicMeshingRouting::removeNodeCosts(uint32_t nodeId) {
    costs.erase(nodeId);
    for (auto &entry : costs) {
        entry.second.erase(nodeId);
    }
    Serial.printf("Node %u removed from cost matrix\n", nodeId);
}

void DynamicMeshingRouting::recalculatePaths() {
    for (const auto &startNode : mesh.getNodeList()) {
        for (const auto &endNode : mesh.getNodeList()) {
            if (startNode != endNode) {
                findBestPath(startNode, endNode);
            }
        }
    }
}

void DynamicMeshingRouting::printPath(uint32_t from, uint32_t to) {
    std::vector<uint32_t> path;
    uint32_t current = to;

    while (current != from && paths[current].size() > 0) {
        path.push_back(current);
        current = paths[current].back();
    }
    path.push_back(from);

    Serial.print("Path: ");
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        Serial.print(*it);
        if (it + 1 != path.rend()) {
            Serial.print(" -> ");
        }
    }
    Serial.println();
}

/* void DynamicMeshingRouting::printCostMatrix() {
    Serial.println("Cost Matrix:");
    if (costs.empty()) {
        Serial.println("No costs recorded yet.");
        return; // Exit if the costs are empty
    }
    
    for (const auto &fromEntry : costs) {
        for (const auto &toEntry : fromEntry.second) {
            Serial.print("Cost from Node ");
            Serial.print(fromEntry.first);
            Serial.print(" to Node ");
            Serial.print(toEntry.first);
            Serial.print(": ");
            Serial.println(toEntry.second);
        }
    }
    Serial.println(); // Add a blank line for readability
} */
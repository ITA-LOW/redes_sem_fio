#include "DynamicMeshingRouting.h"

DynamicMeshingRouting::DynamicMeshingRouting() 
    : taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, [this](){ sendMessage(mesh.getNodeId()); }) {}

void DynamicMeshingRouting::setup(uint32_t fixedNodeId) {
    Serial.begin(115200);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
    mesh.onReceive([this](uint32_t from, String &msg){ receivedCallback(from, msg); });
    mesh.onNewConnection([this](uint32_t nodeId){ newConnectionCallback(nodeId); });
    mesh.onDroppedConnection([this](uint32_t nodeId){ droppedConnectionCallback(nodeId); });

    userScheduler.addTask(taskSendMessage);
    taskSendMessage.enable();
}

void DynamicMeshingRouting::loop() {
    mesh.update();
}

void DynamicMeshingRouting::sendMessage(uint32_t nodeId) {
    uint32_t centralNodeId = NODE_6_ID; // Definindo o nó central como o nó receptor
    uint32_t nextNode = findBestPath(nodeId, centralNodeId); // Encontra o melhor caminho

    if (nextNode != -1) {
        String msg = "Hello from Node " + String(nodeId);  // Mensagem do nó
        mesh.sendSingle(nextNode, msg); // Envia para o próximo nó
        Serial.printf("Node %u sending message to node %u\n", nodeId, nextNode);
        increaseCost(nodeId, nextNode); // Aumenta o custo da aresta
    } else {
        Serial.println("No nodes available for sending messages.");
    }
}

void DynamicMeshingRouting::receivedCallback(uint32_t from, String &msg) {
    Serial.printf("Received from node %u: %s\n", from, msg.c_str());

    // Se a mensagem foi recebida no nó receptor (nó central)
    if (mesh.getNodeId() == NODE_6_ID) {
        Serial.println("Message received at node 6 (central): " + msg);
        // Imprime o caminho percorrido
        printPath(from, NODE_6_ID);
    }
}

void DynamicMeshingRouting::newConnectionCallback(uint32_t nodeId) {
    Serial.printf("New connection with node %u\n", nodeId);
    initializeNodeCosts(nodeId);
}

void DynamicMeshingRouting::droppedConnectionCallback(uint32_t nodeId) {
    Serial.printf("Node %u disconnected\n", nodeId);
    removeNodeCosts(nodeId);
}

uint32_t DynamicMeshingRouting::findBestPath(uint32_t from, uint32_t to) {
    std::map<uint32_t, int> distances; // Distâncias de cada nó
    std::map<uint32_t, uint32_t> previous; // Nó anterior para rastrear o caminho
    std::vector<uint32_t> unvisited; // Nós não visitados

    for (const auto &node : mesh.getNodeList()) {
        distances[node] = (node == from) ? 0 : INT_MAX; // Distância inicial
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
            paths[to].push_back(previous[to]);
            return currentNode;
        }

        for (const auto &neighbor : costs[currentNode]) {
            int newDist = distances[currentNode] + neighbor.second; 
            if (newDist < distances[neighbor.first]) {
                distances[neighbor.first] = newDist;
                previous[neighbor.first] = currentNode; 
            }
        }
    }

    return -1; 
}

void DynamicMeshingRouting::increaseCost(uint32_t from, uint32_t to) {
    costs[from][to] += random(1, 5); 
    costs[to][from] = costs[from][to]; 
}

void DynamicMeshingRouting::initializeNodeCosts(uint32_t newNode) {
    for (auto &entry : costs) {
        costs[newNode][entry.first] = random(5, 15); 
        costs[entry.first][newNode] = costs[newNode][entry.first];
    }
}

void DynamicMeshingRouting::removeNodeCosts(uint32_t nodeId) {
    costs.erase(nodeId);
    for (auto &entry : costs) {
        entry.second.erase(nodeId);
    }
    Serial.printf("Node %u removed from cost matrix\n", nodeId);
}

void DynamicMeshingRouting::printPath(uint32_t from, uint32_t to) {
    std::vector<uint32_t> path;
    uint32_t current = to;

    while (current != from) {
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

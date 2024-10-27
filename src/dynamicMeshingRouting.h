#ifndef DYNAMICMESHINGROUTING_H
#define DYNAMICMESHINGROUTING_H

#include <painlessMesh.h>
#include <unordered_map>
#include <queue>
#include <limits>

#define MESH_PREFIX "YourMeshName" // Nome da rede mesh
#define MESH_PASSWORD "YourPassword" // Senha da rede mesh
#define MESH_PORT 5555 // Porta para comunicação

const int INFINITY_COST = 99999; // Valor que representa um custo infinito

class Graph {
public:
    void addEdge(uint32_t from, uint32_t to, int cost);
    int getCost(uint32_t from, uint32_t to);
    void setCost(uint32_t from, uint32_t to, int newCost);
    void incrementCost(uint32_t from, uint32_t to);
    const std::unordered_map<uint32_t, int>& getAdjacencyList(uint32_t node);

private:
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, int>> adjacencyList;
};

class dynamicMeshingRouting {
public:
    void init();
    void loop();
    void receivedCallback(uint32_t from, String &msg);
    void calculateShortestPath(uint32_t targetNode);
    void setReceiverNode(uint32_t nodeId);
    void initCosts();
    void updateEdgeCost(uint32_t fromNode, uint32_t toNode);
    painlessMesh mesh;
    Graph graph;
    uint32_t receiverNodeId;
    std::unordered_map<uint32_t, uint32_t> previousNode;
    std::deque<uint32_t> pathToTarget;
    uint32_t getMinCostNode();
    void sendMessage(String message, uint32_t targetNode);
    void incrementRouteCost(uint32_t fromNode, uint32_t toNode);
    
    static void onReceiveStatic(uint32_t from, String &msg) {
        instance->receivedCallback(from, msg);
    }
    
    static dynamicMeshingRouting *instance; // Instância estática
};

#endif // DYNAMICMESHINGROUTING_H

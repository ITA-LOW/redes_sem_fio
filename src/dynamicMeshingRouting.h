#ifndef DYNAMIC_MESHING_ROUTING_H
#define DYNAMIC_MESHING_ROUTING_H

#include "painlessMesh.h"
#include <map>
#include <vector>

#define MESH_PREFIX     "meshNetwork"
#define MESH_PASSWORD   "securePassword"
#define MESH_PORT       5555

// Defina os IDs fixos para os dispositivos
#define NODE_1_ID 1 // ID para o nó emissor
#define NODE_2_ID 2
#define NODE_3_ID 3
#define NODE_4_ID 4
#define NODE_5_ID 5
#define NODE_6_ID 6 // ID para o nó receptor

class DynamicMeshingRouting {
public:
    DynamicMeshingRouting();
    void setup(uint32_t fixedNodeId);  // Setup com ID fixo
    void loop();

private:
    Scheduler userScheduler;
    painlessMesh mesh;
    Task taskSendMessage;

    std::map<uint32_t, std::map<uint32_t, int>> costs; // Custo das arestas
    std::map<uint32_t, std::vector<uint32_t>> paths; // Caminhos percorridos

    void sendMessage(uint32_t nodeId);
    void receivedCallback(uint32_t from, String &msg);
    void newConnectionCallback(uint32_t nodeId);
    void droppedConnectionCallback(uint32_t nodeId);
    uint32_t findBestPath(uint32_t from, uint32_t to);  // Implementação do algoritmo de Dijkstra
    void increaseCost(uint32_t from, uint32_t to);
    void initializeNodeCosts(uint32_t newNode);
    void removeNodeCosts(uint32_t nodeId);
    void printPath(uint32_t from, uint32_t to); // Método para imprimir o caminho percorrido
};

#endif // DYNAMIC_MESHING_ROUTING_H

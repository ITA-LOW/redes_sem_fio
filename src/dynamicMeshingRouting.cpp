#include "dynamicMeshingRouting.h"

// Inicializa a rede mesh
dynamicMeshingRouting* dynamicMeshingRouting::instance = nullptr;

void dynamicMeshingRouting::init() {
    instance = this;
    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
    mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
    mesh.onReceive(onReceiveStatic); // Define o callback para receber mensagens
    initCosts(); // Inicializa os custos da rede
}

void dynamicMeshingRouting::loop() {
    mesh.update(); // Atualiza a rede mesh
}

void dynamicMeshingRouting::setReceiverNode(uint32_t nodeId) {
    receiverNodeId = nodeId; // Define o nó receptor
}

void dynamicMeshingRouting::initCosts() {
    for (const auto& node : graph.getAdjacencyList(mesh.getNodeId())) {
        graph.setCost(mesh.getNodeId(), node.first, 1); // Define custo 1 para cada aresta
    }
}

void dynamicMeshingRouting::updateEdgeCost(uint32_t fromNode, uint32_t toNode) {
    graph.incrementCost(fromNode, toNode); // Corrigido para usar incrementCost
}

uint32_t dynamicMeshingRouting::getMinCostNode() {
    uint32_t minNode = 0;
    int minCost = INFINITY_COST;

    for (const auto& node : graph.getAdjacencyList(mesh.getNodeId())) {
        if (node.second < minCost) {
            minCost = node.second;
            minNode = node.first;
        }
    }

    if (minNode != 0 && previousNode.find(minNode) != previousNode.end()) {
        uint32_t previous = previousNode[minNode];
        incrementRouteCost(previous, minNode);
    }

    return minNode;
}

void dynamicMeshingRouting::incrementRouteCost(uint32_t fromNode, uint32_t toNode) {
    graph.incrementCost(fromNode, toNode);
}

void Graph::addEdge(uint32_t from, uint32_t to, int cost) {
    adjacencyList[from][to] = cost;
}

int Graph::getCost(uint32_t from, uint32_t to) {
    if (adjacencyList.find(from) != adjacencyList.end() &&
        adjacencyList[from].find(to) != adjacencyList[from].end()) {
        return adjacencyList[from][to];
    }
    return INFINITY_COST; // Retorna custo "infinito" se não houver aresta
}

void Graph::setCost(uint32_t from, uint32_t to, int newCost) {
    adjacencyList[from][to] = newCost; // Atualiza o custo da aresta
}

void Graph::incrementCost(uint32_t from, uint32_t to) {
    int currentCost = getCost(from, to);
    setCost(from, to, currentCost + 1); // Incrementa o custo
}

void Graph::removeNode(uint32_t node) {
    // Remove todas as arestas conectadas ao nó a ser removido
    adjacencyList.erase(node); // Remove o nó do grafo

    // Remove qualquer referência ao nó nos adjacentes
    for (auto& [from, neighbors] : adjacencyList) {
        neighbors.erase(node); // Remove a aresta do nó adjacente
    }
}

const std::unordered_map<uint32_t, int>& Graph::getAdjacencyList(uint32_t node) {
    static const std::unordered_map<uint32_t, int> empty; // Valor de retorno padrão se o nó não existir
    auto it = adjacencyList.find(node);
    return (it != adjacencyList.end()) ? it->second : empty;
}

void dynamicMeshingRouting::calculateShortestPath(uint32_t targetNode) {
    // Define o custo inicial como infinito para todos os nós
    std::unordered_map<uint32_t, int> distance;
    std::unordered_map<uint32_t, uint32_t> previous;
    for (const auto& node : graph.getAdjacencyList(mesh.getNodeId())) {
        distance[node.first] = INFINITY_COST;
    }
    distance[mesh.getNodeId()] = 0;

    // Usamos uma fila de prioridade para gerenciar os nós a serem processados
    using NodeCostPair = std::pair<int, uint32_t>;
    std::priority_queue<NodeCostPair, std::vector<NodeCostPair>, std::greater<NodeCostPair>> pq;
    pq.push({0, mesh.getNodeId()});

    while (!pq.empty()) {
        auto [cost, currentNode] = pq.top();
        pq.pop();

        // Se alcançamos o nó alvo, finalizamos o caminho
        if (currentNode == targetNode) break;

        // Itera sobre os vizinhos do nó atual
        for (const auto& neighbor : graph.getAdjacencyList(currentNode)) {
            int newDist = cost + neighbor.second;
            if (newDist < distance[neighbor.first]) {
                distance[neighbor.first] = newDist;
                previous[neighbor.first] = currentNode;
                pq.push({newDist, neighbor.first});
            }
        }
    }

    // Armazena o caminho encontrado em uma lista para uso posterior
    pathToTarget.clear();
    uint32_t step = targetNode;
    while (step != mesh.getNodeId() && previous.find(step) != previous.end()) {
        pathToTarget.push_front(step);
        step = previous[step];
    }
    pathToTarget.push_front(mesh.getNodeId()); // Adiciona o nó inicial
}

void dynamicMeshingRouting::sendMessage(String message, uint32_t targetNode) {
    // Calcula o caminho para o nó alvo
    calculateShortestPath(targetNode);

    // Se o caminho está vazio ou apenas contém o nó atual, a mensagem já está no nó receptor
    if (pathToTarget.size() <= 1) {
        Serial.println("Mensagem já no nó receptor.");
        return;
    }

    // O próximo nó na rota
    uint32_t nextNode = *(++pathToTarget.begin());
    mesh.sendSingle(nextNode, message);

    Serial.println("[Nó " + String(mesh.getNodeId()).substring(String(mesh.getNodeId()).length() - 4) + "] Enviando mensagem: \"" + message + "\" para o nó " + String(nextNode).substring(nextNode - 4));
}

void dynamicMeshingRouting::receivedCallback(uint32_t from, String &msg) {
    Serial.println("[Nó " + String(mesh.getNodeId()).substring(String(mesh.getNodeId()).length() - 4) + "] Recebida mensagem de " + String(from).substring(from - 4) + ": " + msg);

    // Verifica se este nó é o nó receptor final
    if (mesh.getNodeId() == receiverNodeId) {
        Serial.println("[Nó " + String(mesh.getNodeId()).substring(String(mesh.getNodeId()).length() - 4) + "] Nó receptor recebeu: \"" + msg + "\"");
        return;
    }

    // Continua enviando a mensagem ao próximo nó na rota
    if (!pathToTarget.empty()) {
        sendMessage(msg, receiverNodeId);
    }
}

void dynamicMeshingRouting::handleNodeEntry(uint32_t nodeId) {
    // Adiciona o novo nó ao grafo e define as conexões iniciais
    for (const auto& neighbor : mesh.getNodeList()) {
        if (neighbor != nodeId) {
            graph.addEdge(nodeId, neighbor, 1); // Define o custo inicial como 1 para novos nós
            graph.addEdge(neighbor, nodeId, 1); // Define a conexão bidirecional
        }
    }
    // Recalcula os custos das rotas, considerando o novo nó
    initCosts();
    Serial.println("Novo nó " + String(nodeId).substring(String(mesh.getNodeId()).length() - 4) + " adicionado à rede.");
}

void dynamicMeshingRouting::handleNodeExit(uint32_t nodeId) {
    // Remove todas as conexões relacionadas ao nó que saiu
    for (const auto& neighbor : graph.getAdjacencyList(nodeId)) {
        graph.setCost(nodeId, neighbor.first, INFINITY_COST); // Define o custo como infinito
        graph.setCost(neighbor.first, nodeId, INFINITY_COST); // Remove a conexão bidirecional
    }

    // Remove o nó do grafo (opcional, dependendo da implementação do grafo)
    graph.removeNode(nodeId);

    // Recalcula as rotas e custos, considerando a saída do nó
    initCosts();
    Serial.println("Nó " + String(nodeId).substring(String(mesh.getNodeId()).length() - 4) + " removido da rede.");
}
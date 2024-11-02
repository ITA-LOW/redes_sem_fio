#include "dynamicMeshingRouting.h"
#include <TaskScheduler.h> // Move a inclusão para o arquivo .cpp

// Inicializa a rede mesh
dynamicMeshingRouting* dynamicMeshingRouting::instance = nullptr;

dynamicMeshingRouting::dynamicMeshingRouting() 
    : taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, [this]() { 
        if (receiverNodeId != mesh.getNodeId()) { // Certifique-se de que não envia a si mesmo
            sendMessage("Mensagem automática", receiverNodeId); 
        }
    }) 
{}

void dynamicMeshingRouting::init() {
    instance = this;
    mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT);
    mesh.onReceive(onReceiveStatic); // Define o callback para receber mensagens
    initCosts(); // Inicializa os custos da rede
    
    scheduler.addTask(taskSendMessage); // Adiciona a tarefa de envio de mensagens ao agendador
    taskSendMessage.enable(); // Habilita a tarefa para que comece a ser executada
}

void dynamicMeshingRouting::loop() {
    mesh.update(); // Atualiza a rede mesh
    scheduler.execute(); // Executa o agendador para verificar tarefas ativas
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
    std::unordered_map<uint32_t, int> costs;
    std::unordered_map<uint32_t, uint32_t> previous;
    std::priority_queue<std::pair<int, uint32_t>, std::vector<std::pair<int, uint32_t>>, std::greater<std::pair<int, uint32_t>>> queue;

    // Inicialização
    for (const auto &node : graph.getAdjacencyList(currentNode)) {
        costs[node.first] = INFINITY_COST;
        previous[node.first] = UINT32_MAX; // Indica que não há anterior
    }

    costs[receiverNodeId] = 0;
    queue.push({0, receiverNodeId});

    while (!queue.empty()) {
        uint32_t currentNode = queue.top().second;
        queue.pop();

        if (currentNode == targetNode) break; // Caminho encontrado

        for (const auto &neighbor : graph.getAdjacencyList(currentNode)) {
            int newCost = costs[currentNode] + neighbor.second;
            if (newCost < costs[neighbor.first]) {
                costs[neighbor.first] = newCost;
                previous[neighbor.first] = currentNode;
                queue.push({newCost, neighbor.first});
            }
        }
    }

    // Reconstrução do caminho
    pathToTarget.clear();
    for (uint32_t at = targetNode; at != UINT32_MAX; at = previous[at]) {
        pathToTarget.push_front(at);
    }

    // Verifica se encontramos um caminho válido
    if (pathToTarget.size() <= 1) {
        Serial.println("Caminho não encontrado.");
    } else {
        Serial.print("Caminho encontrado: ");
        for (const auto &node : pathToTarget) {
            Serial.print(node);
            Serial.print(" ");
        }
        Serial.println();

        // Incrementa o custo das arestas usadas
        for (size_t i = 0; i < pathToTarget.size() - 1; ++i) {
            uint32_t fromNode = pathToTarget[i];
            uint32_t toNode = pathToTarget[i + 1];
            graph.incrementCost(fromNode, toNode); // Incrementa o custo da aresta
        }
    }
}

void dynamicMeshingRouting::sendMessage(String message, uint32_t targetNode) {
    calculateShortestPath(targetNode);
    if (pathToTarget.size() <= 1) {
        Serial.println("A mensagem já está no nó receptor.");
        return;
    }

    // Envia a mensagem para o próximo nó no caminho
    uint32_t nextNode = pathToTarget[1]; // O primeiro nó após o receptor
    sendMessage(message,nextNode);
    Serial.print("Mensagem enviada para o nó: ");
    Serial.println(nextNode);
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
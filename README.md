# dynamicMeshingRouting - Rede Mesh Dinâmica para Roteamento

Este projeto implementa um sistema de roteamento em uma rede mesh para dispositivos ESP, utilizando a classe `dynamicMeshingRouting` para gerenciar a rede e a comunicação entre os nós.

## Sumário

- [Descrição do Projeto](#descrição-do-projeto)
- [Classes e Funções](#classes-e-funções)
  - [dynamicMeshingRouting](#dynamicmeshingrouting)
    - [Funções da Classe dynamicMeshingRouting](#funções-da-classe-dynamicmeshingrouting)
  - [Graph](#graph)
    - [Funções da Classe Graph](#funções-da-classe-graph)
- [Notas de Implementação](#notas-de-implementação)

## Descrição do Projeto

A rede mesh é configurada para funcionar mesmo com barreiras físicas e interferências, simulando um ambiente real com obstáculos. Cada dispositivo ESP atua como um nó, e as mensagens são roteadas dinamicamente com base no custo das conexões entre os nós. O grafo da rede armazena as relações de custo entre nós, ajudando a escolher o caminho de menor custo para enviar mensagens até o nó receptor final.

## Classes e Funções

### `dynamicMeshingRouting`

A classe `dynamicMeshingRouting` é responsável pela configuração da rede mesh, pelo envio e recebimento de mensagens, e pelo cálculo de caminhos de menor custo. A seguir, cada função é detalhada.

#### Funções da Classe `dynamicMeshingRouting`

- **init()**
  - Inicializa a rede mesh e define as configurações básicas, como mensagens de debug e autenticação.
  - Define o callback `onReceiveStatic` para lidar com mensagens recebidas e chama `initCosts()` para configurar os custos iniciais das conexões entre nós.

- **loop()**
  - Atualiza o estado da rede mesh chamando `mesh.update()` continuamente.

- **setReceiverNode(uint32_t nodeId)**
  - Define o ID do nó receptor final para as mensagens. Esse ID é armazenado na variável `receiverNodeId`.

- **initCosts()**
  - Define o custo inicial de 1 para cada conexão (ou aresta) entre o nó atual e os seus vizinhos diretos.

- **updateEdgeCost(uint32_t fromNode, uint32_t toNode)**
  - Incrementa o custo de uma aresta específica no grafo, entre `fromNode` e `toNode`.

- **getMinCostNode()**
  - Determina o nó vizinho com o menor custo de conexão. Atualiza o custo da rota para esse nó, caso já tenha sido visitado, e retorna o nó de menor custo.

- **incrementRouteCost(uint32_t fromNode, uint32_t toNode)**
  - Incrementa o custo de uma rota específica no grafo entre dois nós, aumentando o custo de envio entre `fromNode` e `toNode`.

- **calculateShortestPath(uint32_t targetNode)**
  - Calcula o caminho de menor custo até um nó alvo, `targetNode`, utilizando uma fila de prioridade para priorizar nós de menor custo.
  - Armazena o caminho encontrado em `pathToTarget`.

- **sendMessage(String message, uint32_t targetNode)**
  - Envia uma mensagem para o `targetNode` através do caminho de menor custo calculado por `calculateShortestPath()`.
  - Caso o próximo nó na rota seja o nó receptor, a mensagem é enviada diretamente, senão é enviada ao próximo nó no caminho.

- **receivedCallback(uint32_t from, String &msg)**
  - Função callback para tratar mensagens recebidas. Verifica se o nó atual é o receptor final e, caso não seja, reenvia a mensagem pelo próximo nó no caminho.

- **handleNodeEntry(uint32_t nodeId)**
  - Adiciona um novo nó ao grafo e configura conexões iniciais com custo 1 entre o novo nó e seus vizinhos. Atualiza os custos da rota.

- **handleNodeExit(uint32_t nodeId)**
  - Remove um nó do grafo, define o custo das conexões associadas como infinito (`INFINITY_COST`), e atualiza as rotas restantes.

### `Graph`

A classe `Graph` armazena o grafo que representa as conexões entre os nós da rede mesh, com cada conexão (aresta) associada a um custo. O grafo é implementado como uma lista de adjacência.

#### Funções da Classe `Graph`

- **addEdge(uint32_t from, uint32_t to, int cost)**
  - Adiciona uma aresta com um custo entre dois nós (`from` e `to`), indicando uma conexão bidirecional entre eles.

- **getCost(uint32_t from, uint32_t to)**
  - Retorna o custo de uma aresta entre dois nós. Se a aresta não existir, retorna `INFINITY_COST`.

- **setCost(uint32_t from, uint32_t to, int newCost)**
  - Atualiza o custo de uma aresta existente entre dois nós.

- **incrementCost(uint32_t from, uint32_t to)**
  - Incrementa o custo de uma aresta entre dois nós em 1 unidade.

- **removeNode(uint32_t node)**
  - Remove um nó do grafo e elimina todas as arestas conectadas a esse nó.

- **getAdjacencyList(uint32_t node)**
  - Retorna a lista de adjacência para um nó específico, ou uma lista vazia se o nó não estiver no grafo.

## Notas de Implementação

1. **Instância Singleton:** `dynamicMeshingRouting` usa um padrão singleton para garantir uma única instância, acessível via `instance`.
2. **Custos de Arestas Dinâmicos:** O custo das arestas é atualizado dinamicamente para refletir condições de rede em tempo real.
3. **Fila de Prioridade para Cálculo de Rotas:** Para eficiência, o cálculo de menor custo usa uma fila de prioridade, que prioriza nós com custos menores de transmissão.
4. **Callback para Mensagens Recebidas:** O callback `receivedCallback` permite que cada nó verifique se é o destinatário final ou se precisa encaminhar a mensagem.

Este projeto fornece um exemplo de implementação de roteamento dinâmico em uma rede mesh, onde as condições de rede são simuladas com interferências e barreiras físicas, permitindo uma análise eficiente da resiliência e eficiência do protocolo.

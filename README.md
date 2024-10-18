# Rede ad hoc com ESPs Usando painlessMesh

Este projeto implementa uma rede ad hoc dinâmica com ESPs, utilizando a biblioteca **painlessMesh**. A rede é capaz de rotear mensagens de nós emissores até um nó central de recebimento, adaptando-se automaticamente a novos nós e desconexões.

## Funcionalidades

- **Matriz de Custo Flexível**: Usa `std::map` para ajustar dinamicamente os custos de rotas entre nós. Nós podem ser adicionados ou removidos em tempo real.
- **Roteamento de Mensagens**: As mensagens são roteadas do emissor até a central de recebimento (nó final) através de caminhos de menor custo.
- **Aumento de Custo Dinâmico**: Rota usada com frequência tem seu custo aumentado, incentivando o uso de rotas alternativas.
- **Reenvio em Caso de Falha**: Se uma mensagem não encontrar uma rota válida, ela tenta reenviar automaticamente.
- **Detecção Dinâmica de Novos Nós**: Novos nós são identificados e integrados à rede sem reconfiguração.
- **Remoção Automática de Nós**: Nós desconectados são removidos da matriz de custo, evitando rotas inválidas.

## Estrutura do Código

### Funções Principais

- `initializeNodeCosts(uint32_t newNode)`: Inicializa custos de novos nós.
- `removeNodeCosts(uint32_t nodeId)`: Remove nós desconectados da matriz.
- `findBestPath(uint32_t from, uint32_t to)`: Encontra o melhor caminho baseado em custo.
- `sendMessage()`: Envia mensagens da origem para o nó final.
- `receivedCallback(uint32_t from, String &msg)`: Processa mensagens recebidas.
- `attemptResendMessage(uint32_t from, String &msg, uint32_t targetNode)`: Tenta reenviar mensagem em caso de falha.

### Eventos

- `newConnectionCallback(uint32_t nodeId)`: Detecta novos nós e os adiciona à rede.
- `droppedConnectionCallback(uint32_t nodeId)`: Remove nós desconectados.

## Requisitos

- **Hardware**: ESP8266 ou ESP32.
- **Software**: Arduino IDE, biblioteca painlessMesh.

## Como Usar

1. Defina o nome da rede (`MESH_PREFIX`), senha (`MESH_PASSWORD`), e porta (`MESH_PORT`).
2. Carregue o código em cada ESP.
3. Use o monitor serial para acompanhar o roteamento e a detecção de novos nós.

## Expansão

- **Adição de Novos Nós**: Novos nós são integrados automaticamente.
- **Customização de Roteamento**: Pode ser ajustado para incluir critérios como latência ou qualidade de sinal.

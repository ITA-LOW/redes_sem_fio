#include "DynamicMeshingRouting.h"

DynamicMeshingRouting dynamicMeshingRouting;

void setup() {
    // Altere o ID conforme o ESP
    dynamicMeshingRouting.setup(NODE_1_ID);  // Para o primeiro nó
    // Exemplo: Para o segundo nó, mude para NODE_2_ID
}

void loop() {
    dynamicMeshingRouting.loop();
}

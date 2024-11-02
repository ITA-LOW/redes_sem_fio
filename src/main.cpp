#include <Arduino.h>
#include "dynamicMeshingRouting.h"

dynamicMeshingRouting meshNetwork;

void setup() {
    Serial.begin(115200); // Inicializa a serial para depuração
    while (!Serial); // Aguarda a inicialização da serial, útil para algumas placas

    meshNetwork.init(); // Inicializa a rede mesh
    Serial.println("Rede mesh inicializada. ID do nó: " + String(meshNetwork.mesh.getNodeId()));

    // Defina o nó receptor (exemplo: se o ID do nó receptor for 2)
    meshNetwork.setReceiverNode(761895598);
    //Serial.println("Nó receptor definido como 2.");

    // Enviar uma mensagem do nó atual para o nó receptor 1
    String message = "SEND:status do sensor";
    meshNetwork.sendMessage(message, 761895598);  // Aqui você define a mensagem e o nó alvo
}

void loop() {
    meshNetwork.loop(); // Chama o loop da rede mesh
}


#pragma once

#include <string>

// Structure qui représente un message DTN-Edge
struct Message {

    // Identifiant unique du message
    std::string id;

    // Date et heure de création
    std::string created_at;

    // Données originales reçues de l'application
    std::string payload;

    // Priorité du message (0 par défaut)
    int priority{0};

    // Statut initial : le message attend son envoi
    std::string status{"PENDING"};

    // Nombre de tentatives d'envoi échouées
    int retry_count{0};
};

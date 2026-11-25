
#pragma once

#include "message.hpp"
#include <string>

// Classe responsable de la création et de la conversion des messages
class MessageManager {

public:

    // Crée un message à partir d'un payload
    // La priorité vaut 0 si aucune priorité n'est fournie
    Message create(
        const std::string& payload,
        int priority = 0
    ) const;

    // Protège les caractères spéciaux pour le format JSON
    static std::string json_escape(
        const std::string& value
    );

    // Convertit un objet Message en chaîne JSON
    static std::string to_json(
        const Message& message
    );
};

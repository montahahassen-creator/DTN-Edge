
#include "message_manager.hpp"

#include <random>
#include <sstream>
#include <iomanip>
#include <ctime>

// =====================================================
// 1. GENERER UN IDENTIFIANT UNIQUE
// =====================================================

namespace {

std::string make_id() {

    // Générateur de nombres aléatoires
    std::random_device rd;
    std::mt19937_64 gen(rd());

    // Distribution sur les nombres entiers non signés
    std::uniform_int_distribution<unsigned long long> dist;

    // Flux utilisé pour construire l'identifiant
    std::ostringstream out;

    // Conversion des nombres en hexadecimal
    out << std::hex << dist(gen) << dist(gen);

    // Retourner l'identifiant généré
    return out.str();
}


// =====================================================
// 2. GENERER LA DATE ET L'HEURE
// =====================================================

std::string now_iso() {

    // Récupérer l'heure actuelle
    std::time_t now = std::time(nullptr);

    // Convertir l'heure en UTC
    std::tm utc_time{};

    gmtime_r(&now, &utc_time);

    // Préparer un flux pour formater la date
    std::ostringstream out;

    // Format ISO 8601
    out << std::put_time(
        &utc_time,
        "%Y-%m-%dT%H:%M:%SZ"
    );

    return out.str();
}

} // Fin du namespace anonyme


// =====================================================
// 3. CREER UN MESSAGE
// =====================================================

Message MessageManager::create(
    const std::string& payload,
    int priority
) const {

    // Créer un objet Message vide
    Message m;

    // Générer son identifiant
    m.id = make_id();

    // Ajouter la date de création
    m.created_at = now_iso();

    // Copier les données reçues
    m.payload = payload;

    // Définir la priorité
    m.priority = priority;

    // Le statut et retry_count utilisent
    // leurs valeurs initiales définies dans message.hpp

    return m;
}


// =====================================================
// 4. PROTEGER LES CARACTERES SPECIAUX DU JSON
// =====================================================

std::string MessageManager::json_escape(
    const std::string& value
) {

    std::string result;

    // Parcourir chaque caractère de la chaîne
    for (char c : value) {

        switch (c) {

            case '"':
                result += "\\\"";
                break;

            case '\\':
                result += "\\\\";
                break;

            case '\n':
                result += "\\n";
                break;

            case '\r':
                result += "\\r";
                break;

            case '\t':
                result += "\\t";
                break;

            default:

                // Protéger les autres caractères de contrôle
                if (static_cast<unsigned char>(c) < 0x20) {

                    const char hex[] = "0123456789abcdef";

                    result += "\\u00";
                    result += hex[
                        (static_cast<unsigned char>(c) >> 4) & 0x0F
                    ];
                    result += hex[
                        static_cast<unsigned char>(c) & 0x0F
                    ];

                } else {

                    result += c;
                }

                break;
        }
    }

    return result;
}


// =====================================================
// 5. CONVERTIR UN MESSAGE EN JSON
// =====================================================

std::string MessageManager::to_json(
    const Message& m
) {

    // Construire le JSON à partir des attributs du message
    return "{\"id\":\"" + json_escape(m.id) +
           "\",\"created_at\":\"" + json_escape(m.created_at) +
           "\",\"priority\":" + std::to_string(m.priority) +
           ",\"payload\":\"" + json_escape(m.payload) + "\"}";
}

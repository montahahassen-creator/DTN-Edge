#pragma once

#include "message.hpp"
#include <sqlite3.h> // Bibliothèque officielle C de SQLite
#include <string>
#include <vector>
#include <mutex>     // Pour protéger l'accès concurrent entre plusieurs threads

class Storage {
public:
    // Constructeur : ouvre la base SQLite (et la crée si elle n'existe pas encore)
    explicit Storage(const std::string& path);

    // Destructeur : ferme proprement la connexion SQLite à l'arrêt du programme
    ~Storage();

    // Interdit la copie de l'objet Storage (évite de dupliquer le pointeur SQLite)
    Storage(const Storage&) = delete;
    Storage& operator=(const Storage&) = delete;

    // 1. Enregistre un nouveau message dans la base de données
    bool insert(const Message& m);

    // 2. Récupère une liste de messages en attente ("PENDING") triés par priorité
    std::vector<Message> pending(int limit = 10);

    // 3. Met à jour le statut d'un message à "SENT" après réception de l'ACK
    bool mark_sent(const std::string& id);

    // 4. Incrémente le compteur d'échecs et enregistre l'erreur réseau
    bool mark_retry(const std::string& id, const std::string& error);

private:
    sqlite3* db_{nullptr}; // Pointeur vers la connexion SQLite active
    std::mutex mtx_;       // Verrou (mutex) pour garantir la sécurité multi-thread

    // Méthode interne pour exécuter une requête SQL simple (ex: CREATE TABLE)
    bool exec(const char* sql);
};
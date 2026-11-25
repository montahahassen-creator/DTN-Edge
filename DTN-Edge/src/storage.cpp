#include "storage.hpp"
#include <filesystem> // Pour créer automatiquement les dossiers parents
#include <stdexcept>  // Pour lever std::runtime_error en cas d'échec d'ouverture

// --- CONSTRUCTEUR : Création du dossier, de la base et de la table ---
Storage::Storage(const std::string& path) {
    // 1. Vérifie le chemin parent (ex: "/var/lib/dtn-edge") et le crée s'il n'existe pas
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }

    // 2. Ouvre le fichier de base de données SQLite (le crée automatiquement s'il est absent)
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        throw std::runtime_error("SQLite open failed");
    }

    // 3. Active le mode WAL (Write-Ahead Logging) pour améliorer la concurrence et protéger contre les coupures de courant
    exec("PRAGMA journal_mode=WAL;");

    // 4. Crée la table 'messages' si elle n'existe pas encore
    exec(
        "CREATE TABLE IF NOT EXISTS messages ("
        "id TEXT PRIMARY KEY,"                    // Identifiant unique (clé primaire)
        "created_at TEXT NOT NULL,"               // Date de création
        "payload TEXT NOT NULL,"                  // Contenu du message
        "priority INTEGER NOT NULL,"              // Niveau de priorité
        "status TEXT NOT NULL,"                   // Statut : PENDING ou SENT
        "retry_count INTEGER NOT NULL DEFAULT 0," // Compteur d'échecs (initialisé à 0)
        "last_error TEXT DEFAULT '')"             // Description de la dernière erreur réseau
    );
}

// --- DESTRUCTEUR : Fermeture propre ---
Storage::~Storage() {
    if (db_) {
        sqlite3_close(db_); // Ferme le fichier .db et libère les ressources SQLite
    }
}

// --- MÉTHODE INTERNE : Exécuter une commande SQL sans résultat attendu ---
bool Storage::exec(const char* sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        if (err) sqlite3_free(err); // Libère la mémoire allouée par SQLite pour le message d'erreur
        return false;
    }
    return true;
}

// --- 1. INSERT : Sauvegarder un nouveau message ---
bool Storage::insert(const Message& m) {
    std::lock_guard<std::mutex> lock(mtx_); // Verrouillage pour l'accès multi-thread

    // Requête préparée avec des paramètres '?' pour éviter les injections SQL
    const char* sql =
        "INSERT INTO messages(id, created_at, payload, priority, status, retry_count) "
        "VALUES(?, ?, ?, ?, ?, ?)";

    sqlite3_stmt* st = nullptr;
    // Compilation de la requête SQL
    if (sqlite3_prepare_v2(db_, sql, -1, &st, nullptr) != SQLITE_OK) {
        return false;
    }

    // Liaison des valeurs aux paramètres '?' (l'indexation commence à 1 en SQLite)
    sqlite3_bind_text(st, 1, m.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, m.created_at.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 3, m.payload.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st, 4, m.priority);
    sqlite3_bind_text(st, 5, m.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(st, 6, m.retry_count);

    // Exécution de l'insertion
    bool ok = (sqlite3_step(st) == SQLITE_DONE);

    // Libération de la requête préparée (obligatoire pour éviter les fuites mémoire)
    sqlite3_finalize(st);
    return ok;
}

// --- 2. PENDING : Récupérer les messages en attente par ordre de priorité ---
std::vector<Message> Storage::pending(int limit) {
    std::lock_guard<std::mutex> lock(mtx_); // Verrouillage pour l'accès multi-thread
    std::vector<Message> out;

    // Sélectionne uniquement les messages 'PENDING', triés par priorité décroissante puis par date croissante
    const char* sql =
        "SELECT id, created_at, payload, priority, status, retry_count "
        "FROM messages "
        "WHERE status='PENDING' "
        "ORDER BY priority DESC, created_at ASC "
        "LIMIT ?";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &st, nullptr) != SQLITE_OK) {
        return out;
    }

    sqlite3_bind_int(st, 1, limit); // Définit le nombre maximal de messages à retourner

    // Parcourt chaque ligne retournée par la requête
    while (sqlite3_step(st) == SQLITE_ROW) {
        Message m;
        // Extraction des colonnes (l'indexation commence à 0 en lecture)
        m.id          = reinterpret_cast<const char*>(sqlite3_column_text(st, 0));
        m.created_at  = reinterpret_cast<const char*>(sqlite3_column_text(st, 1));
        m.payload     = reinterpret_cast<const char*>(sqlite3_column_text(st, 2));
        m.priority    = sqlite3_column_int(st, 3);
        m.status      = reinterpret_cast<const char*>(sqlite3_column_text(st, 4));
        m.retry_count = sqlite3_column_int(st, 5);
        out.push_back(std::move(m));
    }

    sqlite3_finalize(st);
    return out;
}

// --- 3. MARK_SENT : Confirmer l'envoi après réception de l'ACK ---
bool Storage::mark_sent(const std::string& id) {
    std::lock_guard<std::mutex> lock(mtx_);

    const char* sql = "UPDATE messages SET status='SENT' WHERE id=?";
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &st, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(st, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(st) == SQLITE_DONE);
    sqlite3_finalize(st);
    return ok;
}

// --- 4. MARK_RETRY : Enregistrer un échec d'envoi (réseau indisponible) ---
bool Storage::mark_retry(const std::string& id, const std::string& error) {
    std::lock_guard<std::mutex> lock(mtx_);

    // Le statut reste 'PENDING', on incrémente retry_count et on sauvegarde le message d'erreur
    const char* sql =
        "UPDATE messages "
        "SET retry_count = retry_count + 1, last_error = ? "
        "WHERE id = ?";

    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &st, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(st, 1, error.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, id.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(st) == SQLITE_DONE);
    sqlite3_finalize(st);
    return ok;
}
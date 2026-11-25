#include "Database.h"

int Database::changed()const{
        return sqlite3_changes(db_);
    }

int Database::inserted()const{
        return (int)sqlite3_last_insert_rowid(db_);
    }

void Database::execute(const std::string&sql){
        char*e=nullptr;
        if(sqlite3_exec(db_,sql.c_str(),nullptr,nullptr,&e)!=SQLITE_OK)
        {
            std::string s=e?e:"SQL error";
            sqlite3_free(e);
            throw std::runtime_error(s);
        }
    }

sqlite3* Database::raw(){
        return db_;
    }

Database::~Database(){
        sqlite3_close(db_);
    }

Database::Database(const fs::path&path){
        fs::create_directories(path.parent_path());
        if(sqlite3_open(path.string().c_str(),&db_)!=SQLITE_OK)throw std::runtime_error("Unable to open SQLite database");
        execute("PRAGMA foreign_keys=ON");
        execute("CREATE TABLE IF NOT EXISTS users(id INTEGER PRIMARY KEY,name TEXT NOT NULL,email TEXT UNIQUE NOT NULL,salt TEXT NOT NULL,hash TEXT NOT NULL)");
        execute("CREATE TABLE IF NOT EXISTS tasks(id INTEGER PRIMARY KEY,user_id INTEGER NOT NULL,title TEXT NOT NULL,subject TEXT NOT NULL,category TEXT NOT NULL,description TEXT DEFAULT '',deadline TEXT NOT NULL,estimated REAL NOT NULL,remaining REAL NOT NULL,difficulty TEXT NOT NULL,priority TEXT NOT NULL,status TEXT NOT NULL DEFAULT 'Pending',FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE)");
        execute("CREATE TABLE IF NOT EXISTS availability(user_id INTEGER NOT NULL,weekday INTEGER NOT NULL,hours REAL NOT NULL,PRIMARY KEY(user_id,weekday),FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE)");
        // Additive tables: older StudySync databases remain compatible.
        execute("CREATE TABLE IF NOT EXISTS time_logs(id INTEGER PRIMARY KEY,user_id INTEGER NOT NULL,task_id INTEGER NOT NULL,minutes INTEGER NOT NULL,logged_at TEXT NOT NULL DEFAULT (datetime('now')),FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE,FOREIGN KEY(task_id) REFERENCES tasks(id) ON DELETE CASCADE)");
        execute("CREATE TABLE IF NOT EXISTS pinned_sessions(id INTEGER PRIMARY KEY,user_id INTEGER NOT NULL,task_id INTEGER NOT NULL,day TEXT NOT NULL,hours REAL NOT NULL,FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE,FOREIGN KEY(task_id) REFERENCES tasks(id) ON DELETE CASCADE)");
        execute("CREATE TABLE IF NOT EXISTS dismissed_alerts(user_id INTEGER NOT NULL,alert_key TEXT NOT NULL,PRIMARY KEY(user_id,alert_key),FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE)");
    }

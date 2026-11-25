#pragma once
#include "../utils/Common.h"
// Owns the SQLite connection, creates the tables and provides prepared statements.
class Database
{
    sqlite3* db_=nullptr;
    public:
    struct Statement
    {
        sqlite3_stmt* q=nullptr;
        Statement(sqlite3*db,const std::string&sql)
        {
            if(sqlite3_prepare_v2(db,sql.c_str(),-1,&q,nullptr)!=SQLITE_OK)throw std::runtime_error(sqlite3_errmsg(db));
        }
        ~Statement()
        {
            sqlite3_finalize(q);
        }
        Statement(const Statement&)=delete;
        Statement& operator=(const Statement&)=delete;
        void bind(int i,int v)
        {
            sqlite3_bind_int(q,i,v);
        }
        void bind(int i,double v)
        {
            sqlite3_bind_double(q,i,v);
        }
        void bind(int i,const std::string&v)
        {
            sqlite3_bind_text(q,i,v.c_str(),(int)v.size(),SQLITE_TRANSIENT);
        }
        bool next()
        {
            int r=sqlite3_step(q);
            if(r==SQLITE_ROW)return true;
            if(r==SQLITE_DONE)return false;
            throw std::runtime_error(sqlite3_errmsg(sqlite3_db_handle(q)));
        }
        void run()
        {
            if(sqlite3_step(q)!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(sqlite3_db_handle(q)));
        }
        int integer(int i)const
        {
            return sqlite3_column_int(q,i);
        }
        double real(int i)const
        {
            return sqlite3_column_double(q,i);
        }
        std::string text(int i)const
        {
            auto p=(const char*)sqlite3_column_text(q,i);
            return p?p:"";
        }
    };
    explicit Database(const fs::path& path);
    ~Database();
    sqlite3* raw();
    void execute(const std::string&sql);
    int inserted()const;
    int changed()const;
};

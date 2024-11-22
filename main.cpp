// g++ -std=c++17 -o test main.cpp -I/opt/homebrew/opt/libpq/include -L/opt/homebrew/opt/libpq/lib -lpq
#include <iostream>
#include <libpq-fe.h>

int main() {
    const char *conninfo = "dbname=gms user=juuseong password=asdf5788";

    // PostgreSQL 연결 시도
    PGconn *conn = PQconnectdb(conninfo);

    // 연결 성공 여부 확인
    if (PQstatus(conn) == CONNECTION_BAD) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(conn) << std::endl;
        PQfinish(conn);
        return 1;
    }

    std::cout << "Connected to the database successfully!" << std::endl;

    // 연결 종료
    PQfinish(conn);
    return 0;
}

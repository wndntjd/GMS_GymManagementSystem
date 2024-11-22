// g++ -std=c++17 -o test main.cpp -I/opt/homebrew/opt/libpq/include -L/opt/homebrew/opt/libpq/lib -lpq
#include <iostream>
#include <libpq-fe.h>
#include <string>
#include <vector>

PGconn* connectDB() {
    const char* conninfo = "dbname=gms user=juuseong password=asdf5788";
    PGconn* conn = PQconnectdb(conninfo);

    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "Connection to database failed: " << PQerrorMessage(conn) << std::endl;
        PQfinish(conn);
        return nullptr;
    }

    std::cout << "Connected to database successfully!" << std::endl;
    return conn;
}

bool login(PGconn *conn, int userID, const std::string &role) {
    if (role != "admin" && role != "member" && role != "trainer" && role != "fdstaff") {
        std::cout << "Invalid role!" << std::endl;
        return false;
    }
    if (role == "admin" ) {
        if (userID == 23571113) {
            std::cout << "Login successful as " << role << "!" << std::endl;
            return true;
        }
        else {
            std::cout << "Login failed!" << std::endl;
            return false;
        }
    }
    // role에 's'를 붙여서 테이블 이름을 결정
    std::string tableName = role + "s";  // 예: "trainer" -> "trainers"

    // 해당 테이블에서 role을 확인하는 쿼리
    std::string query = "SELECT role FROM " + tableName + " WHERE ID = " + std::to_string(userID) + ";";
    
    // 쿼리 실행
    PGresult *res = PQexec(conn, query.c_str());

    // 쿼리 실행 결과 확인
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        std::cout << "Login successful as " << role << "!" << std::endl;
        PQclear(res);
        return true;
    }

    std::cout << "Login failed!" << std::endl;
    return false;
}

int main() {
    PGconn* conn = connectDB();
    if (!conn) return 1;

    int userID; std::cout << "Enter ID: "; std::cin >> userID;
    std::string role; std::cout << "Enter role (admin, member, trainer, fdstaff): "; std::cin >> role;

    if (login(conn, userID, role)) {
        std::cout << "User logged in successfully!" << std::endl;
    } else {
        std::cout << "Login failed!" << std::endl;
    }

    PQfinish(conn);
    return 0;
}

// g++ -std=c++17 -o test main.cpp -I/opt/homebrew/opt/libpq/include -L/opt/homebrew/opt/libpq/lib -lpq
#include <iostream>
#include <libpq-fe.h>
#include <string>
#include <vector>

PGconn* connectDB() {
    const char* conninfo = "dbname=gms user=juuseong";
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
        if (userID == 23571113) return true;
        else return false;
    }

    // role에 's'를 붙여서 테이블 이름을 결정
    std::string tableName = role + "s";  // 예: "trainer" -> "trainers"

    // 해당 테이블에서 role을 확인하는 쿼리
    std::string query = "SELECT * FROM " + tableName + " WHERE ID = " + std::to_string(userID) + ";";
    
    // 쿼리 실행
    PGresult *res = PQexec(conn, query.c_str());

    // 쿼리 실행 결과 확인
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0) {
        PQclear(res);
        return true;
    }
    return false;
}

void requestPT(PGconn *conn, int memberID) {
    std::string gender, name, sortBy;
    int trainerID;

    // Step 1: 트레이너 조회 조건 입력
    std::cout << "Enter trainer's gender (M/F) or (all): ";
    std::cin >> gender;
    std::cout << "Enter trainer's name or (all): ";
    std::cin.ignore();
    std::getline(std::cin, name);
    std::cout << "Enter sorting option (id/name/age): ";
    std::getline(std::cin, sortBy);

    // Step 2: 트레이너 조회 쿼리 생성
    std::string query = "SELECT ID, name, height, weight, age, sex FROM Trainers WHERE 1=1";
    if (gender == "M" || gender == "F") query += " AND sex = '" + gender + "'";
    if (name != "all") query += " AND name ILIKE '%" + name + "%'";
    if (sortBy == "id" || sortBy == "name" || sortBy == "age") query += " ORDER BY " + sortBy + ";";
    else std::cout << "Invalid sorting option. Using default sorting.\n";

    std::cout << query << std::endl;

    PGresult *res = PQexec(conn, query.c_str());
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int rows = PQntuples(res);
        if (rows == 0) {
            std::cout << "No trainers match your criteria.\n";
        } else {
            std::cout << "Available trainers:\n";
            for (int i = 0; i < rows; ++i) {
                std::cout << "ID: " << PQgetvalue(res, i, 0) << ", Name: " << PQgetvalue(res, i, 1)
                          << ", Height: " << PQgetvalue(res, i, 2) << ", Weight: " << PQgetvalue(res, i, 3)
                          << ", Age: " << PQgetvalue(res, i, 4) << ", Sex: " << PQgetvalue(res, i, 5) << "\n";
            }
        }
    }
    PQclear(res);

    // Step 3: 트레이너 선택
    std::cout << "Enter the ID of the trainer you'd like to apply for: ";
    std::cin >> trainerID;

    // Step 4: 트랜잭션 시작 및 PT 신청
    std::string applyQuery = "BEGIN; "
                             "INSERT INTO PTs (TID, MID, remaining) VALUES (" + std::to_string(trainerID) + ", " +
                             std::to_string(memberID) + ", 10); "
                             "COMMIT;";

    res = PQexec(conn, applyQuery.c_str());
    if (PQresultStatus(res) == PGRES_COMMAND_OK) {
        std::cout << "PT session successfully applied!\n";
    } else {
        std::cerr << "Failed to apply for PT. Rolling back...\n";
        PQexec(conn, "ROLLBACK;");
    }
    PQclear(res);
}

void requestLocker(PGconn *conn, int memberID) {
    std::string size;
    int lockerID;

    // Step 1: 사물함 조회 조건 입력
    std::cout << "Enter locker size (Small/Medium/Large) or (all): ";
    std::cin >> size;

    // Step 2: 사물함 조회
    std::string query = "SELECT number, size FROM Lockers WHERE owner IS NULL";
    if (size == "Small" || size == "Medium" || size == "Large") query += " AND size ILIKE '" + size + "'";
    query += " ORDER BY number;";

    std::cout << query << std::endl;

    PGresult *res = PQexec(conn, query.c_str());
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int rows = PQntuples(res);
        if (rows == 0) {
            std::cout << "No available lockers found.\n";
        } else {
            std::cout << "Available lockers:\n";
            for (int i = 0; i < rows; ++i)
                std::cout << "Number: " << PQgetvalue(res, i, 0) << ", Size: " << PQgetvalue(res, i, 1) << "\n";
        }
    }
    PQclear(res);

    // Step 3: 사물함 선택
    std::cout << "Enter the locker number you'd like to apply for: ";
    std::cin >> lockerID;

    // Step 4: 트랜잭션 시작 및 사물함 신청
    std::string applyQuery = "BEGIN; "
                        "UPDATE Lockers "
                        "SET owner = " + std::to_string(memberID) + ", "
                        "end_date = CURRENT_DATE + INTERVAL '30 days' "
                        "WHERE number = " + std::to_string(lockerID) + "; "
                        "COMMIT;";

    res = PQexec(conn, applyQuery.c_str());
    if (PQresultStatus(res) == PGRES_COMMAND_OK) {
        std::cout << "Locker successfully applied!\n";
    } else {
        std::cerr << "Failed to apply for locker. Rolling back...\n";
        PQexec(conn, "ROLLBACK;");
    }
    PQclear(res);
}

void requestWaiting(PGconn *conn, int memberID) {
    //unimplemented
}


void performMemberActions(PGconn *conn, int memberID) {
    int choice;
    while (true) {
        std::cout << "\n=== Member Menu ===\n";
        std::cout << "1. Request PT\n";
        std::cout << "2. Request Locker\n";
        std::cout << "3. Request Waiting\n";
        std::cout << "4. Logout\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                requestPT(conn, memberID);
                break;
            case 2:
                requestLocker(conn, memberID);
                break;
            case 3:
                requestWaiting(conn, memberID);
                break;
            case 4:
                std::cout << "Logging out...\n";
                return;
            default:
                std::cout << "Invalid choice. Please try again.\n";
        }
    }
}

int main() {
    PGconn* conn = connectDB();
    if (!conn) return 1;

    int userID; std::cout << "Enter ID: "; std::cin >> userID;
    std::string role; std::cout << "Enter role (admin, member, trainer, fdstaff): "; std::cin >> role;

    if (login(conn, userID, role)) std::cout << "Login successful as " << role << "!" << std::endl;
    else {
        std::cout << "Login failed!" << std::endl;
        PQfinish(conn);
        return 1;
    }

    if (login(conn, userID, role)) {
        if (role == "member") performMemberActions(conn, userID);
        else std::cout << "Other roles are not yet implemented.\n";
    }

    PQfinish(conn);
    return 0;
}

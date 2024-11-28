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

void requestWaiting(PGconn *conn, int userID, const std::string &role) {
    int equipmentID;

    // Step 1: 사용 가능한 운동기구 조회
    std::string query = "SELECT ID, name FROM Equipments;";
    PGresult *res = PQexec(conn, query.c_str());
    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        int rows = PQntuples(res);
        if (rows > 0) {
            std::cout << "Available equipment:\n";
            for (int i = 0; i < rows; ++i) {
                std::cout << "ID: " << PQgetvalue(res, i, 0)
                          << ", Name: " << PQgetvalue(res, i, 1) << "\n";
            }
        } else {
            std::cout << "No equipment available.\n";
            PQclear(res);
            return;
        }
    } else {
        std::cerr << "Error retrieving equipment list.\n";
        PQclear(res);
        return;
    }
    PQclear(res);

    // Step 2: 사용자 입력을 통한 운동기구 선택
    std::cout << "Enter the ID of the equipment you'd like to wait for: ";
    std::cin >> equipmentID;

    // Step 3: 대기열에 추가
    std::string waitQuery =
        "INSERT INTO Waitings (EID, ownerID, wait_num, role) "
        "VALUES (" +
        std::to_string(equipmentID) + ", " + std::to_string(userID) +
        ", (SELECT COALESCE(MAX(wait_num), 0) + 1 FROM Waitings WHERE EID = " + std::to_string(equipmentID) + "), '" + role + "');";

    res = PQexec(conn, waitQuery.c_str());
    if (PQresultStatus(res) == PGRES_COMMAND_OK) {
        std::cout << "You've been added to the waiting queue for equipment ID " << equipmentID << ".\n";
    } else {
        std::cerr << "Failed to join the waiting queue.\n"
                  << PQerrorMessage(conn);
    }
    PQclear(res);
}

void completeUsage(PGconn *conn, int userID, const std::string &role) {
    // Step 1: 현재 사용 중인 기구 확인 (wait_num = 1)
    std::string findQuery =
        "SELECT EID FROM Waitings WHERE wait_num = 1 AND ownerID = " + std::to_string(userID) + " AND role = '" + role + "';";
    PGresult *findRes = PQexec(conn, findQuery.c_str());

    if (PQresultStatus(findRes) != PGRES_TUPLES_OK || PQntuples(findRes) == 0) {
        std::cout << "No equipment currently in use for member ID " << userID << ".\n";
        PQclear(findRes);
        return;
    }

    int equipmentID = std::stoi(PQgetvalue(findRes, 0, 0));
    PQclear(findRes);

    // Step 2: 로그 기록
    std::string logQuery =
        "INSERT INTO WaitingLog (EID, uid, end_time) "
        "VALUES (" + std::to_string(equipmentID) + ", " + std::to_string(userID) + ", NOW());";
    PGresult *logRes = PQexec(conn, logQuery.c_str());

    if (PQresultStatus(logRes) != PGRES_COMMAND_OK) {
        std::cerr << "Failed to log equipment usage: " << PQerrorMessage(conn) << "\n";
        PQclear(logRes);
        return;
    }
    PQclear(logRes);

    // Step 3: 현재 사용자의 웨이팅 튜플 삭제
    std::string deleteQuery =
        "DELETE FROM Waitings WHERE EID = " + std::to_string(equipmentID) + " AND wait_num = 1;";
    PGresult *deleteRes = PQexec(conn, deleteQuery.c_str());

    if (PQresultStatus(deleteRes) != PGRES_COMMAND_OK) {
        std::cerr << "Failed to remove waiting entry: " << PQerrorMessage(conn) << "\n";
        PQclear(deleteRes);
        return;
    }
    PQclear(deleteRes);

    // Step 4: 다른 웨이팅 번호들을 1씩 감소
    std::string updateQuery =
        "UPDATE Waitings SET wait_num = wait_num - 1 WHERE EID = " + std::to_string(equipmentID) + " AND wait_num > 1;";
    PGresult *updateRes = PQexec(conn, updateQuery.c_str());

    if (PQresultStatus(updateRes) == PGRES_COMMAND_OK) {
        std::cout << "Waiting numbers for equipment ID " << equipmentID << " have been updated.\n";
    } else {
        std::cerr << "Failed to update waiting numbers: " << PQerrorMessage(conn) << "\n";
    }
    PQclear(updateRes);

    std::cout << "Usage completed and waiting queue updated successfully.\n";
}

void viewMemberInbody(PGconn *conn, int trainerID) {
    // Step 1: 트레이너와 PT 관계를 맺은 회원 목록 조회
    std::string query =
        "SELECT M.ID, M.name "
        "FROM Members M "
        "JOIN PTs P ON M.ID = P.MID "
        "WHERE P.TID = " + std::to_string(trainerID) + ";";

    PGresult *res = PQexec(conn, query.c_str());

    // Step 2: 결과 확인
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Failed to retrieve members list: " << PQerrorMessage(conn) << "\n";
        PQclear(res);
        return;
    }

    int rows = PQntuples(res);
    if (rows == 0) {
        std::cout << "No members are currently assigned to you.\n";
        PQclear(res);
        return;
    }

    // Step 3: 회원 목록 출력
    std::cout << "Your members:\n";
    for (int i = 0; i < rows; ++i) {
        std::cout << "ID: " << PQgetvalue(res, i, 0)
                  << ", Name: " << PQgetvalue(res, i, 1) << "\n";
    }

    PQclear(res);

    // Step 4: 회원 ID 선택
    int selectedMemberID;
    std::cout << "Enter the ID of the member to view their inbody information: ";
    std::cin >> selectedMemberID;

    // Step 5: 선택된 회원의 모든 인바디 정보 조회
    query =
        "SELECT date, height, weight, muscle, score "
        "FROM Inbodys "
        "WHERE MID = " + std::to_string(selectedMemberID) +
        " ORDER BY date DESC;";

    res = PQexec(conn, query.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        std::cerr << "Failed to retrieve inbody information: " << PQerrorMessage(conn) << "\n";
        PQclear(res);
        return;
    }

    rows = PQntuples(res);
    if (rows == 0) {
        std::cout << "No inbody information available for the selected member.\n";
    } else {
        std::cout << "Inbody Information (latest first):\n";
        for (int i = 0; i < rows; ++i) {
            std::cout << "Date: " << PQgetvalue(res, i, 0)
                      << ", Height: " << PQgetvalue(res, i, 1) << " cm"
                      << ", Weight: " << PQgetvalue(res, i, 2) << " kg"
                      << ", Muscle: " << PQgetvalue(res, i, 3) << " kg"
                      << ", Score: " << PQgetvalue(res, i, 4) << " points\n";
        }
    }

    PQclear(res);
}

void viewSalary(PGconn *conn, int ID, const std::string &role) {
    // Check if the role is valid
    if (role != "trainer" && role != "fdstaff") {
        std::cerr << "Invalid role! Please specify 'trainer' or 'fdstaff'.\n";
        return;
    }

    std::string query;

    // Build the query based on the role
    if (role == "trainer") {
        query =
            "SELECT T.ID, T.name, L.salary * T.PT_count * 2 AS TotalSalary "
            "FROM Trainers T "
            "JOIN Levels L ON T.level = L.level "
            "WHERE T.ID = " + std::to_string(ID) + ";";
    } else if (role == "fdstaff") {
        query =
            "SELECT F.ID, F.name, L.salary * F.attendance_days AS TotalSalary "
            "FROM FDStaffs F "
            "JOIN Levels L ON F.level = L.level "
            "WHERE F.ID = " + std::to_string(ID) + ";";
    }

    // Execute the query
    PGresult *res = PQexec(conn, query.c_str());

    if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        if (PQntuples(res) == 0) {
            std::cout << "No salary data found for the specified " << role << ".\n";
        } else {
            std::cout << "Salary Details:\n";
            std::cout << "ID: " << PQgetvalue(res, 0, 0)
                      << ", Name: " << PQgetvalue(res, 0, 1)
                      << ", Total Salary: " << PQgetvalue(res, 0, 2) << " KRW\n";
        }
    } else {
        std::cerr << "Error executing salary query: " << PQerrorMessage(conn) << "\n";
    }

    PQclear(res);
}


void performMemberActions(PGconn *conn, int memberID, const std::string &role) {
    int choice;
    while (true) {
        std::cout << "\n=== Member Menu ===\n";
        std::cout << "1. Request PT\n";
        std::cout << "2. Request Locker\n";
        std::cout << "3. Request Waiting\n";
        std::cout << "4. Complete Usage\n";
        std::cout << "5. Logout\n";
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
                requestWaiting(conn, memberID, role);
                break;
            case 4:
                completeUsage(conn, memberID, role);
                break;
            case 5:
                std::cout << "Logging out...\n";
                return;
            default:
                std::cout << "Invalid choice. Please try again.\n";
        }
    }
}

void performTrainerActions(PGconn *conn, int trainerID, const std::string &role) {
    int choice;
    while (true) {
        std::cout << "\n=== Trainer Menu ===\n";
        std::cout << "1. View PT member's Inbody\n";
        std::cout << "2. View Salary \n";
        std::cout << "3. Request Waiting\n";
        std::cout << "4. Complete Usage\n";
        std::cout << "5. Logout\n";
        std::cout << "Enter your choice: ";
        std::cin >> choice;

        switch (choice) {
            case 1:
                viewMemberInbody(conn, trainerID);
                break;
            case 2:
                viewSalary(conn, trainerID, role);
                break;
            case 3:
                requestWaiting(conn, trainerID, role);
                break;
            case 4:
                completeUsage(conn, trainerID, role);
                break;
            case 5:
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
        if (role == "member") performMemberActions(conn, userID, role);
        else if (role == "trainer") performTrainerActions(conn, userID, role);
        //else if (role == "fdstaff") performFDStaffActions(conn, userID);
        //else if (role == "admin") performAdminActions(conn);
        else std::cout << "Other roles are not yet implemented.\n";
    }

    PQfinish(conn);
    return 0;
}

#include "db/MySQLConn.h"

MySQLConn::MySQLConn() {
    // 初始化 MySQL 环境
    conn_ = mysql_init(nullptr);
    // 设置字符集，防止中文乱码 (对应我们在 SQL 里的 utf8mb4)
    mysql_set_character_set(conn_, "utf8mb4");
    result_ = nullptr;
}

MySQLConn::~MySQLConn() {
    freeResult();
    if (conn_ != nullptr) {
        mysql_close(conn_); // 断开连接
    }
}

bool MySQLConn::connect(const std::string& user, const std::string& passwd, 
                        const std::string& dbName, const std::string& ip, 
                        unsigned short port) {
    // 尝试连接，成功返回 conn_，失败返回 nullptr
    MYSQL* p = mysql_real_connect(conn_, ip.c_str(), user.c_str(), 
                                  passwd.c_str(), dbName.c_str(), port, nullptr, 0);
    if (p == nullptr) {
        std::cerr << "[DB Error] Connect Failed: " << mysql_error(conn_) << std::endl;
    }
    return p != nullptr;
}

bool MySQLConn::update(const std::string& sql) {
    // mysql_query 执行成功返回 0，非 0 则是失败
    if (mysql_query(conn_, sql.c_str())) {
        std::cerr << "[DB Error] Update Failed: " << sql << "\nReason: " << mysql_error(conn_) << std::endl;
        return false;
    }
    return true;
}

bool MySQLConn::query(const std::string& sql) {
    freeResult(); // 查询前先清空上一次的结果
    
    if (mysql_query(conn_, sql.c_str())) {
        std::cerr << "[DB Error] Query Failed: " << sql << "\nReason: " << mysql_error(conn_) << std::endl;
        return false;
    }
    
    // 把服务器的查询结果保存到本地内存
    result_ = mysql_store_result(conn_);
    return result_ != nullptr;
}

bool MySQLConn::next() {
    if (result_ != nullptr) {
        // 获取下一行数据
        row_ = mysql_fetch_row(result_);
        return row_ != nullptr;
    }
    return false;
}

std::string MySQLConn::value(int index) {
    // 工业级防御：如果行有数据且索引没越界
    int colCount = mysql_num_fields(result_);
    if (row_ != nullptr && index >= 0 && index < colCount) {
        // 如果字段值为 NULL，保护性地返回空字符串
        char* val = row_[index];
        if (val == nullptr) {
            return std::string();
        }
        return std::string(val);
    }
    return std::string();
}

void MySQLConn::freeResult() {
    if (result_) {
        mysql_free_result(result_);
        result_ = nullptr;
    }
}
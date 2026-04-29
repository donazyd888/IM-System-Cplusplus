#include <iostream>
#include <string>
#include <unordered_map> // 引入哈希表
#include "network/EventLoop.h"
#include "network/TcpServer.h"
#include "utils/Buffer.h"
#include "db/MySQLConn.h"
#include <csignal>

#include "third_party/json.hpp"
using json = nlohmann::json;

int main() {
    // 忽略 SIGPIPE 信号，防止往已断开的 socket 写数据时导致程序崩溃
    signal(SIGPIPE, SIG_IGN);
    EventLoop loop;
    TcpServer server(&loop, "127.0.0.1", 8888);

    // 🌟 核心花名册：记录所有在线的【用户名 -> 连接对象】
    std::unordered_map<std::string, std::shared_ptr<TcpConnection>> online_users;

    server.setConnectionCallback([&online_users](const std::shared_ptr<TcpConnection>& conn) {
        if (conn->connected()) {
            std::cout << "--- Client Connected, FD: " << conn->getFd() << " ---" << std::endl;
        } else {
            std::cout << "--- Client Disconnected, FD: " << conn->getFd() << " ---" << std::endl;
            for (auto it = online_users.begin(); it != online_users.end(); ++it) {
                if (it->second == conn) {
                    std::cout << "[Business] User '" << it->first << "' offline, removed from routing table." << std::endl;
                    online_users.erase(it);
                    break;
                }
            }
        }
    });

    server.setMessageCallback([&online_users](const std::shared_ptr<TcpConnection>& conn, Buffer* buffer) {
        std::string raw_msg = buffer->retrieveAllAsString();
        std::cout << "[Raw Data] " << raw_msg << std::endl;

        try {
            json j = json::parse(raw_msg);
            std::string type = j["type"];

            if (type == "login") {
                std::string user = j["data"]["username"];
                std::string pwd = j["data"]["password"];
                json response;
                response["type"] = "login_ack";

                MySQLConn db;
                if (db.connect("root", "00008888", "reactor_im", "127.0.0.1")) {
                    char sql[256];
                    // 🌟 修改：同时查出 user_id, password 和 avatar_url
                    snprintf(sql, sizeof(sql), "SELECT user_id, password, avatar_url FROM users WHERE username='%s'", user.c_str());

                    if (db.query(sql) && db.next()) {
                        std::string db_uid = db.value(0);
                        std::string db_pwd = db.value(1);
                        std::string db_avatar = db.value(2); // 获取头像

                        if (db_pwd == pwd) {
                            response["data"]["status"] = "success";
                            response["data"]["msg"] = "Login successful!";
                            // 🌟 新增：把用户的 ID 和头像返回给前端
                            response["data"]["user_id"] = std::stoi(db_uid);
                            response["data"]["username"] = user;
                            response["data"]["avatar_url"] = db_avatar;

                            online_users[user] = conn;

                            conn->send(response.dump() + "\n");

                            // 拉取未读的离线消息
                            char pull_sql[512];
                            snprintf(pull_sql, sizeof(pull_sql),
                                     "SELECT m.msg_id, u.username, m.content "
                                     "FROM messages m JOIN users u ON m.sender_id = u.user_id "
                                     "WHERE m.receiver_id = %s AND m.is_read = 0", db_uid.c_str());

                            if (db.query(pull_sql)) {
                                std::vector<std::string> read_msg_ids;
                                while (db.next()) {
                                    std::string msg_id = db.value(0);
                                    std::string sender_name = db.value(1);
                                    std::string msg_content = db.value(2);

                                    json offline_msg;
                                    offline_msg["type"] = "chat";
                                    offline_msg["data"]["from_user"] = sender_name;
                                    offline_msg["data"]["content"] = msg_content;
                                    offline_msg["data"]["is_offline_history"] = true;

                                    conn->send(offline_msg.dump() + "\n");
                                    read_msg_ids.push_back(msg_id);
                                }

                                for (const auto& id : read_msg_ids) {
                                    char update_sql[128];
                                    snprintf(update_sql, sizeof(update_sql), "UPDATE messages SET is_read = 1 WHERE msg_id = %s", id.c_str());
                                    db.update(update_sql);
                                }
                            }
                            return;
                        } else {
                            response["data"]["status"] = "error";
                            response["data"]["msg"] = "Incorrect password";
                        }
                    } else {
                        response["data"]["status"] = "error";
                        response["data"]["msg"] = "User not found";
                    }
                }
                conn->send(response.dump() + "\n");
            }
            else if (type == "register") {
                // 原有注册逻辑保持不变...
                std::string user = j["data"]["username"];
                std::string pwd = j["data"]["password"];
                json response;
                response["type"] = "register_ack";

                MySQLConn db;
                if (db.connect("root", "00008888", "reactor_im", "127.0.0.1")) {
                    char check_sql[256];
                    snprintf(check_sql, sizeof(check_sql), "SELECT user_id FROM users WHERE username='%s'", user.c_str());

                    if (db.query(check_sql) && db.next()) {
                        response["data"]["status"] = "error";
                        response["data"]["msg"] = "Username already exists";
                    } else {
                        char insert_sql[256];
                        snprintf(insert_sql, sizeof(insert_sql),
                                 "INSERT INTO users(username, password) VALUES('%s', '%s')",
                                 user.c_str(), pwd.c_str());

                        if (db.update(insert_sql)) {
                            response["data"]["status"] = "success";
                            response["data"]["msg"] = "Registration successful!";
                        } else {
                            response["data"]["status"] = "error";
                            response["data"]["msg"] = "Registration failed, database error";
                        }
                    }
                } else {
                    response["data"]["status"] = "error";
                    response["data"]["msg"] = "Database connection failed";
                }
                conn->send(response.dump() + "\n");
            }
            else if (type == "chat") {
                // 原有聊天逻辑保持不变...
                std::string from = j["data"]["from_user"];
                std::string to = j["data"]["to_user"];
                std::string content = j["data"]["content"];

                std::cout << "[Chat] " << from << " -> " << to << " : " << content << std::endl;

                if (online_users.find(to) != online_users.end()) {
                    json forward_msg;
                    forward_msg["type"] = "chat";
                    forward_msg["data"]["from_user"] = from;
                    forward_msg["data"]["content"] = content;

                    online_users[to]->send(forward_msg.dump() + "\n");

                    json ack;
                    ack["type"] = "chat_ack";
                    ack["data"]["status"] = "success";
                    conn->send(ack.dump() + "\n");
                } else {
                    json ack;
                    ack["type"] = "chat_ack";

                    MySQLConn db;
                    if (db.connect("root", "00008888", "reactor_im", "127.0.0.1")) {
                        std::string from_id, to_id;
                        char id_sql[256];

                        snprintf(id_sql, sizeof(id_sql), "SELECT user_id FROM users WHERE username='%s'", from.c_str());
                        if (db.query(id_sql) && db.next()) from_id = db.value(0);

                        snprintf(id_sql, sizeof(id_sql), "SELECT user_id FROM users WHERE username='%s'", to.c_str());
                        if (db.query(id_sql) && db.next()) to_id = db.value(0);

                        if (!from_id.empty() && !to_id.empty()) {
                            char insert_sql[512];
                            snprintf(insert_sql, sizeof(insert_sql),
                                     "INSERT INTO messages(sender_id, receiver_id, content) VALUES(%s, %s, '%s')",
                                     from_id.c_str(), to_id.c_str(), content.c_str());

                            if (db.update(insert_sql)) {
                                ack["data"]["status"] = "offline_saved";
                                ack["data"]["msg"] = "User " + to + " is offline. Message saved to server.";
                            } else {
                                ack["data"]["status"] = "error";
                                ack["data"]["msg"] = "Database error while saving message.";
                            }
                        } else {
                            ack["data"]["status"] = "error";
                            ack["data"]["msg"] = "Invalid sender or receiver.";
                        }
                    } else {
                        ack["data"]["status"] = "error";
                        ack["data"]["msg"] = "Database connection failed.";
                    }
                    conn->send(ack.dump() + "\n");
                }
            }
            // 🌟 新增：获取好友列表逻辑
            else if (type == "get_friend_list") {
                int uid = j["data"]["user_id"];
                json response;
                response["type"] = "get_friend_list_ack";
                response["code"] = 200;
                json data_array = json::array();

                MySQLConn db;
                if (db.connect("root", "00008888", "reactor_im", "127.0.0.1")) {
                    char sql[512];
                    // 联表查询：查 friends 表找到好友的 user_id，再关联 users 表获取用户名和头像
                    snprintf(sql, sizeof(sql),
                             "SELECT u.user_id, u.username, u.avatar_url "
                             "FROM friends f JOIN users u ON f.friend_id = u.user_id "
                             "WHERE f.user_id = %d", uid);

                    if (db.query(sql)) {
                        while (db.next()) {
                            json f_obj;
                            f_obj["user_id"] = std::stoi(db.value(0));
                            f_obj["username"] = db.value(1);
                            f_obj["avatar_url"] = db.value(2);
                            data_array.push_back(f_obj);
                        }
                    }
                }
                response["data"] = data_array;
                conn->send(response.dump() + "\n");
            }
            // 🌟 新增：添加好友逻辑
            else if (type == "add_friend") {
                int my_id = j["data"]["user_id"];
                std::string f_username = j["data"]["friend_username"];
                json response;
                response["type"] = "add_friend_ack";

                MySQLConn db;
                if (db.connect("root", "00008888", "reactor_im", "127.0.0.1")) {
                    char sql[256];
                    // 先根据提供的用户名查找对方是否存在
                    snprintf(sql, sizeof(sql), "SELECT user_id, avatar_url FROM users WHERE username='%s'", f_username.c_str());
                    if (db.query(sql) && db.next()) {
                        std::string f_id_str = db.value(0);
                        std::string f_avatar = db.value(1);
                        int f_id = std::stoi(f_id_str);

                        // 检查是否已是好友，防止重复添加
                        char check_sql[256];
                        snprintf(check_sql, sizeof(check_sql),
                                 "SELECT COUNT(*) FROM friends WHERE user_id=%d AND friend_id=%d", my_id, f_id);
                        if (db.query(check_sql) && db.next() && std::stoi(db.value(0)) > 0) {
                            response["code"] = 409;
                            response["msg"] = "对方已是您的好友，请勿重复添加";
                            conn->send(response.dump() + "\n");
                            return;
                        }

                        char insert_sql[256];
                        snprintf(insert_sql, sizeof(insert_sql),
                                 "INSERT INTO friends(user_id, friend_id) VALUES(%d, %d)", my_id, f_id);

                        if (db.update(insert_sql)) {
                            response["code"] = 200;
                            response["msg"] = "添加成功";
                            // 返回新好友的信息供前端渲染
                            response["data"]["user_id"] = f_id;
                            response["data"]["username"] = f_username;
                            response["data"]["avatar_url"] = f_avatar;
                        } else {
                            response["code"] = 500;
                            response["msg"] = "数据库插入失败";
                        }
                    } else {
                        response["code"] = 404;
                        response["msg"] = "用户不存在";
                    }
                } else {
                    response["code"] = 500;
                    response["msg"] = "数据库连接失败";
                }
                conn->send(response.dump() + "\n");
            }
            else {
                std::cout << "[Business] Unknown message type: " << type << std::endl;
            }

        } catch (const std::exception& e) {
            std::cerr << "[Error] JSON Parse Exception: " << e.what() << " | Raw: " << raw_msg << std::endl;
        }
    });

    std::cout << "Reactor IM Server (Routing Enabled) started..." << std::endl;
    server.start();
    loop.loop();

    return 0;
}
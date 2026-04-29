const { app, BrowserWindow, ipcMain } = require('electron');
const net = require('net');

let mainWindow;
let tcpClient = new net.Socket();

// 🌟 状态升级：将原本的字符串改成对象，保存 ID 和头像
let currentUser = {
    username: "",
    user_id: null,
    avatar_url: ""
};

let isChatReady = false;
let messageCache = [];
let tcpBuffer = "";

function createWindow () {
    mainWindow = new BrowserWindow({
        width: 400,
        height: 500,
        webPreferences: {
            nodeIntegration: true,
            contextIsolation: false
        }
    });
    mainWindow.loadFile('index.html');
}

app.whenReady().then(createWindow);

tcpClient.connect(8888, '127.0.0.1', function() {
    console.log('Connected to C++ Server successfully!');
});

tcpClient.on('data', function(data) {
    tcpBuffer += data.toString();
    let messages = tcpBuffer.split('\n');

    tcpBuffer = messages.pop();

    for (let msg of messages) {
        if (!msg.trim()) continue;
        console.log('后台分发数据:', msg);

        try {
            let j = JSON.parse(msg);

            if (j.type === 'login_ack' || j.type === 'register_ack') {
                if (mainWindow) mainWindow.webContents.send('server-reply', msg);
            }
            // 🌟 路由升级：把好友相关的 ack 也纳入聊天界面的分发逻辑中
            else if (j.type === 'chat' || j.type === 'chat_ack' ||
                j.type === 'get_friend_list_ack' || j.type === 'add_friend_ack') {

                if (isChatReady && mainWindow) {
                    mainWindow.webContents.send('server-reply', msg);
                } else {
                    messageCache.push(msg);
                }
            }
        } catch (e) {
            console.error("JSON Parse Error in main.js:", e);
        }
    }
});

ipcMain.on('send-to-server', (event, jsonString) => {
    tcpClient.write(jsonString + '\n');
});

// 🌟 接口升级：接收从 index.html 传来的完整用户信息对象
ipcMain.on('login-success', (event, userInfo) => {
    currentUser = userInfo; // 现在 userInfo 包含了 user_id, username, avatar_url
    isChatReady = false;
    if (mainWindow) {
        mainWindow.setSize(850, 600);
        mainWindow.center();
        mainWindow.loadFile('chat.html');
    }
});

// 🌟 接口升级：当 chat.html 询问时，返回完整的用户信息对象
ipcMain.handle('get-current-user', () => {
    return currentUser;
});

ipcMain.handle('chat-ui-ready', () => {
    isChatReady = true;
    let cached = [...messageCache];
    messageCache = [];
    return cached;
});
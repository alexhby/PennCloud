// Frontend internal server - interact with storage and master
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <utility>

using namespace std;

const int BUF_SIZE = 1000000;               // TODO decide with backend and http server
bool debugMode = true;
// // render.cpp
// string renderPage(string pagename);
// string renderDrive(vector<string> filenames);


vector<struct sockaddr_in> getAddrs (string address_file);
pair<string, string> parseQuery(string query, string name1, string name2);
string parseQuery(string query, string name);

// locally check for naming constrains (username/filename/foldername)
bool checkName(string name);

// Login session:
int checkLogin(vector<struct sockaddr_in> backendAddrs, string user, string pass);
int createNewUser(vector<struct sockaddr_in> backendAddrs, string user, string pass);
bool changePassword(struct sockaddr_in backendAddr, string user, string old_pass, string new_pass);
bool setSID(struct sockaddr_in backendAddr, string user, int sid);
int getSID(struct sockaddr_in backendAddr, string user);

// Driver functions:
// Return files and folders given a current folder path
vector<string> listFiles(struct sockaddr_in backendAddr, string user, string currentPath);
vector<string> allPaths(struct sockaddr_in backendAddr, string user, string currentPath);      // All paths of a user except for currentpath

bool newfile(struct sockaddr_in backendAddr, string user, string filename, string value);
bool newfolder(struct sockaddr_in backendAddr, string user, string folder);
bool deleteByPath(struct sockaddr_in backendAddr, string user, string path);
bool moveFile(struct sockaddr_in backendAddr, string user, string oldpath, string newpath);
bool renameFile(struct sockaddr_in backendAddr, string user, string oldname, string newname);
string getFile(struct sockaddr_in backendAddr, string user, string file);


// Admin console
vector<bool> getStatus(vector<struct sockaddr_in> addrs); // Run once for backend, once for frontend
string getRaw(struct sockaddr_in backendAddr);            // Get backend raw data



// For sending to backend
bool sendPut(int sock, string r, string c, string v);
bool sendGet(int sock, string r, string c);
bool sendCput(int sock, string r, string c, string v1, string v2);
bool sendDelete(int sock, string r, string c);
bool sendMove(int sock, string r, string oldpath, string newpath);
bool sendRename(int sock, string r, string c_old, string c_new);

bool do_read(int fd, char *buf, int len);
bool do_write(int fd, char *buf, int len);
bool str_write(int fd, string str);


// for testing
// int main(int argc, char *argv[]) {
//     vector<struct sockaddr_in> backendAddrs = getAddrs("servers.txt");
//     int backendCount = backendAddrs.size();

//     string user = "alex";
//     string pass = "1234";
//     string newpass = "5678";
//     int sid = 789;
//     string folder1 = "d>$folder1";
//     string file1 = "d>$folder1>#f1";
//     string file1_new = "d>#f1";
//     string file2 = "d>$folder1>#f2";
//     string file2_new = "d>$folder1>#f2new";
//     string file3 = "d>#f3";

//     int back_index = createNewUser(backendAddrs, user, pass);
//       if (back_index > 0 && back_index < backendAddrs.size()){
//          cerr << "test1 passed\n";
//       } else {
// 	cerr << "test1 failed!\n";
// 	exit(1);
//       }

//     bool status = changePassword(backendAddrs[back_index], user, pass, newpass);
//     if (status){
//         cerr << "test2 passed\n";
//     } 

//     status = setSID(backendAddrs[back_index], user, sid);
//     if (status){
//         cerr << "test3 passed\n";
//     } 

//     int sid_get = getSID(backendAddrs[back_index], user);
//     if (sid_get == sid){
//         cerr << "test4 passed\n";
//     }

//     status = newfolder(backendAddrs[back_index], user, folder1);
//     if (status){
//         cerr << "test5 passed\n";
//     } 

//     status = newfile(backendAddrs[back_index], user, file1, "abc");
//     if (status){
//         cerr << "test6 passed\n";
//     }  

//     status = newfile(backendAddrs[back_index], user, file2, "efg");
//     if (status){
//         cerr << "test7 passed\n";
//     } 

//     status = newfile(backendAddrs[back_index], user, file3, "hyj");
//     if (status){
//         cerr << "test8 passed\n";
//     } 

//     status = deleteByPath(backendAddrs[back_index], user, file3);
//     if (status){
//         cerr << "test9 passed\n";
//     } 

//     status = moveFile(backendAddrs[back_index], user, file1, file1_new);
//     if (status){
//         cerr << "test10 passed\n";
//     } 

//     status = renameFile(backendAddrs[back_index], user, file2, file2_new);
//     if (status){
//         cerr << "test11 passed\n";
//     } 

//     vector<string> filelist = listFiles(backendAddrs[back_index], user, folder1);
//     if (status){
//         cerr << "test12 passed\n";
//     } 
//     // // Given query
//     // string query = "user=xxx&pass=xxx"; //TODO
//     // pair<string,string> user_pass = parseLoginQuery(query);
//     // string user = user_pass.first;
//     // string pass = user_pass.second;
//     // int index = checkLogin(backendAddrs, user, pass); // get the backend storage index
//     // if (index == -1) {
//     //     string page = renderPage("login_failed.html");
//     //     // TODO send http
//     // } else {
//     //     string page = renderPage("success_home.html");
//     //     // TODO send http OK, set cookie
//     // }



// }

// check for naming constrains (username/filename/foldername)
bool checkName(string name) {
    if (name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ01234567890_-") != string::npos)
    {
        return false;
    }
    return true;
}

vector<struct sockaddr_in> getAddrs (string address_file) {

    // Open backend_address_file
    ifstream ifs(address_file);
    if (!ifs.is_open()) {
        cerr << "Error: Fail to open the address file\n";
        exit(1);
    }

    // Read all ip:port from addr file
    // line 1: master, line 2: shadow master
    vector<string> ips;
    vector<int> ports;

    int count = 0;
    string line;
    while (getline(ifs, line)) {
        count++;

        ips.push_back( line.substr(0, line.find(':')) ); // Get a backend IP

        size_t found = line.find(':');
        if (found == string::npos) {
            cerr << "Error: Invalid ip at line " << count << endl;
            exit(1);
        }
        try {                                                  // Get a backend port
            ports.push_back( stoi(line.substr(found + 1), NULL, 10) );
        }
        catch (...) {
            cerr << "Error: Invalid port at line " << count << endl;
            exit(1);
        }
        if (ports.back() < 1 || ports.back() > 65535) {
            cerr << "Error: Invalid port at line " << count << endl;
            exit(1);
        }
    }
    // cerr << "Count: " << count << endl;

    // Connect to backend servers
    vector<struct sockaddr_in> addrs;
    for (int i = 0; i < count; i++) {
        // cerr << "In loop!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
        struct sockaddr_in addr;
        bzero(&addr, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(ports[i]);
        inet_pton(AF_INET, ips[i].c_str(), &(addr.sin_addr));
        addrs.push_back(addr);
    }
    // cerr << "(getAddr) size is " << addrs.size() << endl;
    return addrs;
}


// query format: name1=value1&name2=value2
pair<string, string> parseQuery(string query, string name1, string name2) {
    string value1 = query.substr(query.find('=') + 1, query.find('&') - query.find('=') - 1);

    string query2 = query.substr(query.find('&') + 1);
    string value2 = query2.substr(query2.find('=') + 1);
    string result1 = "";
    string result2 = "";
    if (query.substr(0, query.find('=')) == name1) result1 = value1;
    if (query2.substr(0, query2.find('=')) == name2) result2 = value2;

    if (result1.size() == 0 || result2.size() == 0) return make_pair("", "");
    return make_pair(result1, result2);
}

// format
string parseQuery(string query, string name) {
    if (query.substr(0, query.find('=')) == name) {
        return query.substr(query.find('=') + 1);
    }
    return "";
}



// If successful: return backend storage index, else -1
int checkLogin(vector<struct sockaddr_in> backendAddrs, string user, string pass) {

    // create new sock
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }

    // Connect to master
    connect(sock, (struct sockaddr*)&backendAddrs[0], sizeof(backendAddrs[0]));

    // Compare with backend database
    string msg = "request`";
    msg += user;
    msg += "`\n";

    str_write(sock, msg);               // ask master
    char indexBuf[101];
    bzero(&indexBuf[0], sizeof(indexBuf));


    do_read(sock, indexBuf, 100);       // get backend index from master
    close(sock);
    if (debugMode) cerr << "[master] (checkLogin) sent: " << indexBuf << endl;

    if (indexBuf[0] == '-') return -1;  // case of error

    int index = -1;
    try {
        index = stoi(indexBuf, NULL, 10);
    }
    catch (...) {
        return -1;
    }
    if (index <= 0 || index >= backendAddrs.size()) {
        // including the case when username not found
        return -1;
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddrs[index], sizeof(backendAddrs[index]));

    sendGet(sock, user, "pwd");
    char pwd[51];
    bzero(&pwd[0], sizeof(pwd));
    do_read(sock, pwd, 50);
    close(sock);
    if (debugMode) cerr << "[B" << index << "] sent password: " << pwd << endl;

    if (pass == string(pwd)) {
        return index;
    } else {
        return -1;
    }
}

// Return backend server index, if create new success, otherwise -1
int createNewUser(vector<struct sockaddr_in> backendAddrs, string user, string pass) {
    // create new sock
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to master
    if (connect(sock, (struct sockaddr*)&backendAddrs[0], sizeof(backendAddrs[0])) != 0) {
        cerr << "connect failed " << endl;
        exit(1);
    }

    // Create a new user, error if username already exists
    string msg = "create`";
    msg += user;
    msg += "`\n";
    str_write(sock, msg);               // ask master
    char indexBuf[101];
    bzero(&indexBuf[0], sizeof(indexBuf));

    do_read(sock, indexBuf, 100);       // get backend index from master
    close(sock);


    if (debugMode) cerr << "[master] (createNewUser) sent: " << indexBuf << endl;

    if (indexBuf[0] == '-') return -1;  // case of error
    printf("indexBuf is %s\n",indexBuf);

    int index = -1;
    try {
        index = stoi(indexBuf, NULL, 10);

    }
    catch (...) {
        return -1;
    }
    if (index <= 0 || index >= backendAddrs.size()) {
        // including the case when username not found
        return -1;
    }
    printf("index 2 is %d\n",index);
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddrs[index], sizeof(backendAddrs[index]));
    printf("index 3 is %d\n",index);
    sendPut(sock, user, "pwd", pass);
    char response[51];
    bzero(&response[0], sizeof(response));

    do_read(sock, response, 50);

    if (debugMode) cerr << "[B" << index << "] " << response << endl;
    if (response[0] != '+') return -1;
    printf("index 4 is %d\n",index);
    // create root folder 'd'
    bool status = newfolder(backendAddrs[index], user, "d");
    close(sock);
    if (status) {
        printf("right\n");
        return index;
    } else {
        printf("wrong\n");
        return -1;
    }


}

bool logout(struct sockaddr_in backendAddr, int sock, string user) {
    //TODO
    return false;
}

bool changePassword(struct sockaddr_in backendAddr, string user, string old_pass, string new_pass) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddr, sizeof(backendAddr));
    sendCput(sock, user, "pwd", old_pass, new_pass);
    //wait for ACK
    char response[51];
    bzero(&response[0], sizeof(response));

    do_read(sock, response, 50);
    close(sock);
    if (response[0] == '+') {
        return true;
    } else {
        return false;
    }
}

bool setSID(struct sockaddr_in backendAddr, string user, int sid) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddr, sizeof(backendAddr));
    sendPut(sock, user, "sid", to_string(sid));
    //wait for ACK
    char response[51];
    bzero(&response[0], sizeof(response));

    do_read(sock, response, 50);
    close(sock);
    if (response[0] == '+') {
        return true;
    } else {
        return false;
    }
}

int getSID(struct sockaddr_in backendAddr, string user) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddr, sizeof(backendAddr));
    sendGet(sock, user, "sid");
    //wait for ACK
    char response[101];
    bzero(&response[0], sizeof(response));

    do_read(sock, response, 100);
    close(sock);
    if (debugMode) cerr << "[B] " << response << endl;

    int sid = -1;
    try {
        sid = stoi(response, NULL, 10);
    }
    catch (...) {
        cerr << "Error: Invalid sid from backend" << endl;
        exit(1);
    }
    return sid;
}



// path: path of another file: 'd>$folder1>$folder2>#file2'
// name: name of the file/folder: '#file1' or '$folderA'
// output: 'd>$folder1>$folder2>#file1'
// string nameToFullpath(string path, string name){
//     path = path.substr(0, path.find_last_of('>')+1);     // remove the file/folder name of the given path
//     path += name;
//     return path;
// }



// Return files and folders
vector<string> listFiles(struct sockaddr_in backendAddr, string user, string currentPath) {

    // create new sock
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    printf("before connect\n");
    connect(sock, (struct sockaddr*)&backendAddr, sizeof(backendAddr));

    // Ask backend for list of files
    sendGet(sock, user, currentPath);
    char filelist[BUF_SIZE];
    bzero(&filelist[0], sizeof(filelist));

    printf("before doread\n");
    do_read(sock, filelist, BUF_SIZE - 1);
    printf("this is filelist %s\n", filelist);
    close(sock);
    if (debugMode) cerr << "[B] listFiles: " << filelist << endl;

    printf("before filenames\n");
    vector<string> filenames;
    char *piece;
    piece = strtok(filelist, " `");
    while (piece != NULL) {
        filenames.push_back(string(piece));
        piece = strtok(NULL, " `");
    }
    printf("before return\n");
    return filenames;
}


vector<string> allPaths(struct sockaddr_in backendAddr, string user, string currentPath){
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddr, sizeof(backendAddr));

    string msg("ALLPATH`");
    msg += user;
    msg += "`\n";
    str_write(sock, msg);

    // Get results
    char buf[BUF_SIZE];
    bzero(&buf[0], sizeof(buf));

    do_read(sock, buf, BUF_SIZE - 1);
    close(sock);
    if (debugMode) cerr << "[B] allPaths: " << buf << endl;

    vector<string> paths;
    char *piece;
    piece = strtok(buf, " `");
    while (piece != NULL) {
        string path = string(piece);
        if(path != currentPath) paths.push_back(path);  // Do not add current path
        piece = strtok(NULL, " `");
    }

    return paths;
}




string getFile(struct sockaddr_in backendAddr, string user, string file) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddr, sizeof(backendAddr));

    // Ask backend for list of files
    sendGet(sock, user, file);
    char buf[BUF_SIZE];
    bzero(&buf[0], sizeof(buf));

    do_read(sock, buf, BUF_SIZE - 1);
    close(sock);
    if (debugMode) cerr << "[B] sent content of " << file << endl;

    return string(buf);
}


bool newfile(struct sockaddr_in backendAddr, string user, string filename, string value) {
    // create new sock
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddr, sizeof(backendAddr));
    sendPut(sock, user, filename, value);
    //wait for ACK
    char response[51];
    bzero(&response[0], sizeof(response));

    do_read(sock, response, 50);
    close(sock);
    if (response[0] == '+') {
        return true;
    } else {
        return false;
    }
}

bool newfolder(struct sockaddr_in backendAddr, string user, string folder) {
    // create new sock
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddr, sizeof(backendAddr));
    sendPut(sock, user, folder, "");
    //wait for ACK
    char response[51];
    bzero(&response[0], sizeof(response));

    do_read(sock, response, 50);
    close(sock);
    if (response[0] == '+') {
        return true;
    } else {
        return false;
    }
}



// file or folder
bool deleteByPath(struct sockaddr_in backendAddr, string user, string path) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddr, sizeof(backendAddr));
    sendDelete(sock, user, path);
    //wait for ACK
    char response[51];
    bzero(&response[0], sizeof(response));

    do_read(sock, response, 50);
    close(sock);
    if (response[0] == '+') {
        return true;
    } else {
        return false;
    }
}

bool moveFile(struct sockaddr_in backendAddr, string user, string oldpath, string newpath) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddr, sizeof(backendAddr));
    sendMove(sock, user, oldpath, newpath);
    //wait for ACK
    char response[51];
    bzero(&response[0], sizeof(response));

    do_read(sock, response, 50);
    close(sock);
    if (response[0] == '+') {
        return true;
    } else {
        return false;
    }
}

bool renameFile(struct sockaddr_in backendAddr, string user, string oldname, string newname) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddr, sizeof(backendAddr));
    sendRename(sock, user, oldname, newname);
    //wait for ACK
    char response[51];
    bzero(&response[0], sizeof(response));

    do_read(sock, response, 50);
    close(sock);
    if (response[0] == '+') {
        return true;
    } else {
        return false;
    }
}


vector<bool> getStatus(vector<struct sockaddr_in> addrs) {
    printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n");
    vector<bool> result;
    for (int i = 0; i < addrs.size(); i++) {

        
        int sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            cerr << "Error opening a socket (frontend internal)." << endl;
            exit(1);
        }
        // Connect to the given storage
        if(connect(sock, (struct sockaddr*)&addrs[i], sizeof(addrs[i])) != 0){
            result.push_back(false);
            continue;
        }
        printf("hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh\n");
        str_write(sock, "ALIVE`\n");

        //wait for ACK with timeout (1sec)
        // fd_set rfds;
        // struct timeval tv;
        // int retval;

        // /* Wait up to 1 second. */
        // FD_ZERO(&rfds);
        // FD_SET(sock, &rfds);
        // tv.tv_sec = 1;
        // tv.tv_usec = 0;
        // retval = select(1, &rfds, NULL, NULL, &tv);
        // /* Don't rely on the value of tv now! */
        // printf("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb\n");
        // if (retval == -1) {
        //     cerr << "getStauts select() failed!" << endl;
        //     exit(1);
        // // }
        // else if (retval) {  // there is data to read
        char response[21];
        bzero(&response[0], sizeof(response));
        do_read(sock, response, 20);
        printf("response is %s\n", response);
        result.push_back(true);
        close(sock);

    }
    return result;
}

string getRaw(struct sockaddr_in backendAddr){
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        cerr << "Error opening a socket (frontend internal)." << endl;
        exit(1);
    }
    // Connect to the given storage
    connect(sock, (struct sockaddr*)&backendAddr, sizeof(backendAddr));
    printf("After connect\n");
    str_write(sock, "RAW`\n");
    //wait for ACK
    char data[BUF_SIZE];
    bzero(&data[0], sizeof(data));

    printf("before do read\n");
    do_read(sock, data, BUF_SIZE-1);
    close(sock);
    printf("socket closeds\n");
    
    return string(data);

}




// PUT to same file will be appended
// format：PUT,r,c,size,v
// r: username
// c: filename / @ / *pwd
// TODO: deal with large file
bool sendPut(int sock, string r, string c, string v) {
    string msg("PUT`");
    msg += r;
    msg += '`';
    msg += c;
    msg += '`';
    msg += v;
    msg += "`\n";     // isolation char for backend
    if (debugMode) cerr << msg << endl;
    return str_write(sock, msg);
}

bool sendGet(int sock, string r, string c) {
    string msg("GET`");
    msg += r;
    msg += '`';
    msg += c;
    msg += "`\n";     // isolation char for backend
    if (debugMode) cerr << msg << endl;
    return str_write(sock, msg);
}

// format：CPUT,r,c,v1,v2
bool sendCput(int sock, string r, string c, string v1, string v2) {
    string msg("CPUT`");
    msg += r;
    msg += '`';
    msg += c;
    msg += '`';
    msg += v1;
    msg += '`';
    msg += v2;
    msg += "`\n";     // isolation char for backend
    if (debugMode) cerr << msg << endl;
    return str_write(sock, msg);
}

bool sendDelete(int sock, string r, string c) {
    string msg("DELETE`");
    msg += r;
    msg += '`';
    msg += c;
    msg += "`\n";     // isolation char for backend
    if (debugMode) cerr << msg << endl;
    return str_write(sock, msg);
}

bool sendMove(int sock, string r, string oldpath, string newpath) {
    string msg("MOVE`");
    msg += r;
    msg += '`';
    msg += oldpath;
    msg += '`';
    msg += newpath;
    msg += "`\n";     // isolation char for backend
    if (debugMode) cerr << msg << endl;
    return str_write(sock, msg);
}

bool sendRename(int sock, string r, string oldname, string newname) {
    string msg("RENAME`");
    msg += r;
    msg += '`';
    msg += oldname;
    msg += '`';
    msg += newname;
    msg += "`\n";     // isolation char for backend
    if (debugMode) cerr << msg << endl;
    return str_write(sock, msg);
}



bool str_write(int fd, string str) {
    int len = str.length();
    char* buf = strdup(str.c_str());
    int sent = 0;
    while (sent < len) {
        int n = write(fd, &buf[sent], len - sent);
        if (n < 0)
            return false;
        sent += n;
    }
    return true;
}

bool do_read(int fd, char *buf, int len) {
    int rcvd = 0;
    while (rcvd < len) {
        int n = read(fd, &buf[rcvd], len - rcvd);
        if (n < 0)
            return false;
        if (strchr(buf, '\0'))
            return true;
        rcvd += n;
    }
    return true;
}

bool do_write(int fd, char *buf, int len) {
    int sent = 0;
    while (sent < len) {
        int n = write(fd, &buf[sent], len - sent);
        if (n < 0)
            return false;
        sent += n;
    }
    return true;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>

using namespace std;

// render mail page

string renderPage(string pagename) {
   ifstream ifs(pagename);

   if (!ifs.is_open()) {
      cerr << "Error: Cannot open " << pagename << "\n";
      exit(1);
   }

   stringstream buffer;
   buffer << ifs.rdbuf();
   ifs.close();
   return buffer.str();
}

// helper: replace all ocurrence of a substring
string ReplaceAll(string str, const string& from, const string& to) {
   size_t start_pos = 0;
   while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
   }
   return str;
}

// filename format: d>$folder1>#file1
// currentPath: 
// filenames: files and folders under current path
// folderPaths: all available paths for moving
// hasError: error msg to display for the case a input folder/filename contains special char
string renderDrive(string currentPath, vector<string> allNames, vector<string> folderPaths, bool hasError = false) {
   string page("<!DOCTYPE html><html><head>");
   page += "<meta charset=\"utf-8\" />";
   page += "<title>My Drive - PennCloud</title></head>";
   page += "<body><h1>PennCloud Drive</h1>";

   // Add errorMsg
   if (hasError){
      page += "<font color=\"Red\">Error: The name can only contain letters, numbers, ‘-’ and ‘_’. Please try again.</font><br><br>";
   }
   page += "<b> Current path: drive";

   // d>$folder1>$folder2>#file1 => /folder1/folder2
   // string path = allNames[0].substr(1);              // >$folder1>$folder2>#file1
   // path = path.substr(0, path.find_last_of('>'));     // >$folder1>$folder2
   // path = ReplaceAll(path, string(">$"), string("/"));
   // format: /folder1/folder2

   // format currentPath
   currentPath = currentPath.substr(1);
   currentPath = ReplaceAll(currentPath, string(">$"), string("/"));
   if (currentPath.back() == '>') {
      currentPath.back() = '/';
   } else {
      currentPath += '/';
   }
   page += currentPath;

   page += "</b><br><br>";


   // Separate files and folders
   vector<string> files;
   vector<string> folders;
   for (int i = 0; i < allNames.size(); i++) {
      if (allNames[i].find_last_of('#') != string::npos) {
         files.push_back(allNames[i]);
      } else {
         folders.push_back(allNames[i]);
      }
   }

   // render folders
   page += "<b>Folders:</b><br><table cellspacing=\"12\"><tr><th>Name</th><th>Delete</th></tr>";
   for (int i = 0; i < folders.size(); i++) {
      page += "<tr><td><form action=\"/openfolder\" method=\"post\"><button name=\"";
      page += folders[i];
      page += "\">";

      //page += "<img src=\"folder-icon.png\" alt=\"folder\" style=\"width:20px;height:20px;\">";
      page += folders[i].substr(folders[i].find_last_of("$") + 1);    // Get short folder name, omit $
      page += "</button></form></td>";

      page += "<td><form action=\"/delete\" method=\"post\"><button name=\"delete\" value=\"";
      page += folders[i];
      page += "\">Delete</button></form></td></tr>";
   }
   page += "</table><br>";


   // get paths for selection tag
   string pathOptions = "";
   for (int i = 0; i < folderPaths.size(); i++) {
      pathOptions += "<option value=\"";
      pathOptions += folderPaths[i];
      pathOptions += "\">";

      // d>$f0>$f1 => drive/f0/f1/
      // d> => drive/
      string pathToShow = "drive";
      pathToShow += string(folderPaths[i]).substr(1);    // copy and remove 'd'

      pathToShow = ReplaceAll(pathToShow, string(">$"), string("/"));

      if (pathToShow.back() == '>') {
         pathToShow.back() = '/';
      } else {
         pathToShow += '/';
      }

      pathOptions += pathToShow;
      pathOptions += "</option>";
   }

   // render files
   page += "<b>Files:</b><br>";
   page += "<table cellspacing=\"12\"><tr><th>Name</th><th>Rename</th><th>Move to</th><th>Delete</th></tr>";
   for (int i = 0; i < files.size(); i++) {
      // File name
      page += "<tr><td><form action=\"/downloadfile\" method=\"post\"><button name=\"";
      page += files[i];
      page += "\">";

      //page += "<img src=\"file-icon.png\" alt=\"file\" style=\"width:20px;height:20px;\">";
      page += files[i].substr(files[i].find_last_of('#') + 1);
      page += "</button></form></td>";

      // Rename
      page += "<td><form action=\"/rename\" method=\"post\"><input type=\"text\" name=\"";
      page += files[i];
      page += "\" placeholder=\"Enter new name\" required><input type=\"submit\" value=\"Rename\"></form></td>";

      // Move to
      page += "<td><form action=\"/move\" method=\"post\"><select name=\"";
      page += files[i];
      page += "\">";
      page += pathOptions;
      page += "</select><input type=\"submit\" value=\"Move\"></form></td>";

      // Delete
      page += "<td><form action=\"/delete\" method=\"post\"><button name=\"delete\" value=\"";
      page += files[i];
      page += "\">Delete</button></form></td>";

      page += "</tr>";
   }
   page += "</table>";

   string page4;
   page += "<br><table><tr><td><b>Add a new file:</b></td><td></td></tr>";
   page += "<tr><td><form name=\"input\" action=\"/newfile\" method=\"post\" ENCTYPE=\"multipart/form-data\">";
   page += "<input type=\"file\" name=\"newfile\" required></td>";
   page += "<td><input type=\"submit\" value=\"Upload\"></form></td></tr>";

   page += "<tr><td><b>Add a new folder:</b></td><td></td></tr>";
   page += "<tr><td><form name=\"input\" action=\"/newfolder\" method=\"post\">";
   page += "<input type=\"text\" name=\"foldername\" placeholder=\"Folder Name\" size=\"20\" required></td>";
   page += "<td><input type=\"submit\" value=\"Submit\">";
   page += "</form></td></tr></table><br>";

   // page += "<a href=\"/emails\">Mailbox</a></br>";
   page += "<a href=\"/html/home.html\">Homepage</a>";
   page += "</body></html>";

   return page;
}



string renderAdmin(vector<struct sockaddr_in> frontend_addrs, vector<bool> frontend_status, vector<struct sockaddr_in> backend_addrs, vector<bool> backend_status) {
   
   string page("<!DOCTYPE html><html><head><meta charset=\"utf-8\" /><title>PennCloud - Admin</title></head>");
   page += "<body><h1>Admin Console</h1>";

   page += "<b>Frontend Servers</b><br>";
   page += "<table><tr align=\"left\"><th>Address</th><th>Status</th></tr>";
   for (int i=0; i<frontend_addrs.size(); i++){
      page += "<tr><td>";
      
      // get id and port
      char ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &(frontend_addrs[i].sin_addr), ip, INET_ADDRSTRLEN);
      string recvIP(ip);
      int recvPort = ntohs(frontend_addrs[i].sin_port);

      page += recvIP;
      page += ':';
      page += to_string(recvPort);
      page += "</td><td>";
      // status
      if(frontend_status[i]){
         page += "Alive";
      } else {
         page += "Down";
      }
      page += "</td></tr>";
   }
   page += "</table><br>";


   page += "<b>Backend Servers</b><br>";
   page += "<table><tr align=\"left\"><th>Address</th><th>Status</th></tr>";
   for (int i=0; i<backend_addrs.size(); i++){
         // get id and port
         char ip[INET_ADDRSTRLEN];
         inet_ntop(AF_INET, &(backend_addrs[i].sin_addr), ip, INET_ADDRSTRLEN);
         string recvIP(ip);
         int recvPort = ntohs(backend_addrs[i].sin_port);
      if(i!= 0 && backend_status[i]){
         page += "<tr><td><form action=\"/checkBS\" method=\"post\"><button name=\"";
         page += to_string(i);
         page += "\">";

         page += recvIP;
         page += ':';
         page += to_string(recvPort);
         page += "</button></form></td><td>";
      } else {
         page += "<tr><td>";
         page += recvIP;
         page += ':';
         page += to_string(recvPort);
         page += "</td><td>";
      }

      // status
      if(backend_status[i]){
         page += "Alive";
      } else {
         page += "Down";
      }
      page += "</td></tr>";
   }
   page += "</table><br>";


   page += "<a href=\"/logout\">Logout</a>";
   page += "</body></html>";

   return page;

}

//For testing
// int main () {
//    //cout << renderPage("html/home.html") << endl;
//    vector<string> items;
//    items.push_back("d>$f00>$f0>#file1");
//    items.push_back("d>$f00>$f0>#file2");
//    items.push_back("d>$f00>$f0>#file3");
//    items.push_back("d>$f00>$f0>$f1");
//    items.push_back("d>$f00>$f0>$f2");
//    items.push_back("d>$f00>$f0>$f3");

//    vector<string> folderPaths;
//    folderPaths.push_back("d>");
//    folderPaths.push_back("d>$f00");
//    folderPaths.push_back("d>$f00>$f0>$f1");
//    folderPaths.push_back("d>$f00>$f0>$f2");
//    folderPaths.push_back("d>$f00>$f0>$f3");
//    folderPaths.push_back("d>$f00>$fa");
//    folderPaths.push_back("d>$fb");
//    folderPaths.push_back("d>$fb>$fc");

//    string currentPath = "d>$f00>$f0";

//    // testing admin
//    // Fill in forwardAddrs
//    vector<struct sockaddr_in> addrs(2);
//    string ip = "127.0.0.1";
//    int ports[] = {8000, 8001};

//    for (int i = 0; i < addrs.size(); i++) {
//       bzero(&addrs[i], sizeof(addrs[i]));
//       addrs[i].sin_family = AF_INET;
//       addrs[i].sin_port = htons(ports[i]);
//       inet_pton(AF_INET, ip.c_str(), &(addrs[i].sin_addr));
//    }
//    vector<bool> status(2);
//    status[0] = false;
//    status[1] = true;



//    ofstream ofs;


//    ofs.open("html/admin_test.html");
//    //ofs << renderDrive(currentPath, items, folderPaths, true);
//    ofs << renderAdmin(addrs,status,addrs,status);
//    ofs.close();
//    exit(0);
// }


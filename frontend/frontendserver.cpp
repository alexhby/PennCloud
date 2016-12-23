#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <signal.h>
#include <time.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <unordered_map>

using namespace std;

int t_num = 0;
bool v = false;
int all_threads[100];
void INThandler(int);

string mailserver(std::string username, std::string message, struct sockaddr_in serverSock);

string renderPage(string pagename);
string ReplaceAll(string str, const string& from, const string& to);
string renderDrive(string currentPath, vector<string> allNames, vector<string> folderPaths, bool hasError = false);
string renderAdmin(vector<struct sockaddr_in> frontend_addrs, vector<bool> frontend_status, vector<struct sockaddr_in> backend_addrs, vector<bool> backend_status);

vector<struct sockaddr_in> getAddrs (string address_file);

vector<struct sockaddr_in>  backendAddrs = getAddrs("backendservers.txt");
vector<struct sockaddr_in> frontendAddrs = getAddrs("frontendservers.txt");
unordered_map<string,string> cur_paths;

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

/* Dont need it now
// translate a name to a full path
// path: path of another file: 'd>$folder1>$folder2>#file2'
// name: name of the file/folder: '#file1' or '$folderA'
// output: 'd>$folder1>$folder2>#file1'
// string nameToFullpath(string path, string name);
*/
// Return files and folders given a current folder path
vector<string> listFiles(struct sockaddr_in backendAddr, string user, string folder);
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



bool echo_read(int fd, char *buf, int buff_ptr){
	int len = 10000;
	while(true){
		int n = read(fd, &buf[buff_ptr], len-buff_ptr);
		if(n<0) return false;
		return false;
	}
	return true;
}

bool echo_write(int fd, char *buf, int len){
	int sent = 0;
	while(sent<len){
		int n = write(fd,&buf[sent],len-sent);
		if(n<0){
			return false;
		}
		return true;
	}
	return true;
}

void  INThandler(int sig)
{
     char  c;

     signal(sig, SIG_IGN);
     printf("Do you want to shut down the server? [y/n] ");
     c = getchar();
     if (c == 'y' || c == 'Y'){
     	for(int i=0; i<t_num;i++){
			char shutdown[50] = "-ERR Server shutting down\n";
			echo_write(all_threads[i],shutdown, strlen(shutdown));
			close(all_threads[i]);   		
     	}
        exit(0);
     }
     else{
	     signal(SIGINT,INThandler);
	     getchar();
     }
}

void render_dynamic_page(string page, int comm_fd){
	char HTTP_OK[30];
	memset(HTTP_OK,0,30);
	strcat(HTTP_OK,"HTTP/1.1");
	strcat(HTTP_OK," 200 ");
	strcat(HTTP_OK,"OK\n");

	char content_length[30] = "content-length: ";
	char content_type[30] = "content-type: text/html\n";
	const char* page_chr = page.c_str();
	int file_len = strlen(page_chr);
	char file_len_char[7];
	memset(file_len_char,0,7);
	sprintf(file_len_char,"%d",file_len);
	strcat(content_length,file_len_char);
	strcat(content_length,"\n\n");

	string response = string(HTTP_OK)+string(content_type)+string(content_length)+page;
	echo_write(comm_fd,(char*)response.c_str(),strlen(response.c_str()));

}

void render_dynamic_page_cookie(string page, int comm_fd, string cookie){
	char HTTP_OK[30];
	memset(HTTP_OK,0,30);
	strcat(HTTP_OK,"HTTP/1.1");
	strcat(HTTP_OK," 200 ");
	strcat(HTTP_OK,"OK\n");

	char content_length[30] = "content-length: ";
	char content_type[30] = "content-type: text/html\n";
	const char* page_chr = page.c_str();
	int file_len = strlen(page_chr);
	char file_len_char[7];
	memset(file_len_char,0,7);
	sprintf(file_len_char,"%d",file_len);
	strcat(content_length,file_len_char);
	strcat(content_length,"\n\n");

	string response = string(HTTP_OK)+cookie+string(content_type)+string(content_length)+page;
	echo_write(comm_fd,(char*)response.c_str(),strlen(response.c_str()));
}

void render_static_page(string pagename,int comm_fd){
	string page_str = renderPage(pagename);
	char HTTP_OK[30];
	memset(HTTP_OK,0,30);
	strcat(HTTP_OK,"HTTP/1.1");
	strcat(HTTP_OK," 200 ");
	strcat(HTTP_OK,"OK\n");

	char content_length[30] = "content-length: ";
	char content_type[30] = "content-type: text/html\n";
	const char* page_chr = page_str.c_str();
	int file_len = strlen(page_chr);
	char file_len_char[7];
	memset(file_len_char,0,7);
	sprintf(file_len_char,"%d",file_len);
	strcat(content_length,file_len_char);
	strcat(content_length,"\n\n");

	string response = string(HTTP_OK)+string(content_type)+string(content_length)+page_str;
	echo_write(comm_fd,(char*)response.c_str(),strlen(response.c_str()));

}

void render_static_page_cookie(string pagename,int comm_fd, string cookie){
	string page_str = renderPage(pagename);
	char HTTP_OK[30];
	memset(HTTP_OK,0,30);
	strcat(HTTP_OK,"HTTP/1.1");
	strcat(HTTP_OK," 200 ");
	strcat(HTTP_OK,"OK\n");

	char content_length[30] = "content-length: ";
	char content_type[30] = "content-type: text/html\n";
	const char* page_chr = page_str.c_str();
	int file_len = strlen(page_chr);
	char file_len_char[7];
	memset(file_len_char,0,7);
	sprintf(file_len_char,"%d",file_len);
	strcat(content_length,file_len_char);
	strcat(content_length,"\n\n");

	const char* set_cookie = cookie.c_str();

	string response = string(HTTP_OK)+cookie+string(content_type)+string(content_length)+page_str;
	echo_write(comm_fd,(char*)response.c_str(),strlen(response.c_str()));

}


void *worker(void *arg){
	int comm_fd = *(int*)arg;
	// echo_write(comm_fd,welcome, strlen(welcome));
	int cur = t_num;
	char *buf = new char[10198];
	bzero(buf,sizeof(buf));
	int buff_ptr = 0;
	char error_no_url[30] = "Please enter a url\n";
	char error_no_HTTPv[30] = "Please enter an HTTP version\n";
	char remaining_GET_msg[500];
	memset(remaining_GET_msg,0,500);
	char POST_header[1000];
	bzero(POST_header,sizeof(POST_header));
	memset(POST_header,0,1000);
	char url[50];
	memset(url,0,50);
	char HTTPv[20];
	memset(HTTPv,0,20);
	char postaction[40];
	memset(postaction,0,40);
	string cur_path = "d";

	int serverID;
	string cur_user;

	bool getcommand = false;
	bool postcommand = false;
	bool postbody = false;
	bool head = false;
	bool logout = true;

	while(true){
		signal(SIGINT,INThandler);
		echo_read(comm_fd,buf, buff_ptr);
		while(strchr(buf,'\n')){
			buff_ptr += strlen(buf);
			int cur_len = 0;
			for(int i = 0; i<strlen(buf);i++){
				cur_len++;
				if(buf[i]=='\n'){
					char* copy = new char[10198]; // todo
					bzero(copy,sizeof(copy));
					strncpy(copy,buf,cur_len);
					// printf("This is copy %s\n", copy);
					if(v==true){
						fprintf(stderr,"[%d] C: %s", cur, copy);
					}
					// printf("%s\n", buf);
					

					char command3[4];
					strncpy(command3,copy,3);
					command3[3] = '\0';

					char command4[5];
					strncpy(command4,copy,4);
					command4[4] = '\0';

					char command6[7];
					strncpy(command6,copy,6);
					command6[6] = '\0';



					if(postcommand == true){
						strcat(POST_header,copy);
						if(strlen(copy)<=2 && string(POST_header).find("th:") != string::npos){
							string post_header = string(POST_header);
							bool hasCookie;
							size_t cookieInd = post_header.find("Cookie:");
							// printf("%s\n", post_header.substr(cookieInd,20).c_str());
							// printf("%s\n", post_header.c_str());
							if(cookieInd == string::npos){
								hasCookie = false;
								logout = true;
							}else{
								logout = false;
								hasCookie = true;
								size_t userInd = post_header.find("user=");
								size_t sidInd = post_header.find("sid=",userInd);
								cur_user = post_header.substr(userInd+5,sidInd-userInd-7);
								serverID = atoi((post_header.substr(sidInd+4,1)).c_str());
							}
							postbody = true;
							char postbodymsg_char[250];
							memset(postbodymsg_char,0,250);
							strcpy(postbodymsg_char,buf);
							string postbodymsg = string(postbodymsg_char).substr(2);
							string posttype;
							string posttype2;
							bool success;
							// printf("cur_user is %s and serverID is %d\n", cur_user.c_str(),serverID);
							// printf("This is post header\n %s\n", post_header.c_str());
							// printf("This is post body\n %s\n", postbodymsg.c_str());
							
							cur_path = cur_paths[cur_user];

							if(strcmp(postaction,"/newfile")==0){
								string postmsg = POST_header+postbodymsg;
								printf("This is post %s\n", postmsg.c_str());
								size_t fstart = postmsg.find("filename=\"")+10;
								size_t fend = postmsg.find("Content-Type",fstart)-2;
								size_t cstart = postmsg.find("Content-Type",fend);
								size_t cmid = postmsg.find("\n",cstart)+2;
								size_t cend = postmsg.find("----------",cmid);
								string filename = cur_path+">#" + postmsg.substr(fstart,fend-fstart-1);
								string filecontent = postmsg.substr(cmid+1,cend-cmid-1);
								success = newfile(backendAddrs[serverID],cur_user,filename,filecontent);
								if(success){
									vector<string> allnames = listFiles(backendAddrs[serverID],cur_user,cur_path);
									vector<string> folderPaths = allPaths(backendAddrs[serverID], cur_user, cur_path);
									string drivepage = renderDrive(cur_path,allnames,folderPaths);
									render_dynamic_page(drivepage,comm_fd);
								}
							}else if(strcmp(postaction,"/newfolder")==0){
								string nfolder = postbodymsg.substr(postbodymsg.find("=")+1);
								string actual_path = cur_path + ">$" + nfolder;
								success = newfolder(backendAddrs[serverID],cur_user,actual_path);
								if(success){
									vector<string> allnames = listFiles(backendAddrs[serverID],cur_user,cur_path);
									vector<string> folderPaths = allPaths(backendAddrs[serverID], cur_user, cur_path);
									string drivepage = renderDrive(cur_path,allnames,folderPaths);
									render_dynamic_page(drivepage,comm_fd);
								}
							}else if(strcmp(postaction,"/openfolder")==0){
								posttype+="openfolder";
								string nfolder = postbodymsg.substr(0,postbodymsg.find("="));
								cur_path = nfolder;
								cur_path = ReplaceAll(cur_path, (string)"%3E", (string)">");
								cur_path = ReplaceAll(cur_path, (string)"%24", (string)"$");
								cur_paths[cur_user] = cur_path;
								vector<string> allnames = listFiles(backendAddrs[serverID],cur_user,cur_path);
								vector<string> folderPaths = allPaths(backendAddrs[serverID], cur_user, cur_path);
								string drivepage = renderDrive(cur_path,allnames,folderPaths);
								render_dynamic_page(drivepage,comm_fd);
							}else if(strcmp(postaction,"/downloadfile")==0){
								string dfile = postbodymsg.substr(0,postbodymsg.find("="));
								dfile = ReplaceAll(dfile, (string)"%3E", (string)">");
								dfile = ReplaceAll(dfile,(string)"%24", (string)"$");
								dfile = ReplaceAll(dfile,(string)"%23", (string)"#");

								string file = getFile(backendAddrs[serverID],cur_user,dfile);
								// string content_type = "Content-Type: application/octet-stream\n";
								// string content_length = "Content-Length: "+to_string(file.length())+"\n\n";
								// string eresponse = content_type+content_length+file;
								echo_write(comm_fd,(char*)file.c_str(),file.length());

								vector<string> allnames = listFiles(backendAddrs[serverID],cur_user,cur_path);
								vector<string> folderPaths = allPaths(backendAddrs[serverID], cur_user, cur_path);
								string drivepage = renderDrive(cur_path,allnames,folderPaths);
								render_dynamic_page(drivepage,comm_fd);
							}else if(strcmp(postaction,"/delete")==0){
								posttype+="delete";
								string val = parseQuery(postbodymsg,posttype);
								val = ReplaceAll(val, (string)"%3E", (string)">");
								val = ReplaceAll(val,(string)"%24", (string)"$");
								val = ReplaceAll(val,(string)"%23", (string)"#");
								val = ReplaceAll(val,(string)"%3D", (string)"=");
								success = deleteByPath(backendAddrs[serverID], cur_user, val);
								if(success){
									vector<string> allnames = listFiles(backendAddrs[serverID],cur_user,cur_path);
									vector<string> folderPaths = allPaths(backendAddrs[serverID], cur_user, cur_path);
									string drivepage = renderDrive(cur_path,allnames,folderPaths);
									render_dynamic_page(drivepage,comm_fd);
								}
							}else if(strcmp(postaction,"/move")==0){
								posttype+="move";
								string oldpath = postbodymsg.substr(0,postbodymsg.find("="));
								string newpath = postbodymsg.substr(postbodymsg.find("=")+1);
								oldpath = ReplaceAll(oldpath, (string)"%3E", (string)">");
								oldpath = ReplaceAll(oldpath,(string)"%24", (string)"$");
								oldpath = ReplaceAll(oldpath,(string)"%23", (string)"#");
								oldpath = ReplaceAll(oldpath,(string)"%3D", (string)"=");
								newpath = ReplaceAll(newpath, (string)"%3E", (string)">");
								newpath = ReplaceAll(newpath,(string)"%24", (string)"$");
								newpath = ReplaceAll(newpath,(string)"%23", (string)"#");
								newpath = ReplaceAll(newpath,(string)"%3D", (string)"=");
								newpath += ">#"+oldpath.substr(oldpath.find(cur_path+">#")+3);
								success = moveFile(backendAddrs[serverID],cur_user,oldpath,newpath);
								if(success){
									vector<string> allnames = listFiles(backendAddrs[serverID],cur_user,cur_path);
									vector<string> folderPaths = allPaths(backendAddrs[serverID], cur_user, cur_path);
									string drivepage = renderDrive(cur_path,allnames,folderPaths);
									render_dynamic_page(drivepage,comm_fd);
								}
							}else if(strcmp(postaction,"/rename")==0){
								posttype+="rename";
								string oldname = postbodymsg.substr(0,postbodymsg.find("="));
								string newname = "d>#"+postbodymsg.substr(postbodymsg.find("=")+1);
								oldname = ReplaceAll(oldname, (string)"%3E", (string)">");
								oldname = ReplaceAll(oldname,(string)"%24", (string)"$");
								oldname = ReplaceAll(oldname,(string)"%23", (string)"#");
								oldname = ReplaceAll(oldname,(string)"%3D", (string)"=");
								newname = ReplaceAll(newname, (string)"%3E", (string)">");
								newname = ReplaceAll(newname,(string)"%24", (string)"$");
								newname = ReplaceAll(newname,(string)"%23", (string)"#");
								newname = ReplaceAll(newname,(string)"%3D", (string)"=");
								success = renameFile(backendAddrs[serverID],cur_user,oldname,newname);
								if(success){
									vector<string> allnames = listFiles(backendAddrs[serverID],cur_user,cur_path);
									vector<string> folderPaths = allPaths(backendAddrs[serverID], cur_user, cur_path);
									string drivepage = renderDrive(cur_path,allnames,folderPaths);
									render_dynamic_page(drivepage,comm_fd);
								}
							}else if(strcmp(postaction,"/checklogin")==0){
								pair<string,string> val = parseQuery(postbodymsg,"user","pass");
								if(val.first == "admin" && val.second == "admin"){
									printf("1111111111111111111111111111\n");
									vector<bool> frontstatus = getStatus(frontendAddrs);
									vector<bool> backstatus = getStatus(backendAddrs);
									printf("2222222222222222222222222222222222\n");
									string adminpage = renderAdmin(frontendAddrs,frontstatus,backendAddrs,backstatus);
									printf("33333333333333333333333333333333333333333333\n");
									render_dynamic_page(adminpage,comm_fd);
								}else{

									if(checkName(val.first)){
										serverID = checkLogin(backendAddrs,val.first,val.second);
										if(serverID==-1){
											//invalid login
											render_static_page("html/login_failed.html",comm_fd);
										}else{
											cur_user = val.first;
											string cookie = "Set-Cookie: user=";
											cookie+=val.first;
											cookie+=";";
											cookie+="\nSet-Cookie: sid=";
											cookie+=to_string(serverID);
											cookie+=";\n";
											logout = false;
											render_static_page_cookie("html/home.html",comm_fd,cookie);
										}
									}else{
										//invalid user name
										render_static_page("html/login_failed.html",comm_fd);
									}
								}
							}else if(strcmp(postaction,"/newuser")==0){
								posttype+="user";
								posttype2+="pass";
								pair<string, string> val = parseQuery(postbodymsg,posttype,posttype2);
								// printf("Before chekname\n");
								if(checkName(val.first)){
									// printf("check name success\n");
									int checkExist = checkLogin(backendAddrs,val.first,val.second);
									if(checkExist	 == -1){
										// printf("no existing user\n");
										printf("username %s\n", val.first.c_str());
										printf("password %s\n", val.second.c_str());
										serverID = createNewUser(backendAddrs,val.first,val.second);
										printf("this is serverID %d\n", serverID);
										// printf("got serverID\n");
										if(serverID==-1){
											render_static_page("html/signup_invalid.html",comm_fd);
										}else{
											cur_user = val.first;
											string cookie = "Set-Cookie: user=";
											cookie+=val.first;
											cookie+=";";
											cookie+="\nSet-Cookie: sid=";
											cookie+=to_string(serverID);
											cookie+=";\n";
											// printf("ready to render\n");
											logout = false;
											cur_paths[cur_user] = "d";
											render_static_page_cookie("html/home.html",comm_fd,cookie);
										}
									}else{
										//user already exists
										render_static_page("html/signup_user_exists.html",comm_fd);
									}
								}else{
									//invalide username
									render_static_page("html/signup_invalid.html",comm_fd);
								}
							}else if(strcmp(postaction,"/changepass")==0){
								posttype+="oldpass";
								posttype2+="newpass";
								pair<string,string> val = parseQuery(postbodymsg,posttype,posttype2);
								success = changePassword(backendAddrs[serverID],cur_user,val.first,val.second);
								if(success){
									render_static_page("html/home.html",comm_fd);
								}else{
									render_static_page("html/settings_failed.html",comm_fd);
								}
							}else if(strcmp(postaction,"/emails")==0){
								string emailpage = mailserver(cur_user, "/listpage", backendAddrs[serverID]);
								render_dynamic_page(emailpage,comm_fd);
							}else if(strcmp(postaction,"/drive")==0){
								cur_path = "d";
								cur_paths[cur_user] = 'd';
								printf("in drive action\n");
								vector<string> allnames = listFiles(backendAddrs[serverID],cur_user,cur_path);
								printf("before folderPaths\n");
								vector<string> folderPaths = allPaths(backendAddrs[serverID], cur_user, cur_path);
								printf("Before drivepage\n");
								string drivepage = renderDrive(cur_path,allnames,folderPaths);
								printf("before rendering\n");
								render_dynamic_page(drivepage,comm_fd);
							}else if(strcmp(postaction,"/logout")==0){
								// string cookie = "Set-Cookie: user=-1; Expires=Thu, 01 Jan 1970 00:00:01 GMT;\nSet-Cookie: sid=-1; Expires=Thu, 01 Jan 1970 00:00:01 GMT;\n";
								// logout = true;
								// render_static_page_cookie("html/login.html",comm_fd,cookie);
							}else if(strcmp(postaction,"/listpage")==0){
								string emailaction = string(postaction) + "&" + postbodymsg;
								string eresponse = ReplaceAll(emailaction, (string)"+", (string)" ");
								string emailpage = mailserver(cur_user, eresponse, backendAddrs[serverID]);
								render_dynamic_page(emailpage,comm_fd);
							}else if(strcmp(postaction,"/checkpage")==0){
								string emailaction = string(postaction) + "&" + postbodymsg;
								string eresponse = ReplaceAll(emailaction, (string)"+", (string)" ");
								string emailpage = mailserver(cur_user, eresponse, backendAddrs[serverID]);
								emailpage = ReplaceAll(emailpage, (string)"%0D%0A", (string)"\r\n");
								render_dynamic_page(emailpage,comm_fd);
							}else if(strcmp(postaction,"/replypage")==0){
								string emailaction = string(postaction) + "&" + postbodymsg;
								string eresponse = ReplaceAll(emailaction, (string)"+", (string)" ");
								string emailpage = mailserver(cur_user, eresponse, backendAddrs[serverID]);
								render_dynamic_page(emailpage,comm_fd);
							}else if(strcmp(postaction,"/forwardpage")==0){
								string emailaction = string(postaction) + "&" + postbodymsg;
								string eresponse = ReplaceAll(emailaction, (string)"+", (string)" ");
								string emailpage = mailserver(cur_user, eresponse, backendAddrs[serverID]);
								render_dynamic_page(emailpage,comm_fd);
							}else if(strcmp(postaction,"/deletepage")==0){
								string emailaction = string(postaction) + "&" + postbodymsg;
								string eresponse = ReplaceAll(emailaction, (string)"+", (string)" ");
								string emailpage = mailserver(cur_user, eresponse, backendAddrs[serverID]);
								render_dynamic_page(emailpage,comm_fd);
							}else if(strcmp(postaction,"/composepage")==0){
								string emailpage = mailserver(cur_user, postaction, backendAddrs[serverID]);
								render_dynamic_page(emailpage,comm_fd);
							}else if(strcmp(postaction,"/composesuccess")==0){
								string emailaction = string(postaction) + "&" + postbodymsg;
								string eresponse = ReplaceAll(emailaction, (string)"+", (string)" ");
								string emailpage = mailserver(cur_user, eresponse, backendAddrs[serverID]);
								render_dynamic_page(emailpage,comm_fd);
							}else if(strcmp(postaction,"/checkBS")==0){
								string serverindex = postbodymsg.substr(0,1);
								printf("this is server %s\n", serverindex.c_str());
								string res = getRaw(backendAddrs[stoi(serverindex)]);
								printf("this is res %s\n", res.c_str());
								echo_write(comm_fd,(char*)res.c_str(),res.length());
							}


							postcommand = false;
							close(comm_fd);
						}
					}else if(getcommand == true){
						if(strlen(buf)<=2){
							char HTTP_OK[30];
							memset(HTTP_OK,0,30);
							strcat(HTTP_OK,HTTPv);
							strcat(HTTP_OK," 200 ");
							strcat(HTTP_OK,"OK\n");

							char content_length[30] = "content-length: ";

							// Render page
							string htmlname = string(url).substr(1);
							string getRes_str;
							// TODO cookies
			
							char content_type[30];
							memset(content_type,0,30);
							if (htmlname.find(".jpeg") != std::string::npos) {
							    strcat(content_type,"content-type: image/jpeg\n");
							 //    getcommand = false;
								// close(comm_fd);
							}else if(htmlname.find(".png") != std::string::npos){
								strcat(content_type,"content-type: image/png\n");
								// getcommand = false;
								// close(comm_fd);
							}else{
								strcat(content_type,"content-type: text/html\n");
								getRes_str = renderPage(htmlname);
							}


							
							const char* getRes_char = getRes_str.c_str();
							strcat((char*)getRes_char,"\n");

							int file_len = strlen(getRes_char);

							char file_len_char[7];
							memset(file_len_char,0,7);
							sprintf(file_len_char,"%d",file_len);
							strcat(content_length,file_len_char);
							strcat(content_length,"\n\n");

							echo_write(comm_fd,HTTP_OK,strlen(HTTP_OK));
							echo_write(comm_fd,content_type,strlen(content_type));
							echo_write(comm_fd,content_length,strlen(content_length));
							if(head == false){
								echo_write(comm_fd,(char*)getRes_char,file_len); 
							}
							
							getcommand = false;
							close(comm_fd);
						}else{
							strcat(remaining_GET_msg,copy);
						}
					}else if(strcmp(command3,"GET\0")==0){
						// if(logout == true){
						// 	render_static_page("html/login.html",comm_fd);
						// 	postcommand = false;
						// 	close(comm_fd);
						// }
						char *token;
						token = strtok(copy, " ");
						token = strtok(NULL, " ");
						if(token==NULL){
							echo_write(comm_fd,error_no_url,strlen(error_no_url));
						}else{
							memset(url,0,50);
							strcat(url,token);
						}
						token = strtok(NULL, " ");
						if(token==NULL){
							echo_write(comm_fd,error_no_HTTPv,strlen(error_no_HTTPv));
						}else{
							memset(HTTPv,0,20);
							strncat(HTTPv,token,8);
						}
						getcommand = true;
					}else if(strcmp(command3,"PUT\0")==0){

					}else if(strcmp(command4,"CPUT\0")==0){

					}else if(strcmp(command4,"POST\0")==0){
						char *token;
						token = strtok(copy, " ");
						token = strtok(NULL, " ");
						memset(postaction,0,30);
						strcat(postaction,token);
		
						postcommand = true;
					}else if(strcmp(command4,"HEAD\0")==0){
						char *token;
						token = strtok(copy, " ");
						token = strtok(NULL, " ");
						if(token==NULL){
							echo_write(comm_fd,error_no_url,strlen(error_no_url));
						}else{
							memset(url,0,50);
							strcat(url,token);
						}
						token = strtok(NULL, " ");
						if(token==NULL){
							echo_write(comm_fd,error_no_HTTPv,strlen(error_no_HTTPv));
						}else{
							memset(HTTPv,0,20);
							strncat(HTTPv,token,8);
						}
						getcommand = true;
						head = true;
					}else if(strcmp(command6,"DELETE\0")==0){

					}else{
						if(v==true){
							fprintf(stderr, "[%d] %s\n", cur, "400 Bad Request");
						}
						char error[30] = "400 Bad Request\n";
						echo_write(comm_fd,error, strlen(error));
					}
					memset(buf, 0, cur_len);
					memset(copy,0,strlen(copy));
					buf += (cur_len);
					cur_len = 0;
				}
			}
		}
		if(strlen(buf)>1){
			buff_ptr = strlen(buf);
		}else{
			buff_ptr = 0;
		}
	}
}

int main(int argc, char *argv[])
{
	int port = 0;
	for(int i=0; i<argc; i++){
		if(strcmp(argv[i],"-p")==0){
			port = atoi(argv[i+1]);
		}else if(strcmp(argv[i],"-a")==0){
			fprintf(stderr, "*** Team: T02\n");
		exit(1);
		}else if(strcmp(argv[i],"-v")==0){
			v = true;
		}
	}


	int listen_fd = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in servaddr;
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	printf("port number is %d\n", htons(frontendAddrs[port].sin_port));
	servaddr.sin_port = frontendAddrs[port].sin_port;
	bind(listen_fd,(struct sockaddr*)&servaddr, sizeof(servaddr));

	listen(listen_fd, 10);
	while(true){
		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof(clientaddr);
		int *fd = (int*)malloc(sizeof(int));
		*fd = accept(listen_fd, (struct sockaddr*)&clientaddr, &clientaddrlen);
		t_num++;
		all_threads[t_num-1] = *fd;
		if(v==true){
			fprintf(stderr, "New connection\n");
		}
		pthread_t thread;
		pthread_create(&thread, NULL, worker, fd);
	}



	return 0;
	}

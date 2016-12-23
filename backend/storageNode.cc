#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <signal.h>
#include <map>
#include <vector>
#include <sstream>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <sys/stat.h>
using namespace std;


int getSemiPos(string IP_port){
	for(int i=0; i<IP_port.length(); ++i){
		if(IP_port[i] == ':'){
			return i;
			break;
		}
	}
	return -1;
}

void upperCase(string& str){
	for(int i=0; i<str.length(); ++i){
		if(str[i] >= 'a' && str[i] <= 'z'){
			str[i] = str[i] - 'a' + 'A';
		}
	}
}

bool do_write(int fd, char *buf, int len){
	int sent = 0;
	while (sent < len){
		int n = write(fd, &buf[sent],len-sent);
		if (n<0){
			//fprintf(stderr, "Cannot write the Data\n");
			return false;
		}
		sent += n;
	}
	return true;
}

map<int, bool> threadBook;
vector<int> fd_list;

void stopCatcher(int sig){
	char byeGreet[30] = "-ERR Server shutting down\n";

	for(int i=0; i<fd_list.size(); ++i){
		int fd_temp = fd_list[i];
		if(threadBook[fd_temp]){
			do_write(fd_temp, byeGreet, strlen(byeGreet));
			close(fd_temp);
		}
	}
	exit(0);
}

void writeString(string & str, int comm_fd){
  char * to_write = (char*) str.c_str();
  do_write(comm_fd, to_write, strlen(to_write));
}

struct argClient{
	int fileDes;
	bool hasV;
};

int do_read(int fd, char *buf, int startP){
	int start = startP;

	while (1){
		int n = read(fd, &buf[start], 1001);
		if(n == 0) break;
		if (n<0){
			//fprintf(stderr, "Cannot read the Data\n");
			return 0;
		}
		if(strchr(buf, '\0')) return n;
		//later if sveral sentences, start only adds when not \n \r
	}
	read(fd, &buf[start], 1); //read the last one
	return start - startP;
}
struct lineInfo{
	int type; //1 for mail, 2 for drive, 3 for pwd, 4 for sid
	int beginLine;
	int lineNum;
	bool deleted;
	lineInfo(){
		deleted = 0;
	}
};


bool isSubDirectory(string & str1, string & str2){
	int n = str1.length();
	string sub2 = str2.substr(0, n);
	return (sub2 == str1);
}

bool isNextLevel(string & str1, string & str2){
	if(str2.length() < str1.length()) return false;
	//abc>def     
	int n = str1.size();
	for(int i=n; i<str2.length(); ++i){
		if(str2[i] == '>') return false; 
	}
	return isSubDirectory(str1, str2);

}


bool isFileName(string & filePath){
	for(int i=0; i<filePath.length(); ++i){
		if(filePath[i] == '#') return true;
	}
	return false;
}
map<string, map <string, lineInfo> > local_storage;
// row(user) => (col(file) => begin line NO + line number)
map<string, int> mailLine;
map<string, int> driveLine;
//username(row) => start with "which" line now in those two file

map<string, int> mailDelete;
map<string, int> driveDelete;
int flushB = 0;


void *worker(void *arg){
	//fprintf(stderr, "A new thread open for a new Connection\n");
	argClient* tempPt = (argClient*) arg;
	int comm_fd = tempPt -> fileDes;
	bool hasV = tempPt -> hasV;

	char buf[1010];
	int startP = 0;
	while(true){
		unsigned short textL = do_read(comm_fd, buf, startP);
		if(textL == 0) {
			fprintf(stderr, "THREAD terminates \n");
			close(comm_fd);
			pthread_exit(NULL);	
		}
		textL --;

		char *readBuf;
		string ss = buf;
		string bufS = ss.substr(0, textL+startP+1);

		//No matter what a mess it is, "bufS" is the input string !!!  
		string con;
		stringstream iStream(bufS);

		int curLength = 0;
		while(getline(iStream, con)){   //=============================without any delimiter
			// so the "con" is for each command

			int texL = con.length();
			curLength += texL;
			curLength ++;
			//cout << "the curLength is : " << curLength << "   and the bufS.length is :: " << bufS.length() << endl;
			if(curLength > bufS.length()){
				break;
			}

			stringstream PPAP;
			PPAP.str(con);
			string cmd;
			getline(PPAP, cmd, '`');
			upperCase(cmd);

			string row;
			string col;
			getline(PPAP, row, '`');
			getline(PPAP, col, '`');

			string ok = "+OK ";
			if(cmd == "PUT"){
				if(hasV) printf("%s\n", "PUT command, storing data");
				string value;
				getline(PPAP, value, '`');

				mkdir(row.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);// create the folder !!!!!!!!!!!!!!
				
				
				flushB ++;
				
				if(flushB % 100 == 0){
					string backup = "backup.txt";
					ofstream out;
					out.open(backup.c_str());
					for(auto it = local_storage.begin(); it != local_storage.end(); ++it){
						out << (it -> first) << ":";
						for(auto it2 = (it -> second).begin(); it2 != (it -> second).end(); ++it2){
							out << (it2 -> first) << "`";
							out << (it2 -> second).type << "`" << (it2 -> second).beginLine << "`" << (it2 -> second).lineNum<< "`" << (it2 -> second).deleted;
						}
						out << "\n";
					}
				}
				if(col == "sid" || col == "pwd"){
				/* ================== write in auth file========================= */
					string filename = "";
					if(col == "sid") filename = row + "/" + "sid.txt";
					else filename = row + "/" + "pwd.txt";
					ofstream out;
					out.open(filename.c_str()); //does not need to append
					
					out << value;

					lineInfo temp;
					temp.type = (col == "pwd") ? 3 : 4;
					temp.deleted = 0;
					temp.beginLine = 0;
					temp.lineNum = 1;
					local_storage[row][col] = temp;

					out.close();
				}
				//else if(col.find('.') != string::npos){
				else if(col[0] == 'd'){
				/* =================== this is a normal file, write to drive =============*/
					string filename = row + "/" + "drive.txt";
					ofstream out;
					out.open(filename.c_str(), ios_base::app);

					stringstream singleFile(value);
					string eachLine = "";
					lineInfo temp;

					//if(local_storage.find(row) == local_storage.end() || local_storage[row].find(col) == local_storage[row].end()){
					//this is used for: if file is very large, might need serveral rounds of readin()
						temp.type = 2;
						temp.beginLine = driveLine[row];
						temp.deleted = 0;
					//}
					int tempNum = 0;

					while(getline(singleFile, eachLine, '\n')){
						driveLine[row] ++;
						tempNum ++;
						out << eachLine << "\n";
					}
					temp.lineNum = tempNum;
					local_storage[row][col] = temp;

					out.close();
				}
				else{
				/* ==================== this is an email, store in the mail.txt ===============*/
					string filename = row + "/" + "mail.txt";
					ofstream out;
					out.open(filename.c_str(), ios_base::app);

					stringstream singleFile(value);
					string eachLine = "";

					lineInfo temp;

					//if(local_storage.find(row) == local_storage.end() || local_storage[row].find(col) == local_storage[row].end()){
					//this is used for: if file is very large, might need serveral rounds of readin()
						temp.type = 1;
						temp.beginLine = mailLine[row];
						temp.deleted = 0;
					//}
					int tempNum = 0;

					while(getline(singleFile, eachLine, '\n')){
						mailLine[row] ++;
						tempNum ++;
						out << eachLine << "\n";
					}
					temp.lineNum = tempNum;

					local_storage[row][col] = temp;

					out.close();
				}
				string putSuccess = "+OK PUT success\n";
				writeString(putSuccess, comm_fd);
			}
			else if(cmd == "GET"){
				//col = col.substr(0, col.length() - 1);
				/*
				lineInfo tt = local_storage[row][col];
				cout << tt.type << endl;
				cout << tt.lineNum << endl;
				cout << tt.beginLine << endl;
				*/
				if(isFileName(col) || col[0] == 'e' || col == "pwd" || col == "sid"){
					if(hasV) printf("%s\n", "GET command, get filename, fetching data");

					if(local_storage.find(row) != local_storage.end() && local_storage[row].find(col) != local_storage[row].end() && local_storage[row][col].deleted == 0){
						int type = local_storage[row][col].type;
						int beginLine = local_storage[row][col].beginLine;
						int lineNum = local_storage[row][col].lineNum;
						//open the file and ready to read
						string filename = row + "/" + ((type == 1) ? "mail.txt" : ((type == 2) ? "drive.txt" : ((type == 3) ? "pwd.txt" : "sid.txt")));
						ifstream iFile;
						iFile.open(filename.c_str());
						//skip until the beginline and read lineNum, append to a string
						string cont = "";
						for(int i=0; i<beginLine; ++i){
							getline(iFile, cont);
						}
						string ret = "";
						for(int i=0; i<lineNum; ++i){
							getline(iFile, cont);
							ret += cont;
						}

						//ret += "\n";   //shoud be deleted when really send the message!!!!!!!!!!!!!!
						//send the string to the client
						writeString(ret, comm_fd);
					}
					else{
						char notFound[40] = "-ERR resource not found\n";
						do_write(comm_fd, notFound, strlen(notFound));

					}
				}
				else{
					if(local_storage.find(row) != local_storage.end() && local_storage[row].find(col) != local_storage[row].end() && local_storage[row][col].deleted == 0){
						if(hasV) printf("%s\n", "GET command, get folder, fetching subfiles");
						string listFile = "";
						string recPath = col + ">";
						for(map<string, lineInfo>::iterator it = local_storage[row].begin(); it != local_storage[row].end(); ++it){
							if((it -> second).deleted == 1 || (it -> second).type == 1) continue;

							string temp = it -> first;
							if(isNextLevel(recPath, temp)){
								listFile += (temp + "`");
							}
						}
						writeString(listFile, comm_fd);
					}
					else{
						char notFound[40] = "-ERR file folder not found\n";
						do_write(comm_fd, notFound, strlen(notFound));
					}
				}
			}
			else if(cmd == "CPUT"){

				if(hasV) printf("%s\n", "CPUT command");

				string v1;
				getline(PPAP, v1, '`');
				string v2;
				getline(PPAP, v2, '`');

				string curVal = "";
				//get the current value

				if(local_storage.find(row) == local_storage.end() || local_storage[row].find(col) == local_storage[row].end() || local_storage[row][col].deleted == 1){
					//no value in it
					string no_value = "-ERR no value stored in this location\n";
					writeString(no_value, comm_fd);					
				}
				else{
					//get the current value
					int type = local_storage[row][col].type;
					int beginLine = local_storage[row][col].beginLine;
					int lineNum = local_storage[row][col].lineNum;
					//open the file and ready to read
					string filename = row + "/" + ((type == 1) ? "mail.txt" : ((type == 2) ? "drive.txt" : ((type == 3) ? "pwd.txt" : "sid.txt")));
					ifstream iFile;
					iFile.open(filename.c_str());
					//skip un11til the beginline and read lineNum, append to a string
					string cont = "";
					for(int i=0; i<beginLine; ++i){
						getline(iFile, cont);
					}
					string ret = "";
					for(int i=0; i<lineNum; ++i){
						getline(iFile, cont, '\n');
						ret += cont;
					}

					iFile.close();

					if(ret != v1){
						cout << "they are not equal !!" << endl;

						//cout << "original stored : " << ret << " "<< ret.length() << endl;
						//cout << "user input v1  :" << v1 << " " << v1.length() << endl;

						string stored_not= "-ERR stored value is not v1\n";
						writeString(stored_not, comm_fd);
					}
					else{
						//they are equal, do the replace
						//create a file, name_temp.txt
						string file_temp = row + "/" + "temp_" + ((type == 1) ? "mail.txt" : ((type == 2) ? "drive.txt" : ((type == 3) ? "pwd.txt" : "sid.txt")));
						ofstream tempFile;
						tempFile.open(file_temp.c_str());
						//write the file line by line to the new file

						/* former beginLine number of lines copy to the temp file */
						ifstream iSource;
						iSource.open(filename.c_str());

						string temp = "";
						for(int i=0; i<beginLine; ++i){
							getline(iSource, temp, '\n');
							tempFile << temp << "\n"; 
						}

						/* move the pointer (line number) forward lineNum lines after, we do not need those */
						for(int i=0; i<lineNum; ++i){
							getline(iSource, temp);
						}
						/* copy v2 to the temp file */
						stringstream singleFile(v2);
						string eachLine = "";
						int tempNum = 0;

						while(getline(singleFile, eachLine, '\n')){
							tempNum ++;
							tempFile << eachLine << "\n";
						}

						if(type == 1) mailLine[row] = mailLine[row] + (tempNum - local_storage[row][col].lineNum);
						else if(type == 2) driveLine[row] = driveLine[row] + (tempNum - local_storage[row][col].lineNum);

						/* move the remaining files in the source file */
						while(getline(iSource, temp, '\n')){
							tempFile << temp << '\n'; //already have \n in the file
						}
						//delete the old file, rename the new file
						if(type == 1){
							string mail = row + "/mail.txt";
							string tempMail = row + "/temp_mail.txt";
							remove(mail.c_str());
							rename(tempMail.c_str(), mail.c_str());
						} 
						else if(type == 2){
							string drive = row + "/drive.txt";
							string tempMail = row + "/temp_drive.txt";
							remove(drive.c_str());
							rename(tempMail.c_str(), drive.c_str());
						}
						else if(type == 3){
							string pwd = row + "/pwd.txt";
							string tempMail = row + "/temp_pwd.txt";
							remove(pwd.c_str());
							rename(tempMail.c_str(), pwd.c_str());
						}
						else{
							string sid = row + "/drive.txt";
							string tempMail = row + "/temp_sid.txt";
							remove(sid.c_str());
							rename(tempMail.c_str(), sid.c_str());

						}

						string CPUT_success = "+OK CPUT success\n";
						writeString(CPUT_success, comm_fd);
					}

				}
			}
			else if(cmd == "DELETE"){

				if(hasV) printf("%s\n", "DELETE command, deleting data");

				if(local_storage.find(row) == local_storage.end() || local_storage[row].find(col) == local_storage[row].end()){
					//no value in it
					string no_value = "-ERR no value stored in this location\n";
					writeString(no_value, comm_fd);					
				}
				else {
					int type = local_storage[row][col].type;
					int beginLine = local_storage[row][col].beginLine;
					int lineNum = local_storage[row][col].lineNum;

					//cout << "type is : " << type << endl;

					if(type == 1 && mailDelete[row] < 100000){
						cout << "mail delete cache !!!" << endl;
						mailDelete[row] ++;
						local_storage[row][col].deleted = 1;
						string CPUT_success = "+OK DELETE success\n";
						writeString(CPUT_success, comm_fd);
					}
					else if(type == 2 && driveDelete[row] < 100000){
						cout << "drive delete cache !!!" << endl;

						driveDelete[row] ++;
						local_storage[row][col].deleted = 1;

						if(!isFileName(col)){
							for(map<string, lineInfo>::iterator it = local_storage[row].begin(); it != local_storage[row].end(); ++it){
								if((it -> second).deleted || (it -> second).type == 1) continue;

								string temp = it -> first;
								if(isSubDirectory(col, temp)){
									(it -> second).deleted = 1;
								}
							}
						}

						string CPUT_success = "+OK DELETE success\n";
						writeString(CPUT_success, comm_fd);
					}

					else{   
						if(type == 1) mailDelete[row] = 0;
						else if(type == 2) driveDelete[row] = 0;

						unordered_map<int, int> deLine;
						// beginLine in original file ==> number of lines
						vector<string> toDelete;
						unordered_map<int, int> unDet;

						for(map<string, lineInfo>::iterator it = local_storage[row].begin(); it != local_storage[row].end(); ++it){
							if((it -> second).type == type){
								if((it -> second).deleted == 1){
									deLine[(it -> second).beginLine] = (it -> second).lineNum;
									toDelete.push_back(it -> first);
								}
								else{
									//these are the begin line of the undelete files
									unDet[(it -> second).beginLine] = (it -> second).lineNum;
								}
							}
						}

						for(int i=0; i<toDelete.size(); ++i){
							local_storage[row].erase(toDelete[i]);
						}

						cout << "the actual delete !!!!!!!! "<< endl;

						string file_temp = row + "/" + "temp_" + ((type == 1) ? "mail.txt" : ((type == 2) ? "drive.txt" : ((type == 3) ? "pwd.txt" : "sid.txt")));
						ofstream tempFile;
						tempFile.open(file_temp.c_str());
						//write the file line by line to the new file

						/* former beginLine number of lines copy to the temp file */
						string filename = row + "/" + ((type == 1) ? "mail.txt" : ((type == 2) ? "drive.txt" : ((type == 3) ? "pwd.txt" : "sid.txt")));
						ifstream iSource;
						iSource.open(filename.c_str());

						string temp = "";
						int countLine = 0;
						int realLine = 0;


						int passTot = 0;

						unordered_map<int, int> rewrite;
						//original beginLine => new beginLine

						while(getline(iSource, temp, '\n')){
							if(unDet.find(countLine) != unDet.end()){
								rewrite[countLine] = realLine;
								int tempCount = unDet[countLine];
								for(int i=0; i<tempCount; ++i){
									countLine ++;
									realLine ++;
									tempFile << temp << "\n";
								}
							}
							else if(deLine.find(countLine) != deLine.end()){
								int passLine = deLine[countLine];
								passTot += passLine;
								for(int i=0; i<passLine - 1; ++i){
									getline(iSource, temp);
									countLine ++;
								}
							}
						}

						for(map<string, lineInfo>::iterator it = local_storage[row].begin(); it != local_storage[row].end(); ++it){
							if((it -> second).type == type){
								(it -> second).beginLine = rewrite[(it -> second).beginLine];
							}
						}

						if(type == 1) mailLine[row] = mailLine[row] - passTot;
						else if(type == 2) driveLine[row] = driveLine[row] - passTot;

						//delete the old file, rename the new file
						if(type == 1){
							string mail = row + "/mail.txt";
							string tempMail = row + "/temp_mail.txt";
							remove(mail.c_str());
							rename(tempMail.c_str(), mail.c_str());
						} 
						else if(type == 2){
							string drive = row + "/drive.txt";
							string tempMail = row + "/temp_drive.txt";
							remove(drive.c_str());
							rename(tempMail.c_str(), drive.c_str());
						}

						local_storage[row].erase(col);
						string delete_success = "+OK delete success\n";
						writeString(delete_success, comm_fd);

					}

				}
			}

			else if(cmd == "RENAME" || cmd == "MOVE"){
				if(hasV) printf("%s\n", "RENAME or MOVE command, modifying data");

				string newName;
				getline(PPAP, newName, '`');
				lineInfo temp;
				for(map<string, lineInfo>::iterator it = local_storage[row].begin(); it != local_storage[row].end(); ++it){
					if((it -> first) == col){
						temp = (it -> second);
						local_storage[row].erase(col);
						break;
					}
				}
				local_storage[row][newName] = temp;

				string success = "+OK " + cmd + " success\n";
				writeString(success, comm_fd);
				
			}
			else if(cmd == "ALLMAIL"){
				if(hasV) printf("%s\n", "ALLMAIL command, fetching data");

				int numMail = 0;
				string res = "";
				for(map<string, lineInfo>::iterator it = local_storage[row].begin(); it != local_storage[row].end(); ++it){
					if((it -> second).deleted == 1) continue;
					if((it -> second).type == 1){
						numMail ++;
						res += ("`" + it -> first);
					}   
				}
				res = to_string(numMail) + res + "\n";
				writeString(res, comm_fd);
			}

			else{

				if(hasV) printf("%s\n", "This is not valid command");

				char err[22] = "-ERR Unknown command\n";
				do_write(comm_fd, err, strlen(err));
				if(hasV){
					char dumText[texL];
					memcpy(dumText, &con[0], texL);
					string wholeB = dumText;
					string useb = wholeB.substr(0, texL);

					fprintf(stderr, "[%d] C: %s\n", comm_fd, useb.c_str());
					fprintf(stderr, "[%d] S: %s", comm_fd, err);
				}
			}
		}
		if(con.length() != 0) memcpy(buf, &con[0], con.length());
		startP = con.length();
	}
	
	threadBook[comm_fd] = 0;
	close(comm_fd);
	pthread_exit(NULL);

}
int main(int argc, char *argv[])
{
	signal(SIGINT, stopCatcher);
  	if (argc < 2) {
    	fprintf(stderr, "*** Author: Your name here (SEASlogin here)\n");
    	exit(1);
  	}

 //getopt part
	bool hasP = 0;
	int servNum = -1;
	bool hasA = 0;
	bool hasV = 0;

	for(int i=1; i<argc; ++i){

		if(strcmp(argv[i], "-p\0") == 0){
			hasP = 1;
			servNum = atoi(argv[i+1]);
		}
		if(strcmp(argv[i], "-a\0") == 0) hasA = 1;
		if(strcmp(argv[i], "-v\0") == 0) hasV = 1;
	}

	if(hasA){
		fprintf(stderr, "Yizhou Luo, luoyiz\n");
		exit(0);
	}

	if(hasP == 0){
		fprintf(stderr, "please specify the NO. of this server !\n");
		exit(1);
	}


	//read in the File, and bind the address
	//store the master's address for later communcation
	string serverFile = "servers.txt";
	ifstream iServer;
	iServer.open(serverFile.c_str());
	string line;

	for(int i=0; i<=servNum; ++i){
		getline(iServer, line);
	}

	int semiPos = getSemiPos(line);
	string bindIP = line.substr(0, semiPos);
	string bindPort = line.substr(semiPos+1, line.length() - semiPos - 1);


	int listen_fd = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	//servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	servaddr.sin_addr.s_addr = inet_addr(bindIP.c_str());
	//servaddr.sin_port = htons(portNum);
	servaddr.sin_port = htons(atoi(bindPort.c_str()));
	bind(listen_fd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	listen(listen_fd, 10);

	while (true) {

		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof(clientaddr);
    	int *fd = (int*) malloc(sizeof(int));
		*fd = accept(listen_fd, (struct sockaddr*)&clientaddr, &clientaddrlen);

		fd_list.push_back(*fd);
		threadBook[*fd] = 1;
		//printf("Connection from %s\n", inet_ntoa(clientaddr.sin_addr));
		if(hasV) fprintf(stderr, "[%d] New connection\n", *fd);
		//argument needed by the thread
		argClient *pta = new argClient;
		pta -> fileDes = *fd;
		pta -> hasV = hasV;

		pthread_t thread;
		pthread_create(&thread, NULL, worker, pta);
		//delete pta;
	}
  	return 0;
}

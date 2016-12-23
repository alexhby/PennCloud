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
#include <climits>

using namespace std;

/*
string to_string(int num){
  ostringstream os;
  os << num;
  return os.str();
}
*/


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

unordered_map<string, string> slaveInfo;
//userName => slave index
unordered_map<string, int> numFiles;
//slave (server) index => how many files on that server node


/*
the two maps above need to be initialized
from a file containing the IP addresses of server node
*/



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


		if(hasV){
			cout << "the command master recieved is: " << bufS << endl;
		}

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

			if(cmd == "REQUEST"){
				if(hasV) printf("%s\n", "Request command, fetching index");

				string userName;
				getline(PPAP, userName, '`');

				if(slaveInfo.find(userName) == slaveInfo.end()){
					string exist = "-ERR user does not exist\n";
					writeString(exist, comm_fd);
				}
				else{
					string IP_server = slaveInfo[userName];
					numFiles[IP_server] ++;
					writeString(IP_server, comm_fd);

					if(hasV) printf("Output is : %s\n", IP_server.c_str());
				}

				
			}
			else if(cmd == "CREATE"){
				if(hasV) printf("%s\n", "Create command");

				string userName;
				getline(PPAP, userName, '`');
				if(slaveInfo.find(userName) != slaveInfo.end()){
					string exist = "-ERR user already exist\n";
					writeString(exist, comm_fd);
				}
				else{
					//first find the server wtih least files
					int curMin = INT_MAX;
					string userRet = "";
					for(unordered_map<string, int>::iterator it = numFiles.begin(); it != numFiles.end(); ++it){
						if(it -> second < curMin){
							curMin = it -> second;
							userRet = it -> first;
						}
					}
					//userRet += "\n";
					slaveInfo[userName] = userRet;
					writeString(userRet, comm_fd);
					if(hasV) printf("Output is : %s\n", userRet.c_str());

				}

			}
			else if(cmd == "print"){
				if(hasV) printf("%s\n", "Print command");

				string slaveInfoS = "";
				string numFileS = "";
				for(unordered_map <string, string>::iterator it = slaveInfo.begin(); it != slaveInfo.end(); ++it){
					slaveInfoS += (it -> first + " " + it -> second + "\n");
				}
				for(unordered_map <string, int>::iterator it = numFiles.begin(); it != numFiles.end(); ++it){
					numFileS += (it -> first + " " + to_string(it -> second) + "\n");
				}
				string res = slaveInfoS + "\n\n"+ numFileS;
				writeString(res, comm_fd);
			}

			else{
				char err[22] = "-ERR Unknown command\n";
				do_write(comm_fd, err, strlen(err));
				if(hasV) printf("%s\n", "Invalid command");
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
	int portNum = 10000;
	bool hasA = 0;
	bool hasV = 0;

	for(int i=1; i<argc; ++i){

		if(strcmp(argv[i], "-p\0") == 0){
			hasP = 1;
			portNum = atoi(argv[i+1]);
		}
		if(strcmp(argv[i], "-a\0") == 0) hasA = 1;
		if(strcmp(argv[i], "-v\0") == 0) hasV = 1;
	}

	if(hasA){
		fprintf(stderr, "Yizhou Luo, luoyiz\n");
		exit(0);
	}


	string serverFile = "servers.txt";
	ifstream iServer;
	iServer.open(serverFile.c_str());
	string line;

	getline(iServer, line);

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

/* initialize the map structure */

	for(int i=0; i<2; ++i){
		numFiles[to_string(i)] = 0;
	}

	while (true) {

		struct sockaddr_in clientaddr;
		socklen_t clientaddrlen = sizeof(clientaddr);
    	int *fd = (int*) malloc(sizeof(int));
		*fd = accept(listen_fd, (struct sockaddr*)&clientaddr, &clientaddrlen);

		fd_list.push_back(*fd);
		threadBook[*fd] = 1;
		//printf("Connection from %s\n", inet_ntoa(clientaddr.sin_addr));
		if(hasV) fprintf(stderr, "[%d] New connection\n", *fd);
		//send a greeting to client
		//char greet[60] = "+OK Server ready (Author: Yizhou Luo / luoyiz)\n";
		//do_write(*fd, greet, strlen(greet));
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

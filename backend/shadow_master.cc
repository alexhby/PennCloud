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
using namespace std;

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

map<string, map <string, string> > local_storage;
// row => (col => local store)


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
		while(getline(iStream, con, '\n')){
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
			PPAP >> cmd;
			upperCase(cmd);

			string row;
			string col;
			PPAP >> row;
			PPAP >> col;

			string ok = "+OK ";
			if(cmd == "PUT"){
				printf("%s\n", "PUT command, storing data");
				string value;
				string temp;
				while(PPAP >> temp){
					value += (temp + " ");
				}
				value = value.substr(0, value.length() - 1);
				local_storage[row][col] = value;
				string putSuccess = "+OK PUT success\n";
				writeString(putSuccess, comm_fd);

			}
			else if(cmd == "GET"){

				if(local_storage.find(row) != local_storage.end() && local_storage[row].find(col) != local_storage[row].end()){
					string return_value = local_storage[row][col];
					string ret = "value: " + return_value + "\n";
					writeString(ret, comm_fd);
				}
				else{
					char notFound[40] = "-ERR resource not found\n";
					do_write(comm_fd, notFound, strlen(notFound));

				}
			}
			else if(cmd == "CPUT"){
				string v1;
				string v2;
				PPAP >> v1;
				PPAP >> v2;
				if(local_storage.find(row) == local_storage.end() || local_storage[row].find(col) == local_storage[row].end()){
					//no value in it
					string no_value = "-ERR no value stored in this location\n";
					writeString(no_value, comm_fd);					
				}
				else if(local_storage[row][col] != v1){
					//stored is not v1
					string stored_not= "-ERR stored value is not v1\n";
					writeString(stored_not, comm_fd);
				}
				else{
					//replace v1 with v2
					local_storage[row][col] = v2;
					string CPUT_success = "+OK CPUT success\n";
					writeString(CPUT_success, comm_fd);
				}

			}
			else if(cmd == "DELETE"){
				if(local_storage.find(row) == local_storage.end() || local_storage[row].find(col) == local_storage[row].end()){
					//no value in it
					string no_value = "-ERR no value stored in this location\n";
					writeString(no_value, comm_fd);					
				}
				else{
					local_storage[row].erase(col);
					string delete_success = "+OK delete success\n";
					writeString(delete_success, comm_fd);
				}
			}
			else{
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

	int listen_fd = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htons(INADDR_ANY);
	servaddr.sin_port = htons(portNum);
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
		//send a greeting to client
		char greet[60] = "+OK Server ready (Author: Yizhou Luo / luoyiz)\n";
		do_write(*fd, greet, strlen(greet));
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

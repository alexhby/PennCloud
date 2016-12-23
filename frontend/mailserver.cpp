#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h> 
#include <pthread.h>
#include <signal.h>
#include <map>
#include <sys/stat.h>
#include <iostream>
#include "createpage.h"

void doWrite(int fd, char* buf, int len);

bool vflag = true;

std::string mailserver(std::string username, std::string message, struct sockaddr_in serverSock){ // name: current log in name; e.g., mazhiyu; potential session id; 
	printf("In mailserver\n");
	printf("username is %s, sockaddr_in is %s and %d\n", username.c_str(),inet_ntoa(serverSock.sin_addr),ntohs(serverSock.sin_port));
	char* name = new char[512];
	bzero(name, sizeof(name));
	strcpy(name, username.c_str());

	char* info = new char[2000];
	bzero(info, sizeof(info));
	strcpy(info, message.c_str());

	char* s; 

	while(s = strstr(info, "\r")){
		int rIndex = (int)(s-info);

		for(int i = rIndex; i < strlen(info)-1; i++){
			info[i] = info[i+1]; 
		}

		info[strlen(info)-1] = '\0';
	}
	
	while(s = strstr(info, "\n")){
		int nIndex = (int)(s-info);

		for(int i = nIndex; i < strlen(info)-1; i++){
			info[i] = info[i+1]; 
		}

		info[strlen(info)-1] = '\0';
	}

	if(vflag){
		std::cerr << "Got message from frontend server: " << message << " with length: " << message.length() << std::endl;
	}

	char* outMessage = new char[512];
	char* inMessage = new char[512];
	char* pageInMessage = new char[512];

	char* ss = new char[512];	

	if(ss = strstr(info, "/listpage")){ // action=/listpage; 
		
		if(vflag){
			std::cerr << "action=/listpage" << std::endl;
		}

		int tempSocket = socket(PF_INET, SOCK_STREAM, 0);
		if(tempSocket < 0 && vflag){
			std::cerr << "Cannot open socket" << std::endl;
		}

		else{
			connect(tempSocket, (struct sockaddr*)&serverSock, sizeof(serverSock));

			bzero(outMessage, sizeof(outMessage));
			strcat(outMessage, "ALLMAIL`"); 
			strcat(outMessage, name); 
			strcat(outMessage, "`\n");  

			doWrite(tempSocket, outMessage, strlen(outMessage));

			if(vflag){
				std::cerr << "Message send to server: " << inet_ntoa(serverSock.sin_addr) << " " << ntohs(serverSock.sin_port) << ", message: " << outMessage << std::endl;
			}

			bzero(inMessage, sizeof(inMessage));
			read(tempSocket, inMessage, 512); //3`e>title1`e>title2`e>title3; 

			if(vflag){
				std::cerr << "Got ALLMAIL returning message: " << inMessage << std::endl; 
			}

			close(tempSocket); 

			bzero(pageInMessage, sizeof(pageInMessage));
			strcat(pageInMessage, "listpage`");
			strcat(pageInMessage, inMessage); 

			if(vflag){
				std::cerr << "Message to createPage function: " << pageInMessage << std::endl;
			}

			return createPage(std::string(pageInMessage)); //"listpage`3`title1`title2"; 
			
		}
	}

	else if(ss = strstr(info, "/checkpage")){ // action=/checkpage&&docheck=title1

		if(vflag){
			std::cerr << "action=/checkpage" << std::endl;
		}

		ss = strstr(info, "docheck=");
		int ssIndex = (int)(ss-info);

		char* mailTitle = new char[512];
		bzero(mailTitle, sizeof(mailTitle));

		strncpy(mailTitle, info+ssIndex+8, (int)(strlen(info)-ssIndex-8));
		
		if(vflag){
			std::cerr << "Got email title: " << mailTitle << " with length: " << strlen(mailTitle) << std::endl;
		}

		int tempSocket = socket(PF_INET, SOCK_STREAM, 0);
		if(tempSocket < 0 && vflag){
			std::cerr << "Cannot open socket" << std::endl;
		}

		connect(tempSocket, (struct sockaddr*)&serverSock, sizeof(serverSock));

		bzero(outMessage, sizeof(outMessage));
		strcat(outMessage, "GET`");
		strcat(outMessage, name);
		strcat(outMessage, "`e>");
		strcat(outMessage, mailTitle);
		strcat(outMessage, "`\n"); 

		doWrite(tempSocket, outMessage, strlen(outMessage)); 

		if(vflag){
			std::cerr << "Message send to server: " << inet_ntoa(serverSock.sin_addr) << " " << ntohs(serverSock.sin_port) << ", message: " << outMessage << std::endl;
		}

		bzero(inMessage, sizeof(inMessage));
		read(tempSocket, inMessage, 512); //"content";

		close(tempSocket);

		if(vflag){
			std::cerr << "Got GET email returning message: " << inMessage << std::endl;
		}

		bzero(pageInMessage, sizeof(pageInMessage));
		strcat(pageInMessage, "checkpage`");
		strcat(pageInMessage, inMessage); //checkpage`content;

		if(vflag){
			std::cerr << "Message to createPage function: " << pageInMessage << std::endl;
		}
		return createPage(std::string(pageInMessage));
		
	}

	else if(ss = strstr(info, "/replypage")){ // action=/replypage&&doreply=title1

		if(vflag){
			std::cerr << "action=/replypage" << std::endl;
		}

		ss = strstr(info, "doreply="); 
		int ssIndex = (int)(ss-info);

		char* mailTitle = new char[512];
		bzero(mailTitle, sizeof(mailTitle));

		strncpy(mailTitle, info+ssIndex+8, (int)(strlen(info)-ssIndex-8));

		if(vflag){
			std::cerr << "Got reply email title: " << mailTitle << std::endl;
		}

		bzero(pageInMessage, sizeof(pageInMessage));
		strcat(pageInMessage, "replypage`");
		strcat(pageInMessage, mailTitle);

		if(vflag){
			std::cerr << "Message to createPage function: " << pageInMessage << std::endl;
		}

		return createPage(std::string(pageInMessage)); //replypage`title1
		
	}

	else if(ss = strstr(info, "/forwardpage")){ // action=/forwardpage&&doforward=title1

		if(vflag){
			std::cerr << "action=/forwardpage" << std::endl;
		}

		ss = strstr(info, "doforward=");
		int ssIndex = (int)(ss-info);

		char* mailTitle = new char[512];
		bzero(mailTitle, sizeof(mailTitle));

		strncpy(mailTitle, info+ssIndex+10, (int)(strlen(info)-ssIndex-10));
		
		if(vflag){
			std::cerr << "Got email title: " << mailTitle << " with length: " << strlen(mailTitle) << std::endl;
		}

		bzero(pageInMessage, sizeof(pageInMessage));
		strcat(pageInMessage, "forwardpage`");
		strcat(pageInMessage, mailTitle); //checkpage`content;

		if(vflag){
			std::cerr << "Message to createPage function: " << pageInMessage << std::endl;
		}

		return createPage(std::string(pageInMessage));
		
	}

	else if(ss = strstr(info, "/deletepage")){ // action=/deletepage&&dodelete=title1

		if(vflag){
			std::cerr << "action=/deletepage" << std::endl;
		}

		ss = strstr(info, "dodelete=");
		int ssIndex = (int)(ss-info);

		char* mailTitle = new char[512];
		bzero(mailTitle, sizeof(mailTitle));

		strncpy(mailTitle, info+ssIndex+9, (int)(strlen(info)-ssIndex-9));

		if(vflag){
			std::cerr << "Got delete email title: " << mailTitle << std::endl;
		}

		int tempSocket = socket(PF_INET, SOCK_STREAM, 0);
		if(tempSocket < 0 && vflag){
			std::cerr << "Cannot open socket" << std::endl;
		}

		connect(tempSocket, (struct sockaddr*)&serverSock, sizeof(serverSock));

		bzero(outMessage, sizeof(outMessage));
		strcat(outMessage, "DELETE`");
		strcat(outMessage, name);
		strcat(outMessage, "`e>");
		strcat(outMessage, mailTitle);
		strcat(outMessage, "`\n"); 

		doWrite(tempSocket, outMessage, strlen(outMessage));

		if(vflag){
			std::cerr << "Message send to server: " << inet_ntoa(serverSock.sin_addr) << " " << ntohs(serverSock.sin_port) << ", message: " << outMessage << std::endl;
		}

		bzero(inMessage, sizeof(inMessage));
		read(tempSocket, inMessage, 512); //+OK

		close(tempSocket);

		if(vflag){
			std::cerr << "Got DELETE email returning message: " << inMessage << std::endl;
		}

		bzero(pageInMessage, sizeof(pageInMessage));
		strcat(pageInMessage, "delete`");
		strcat(pageInMessage, inMessage); 

		return mailserver(username, "/listpage", serverSock); 
		
	}

	else if(ss = strstr(info, "composepage")){ // composepage=newemail

		if(vflag){
			std::cerr << "action=/composepage" << inMessage << std::endl;
		}

		bzero(pageInMessage, sizeof(pageInMessage));
		strcat(pageInMessage, "composepage`"); 

		if(vflag){
			std::cerr << "Message to createPage function: " << pageInMessage << std::endl;
		}
		
		return createPage(std::string(pageInMessage));//composepage`
		
	}

	else if(ss = strstr(info, "/composesuccess")){ // action=/composesuccess&to=linh&title=title1&content=dsd 

		if(vflag){
			std::cerr << "Trying to compose email" << std::endl;
		}

		char* to = new char[512];
		bzero(to, sizeof(to));
		char* title = new char[512];
		bzero(title, sizeof(title));
		char* content = new char[1024];
		bzero(content, sizeof(content));

		if(vflag){
			std::cerr << "Parse out from value: " << name << " with length: " << strlen(name) << std::endl;
		}

		ss = strstr(info, "to=");
		int ssIndex = (int)(ss-info);

		int i = 3; 
		
		while(ssIndex+i < strlen(info) && info[ssIndex+i] != ' ' && info[ssIndex+i] != '\n' && info[ssIndex+i] != '&'){
			strncat(to, &info[ssIndex+i], 1);
			i++;
		}

		if(vflag){
			std::cerr << "Parse out to value: " << to << " with length: " << strlen(to) << std::endl;
		}

		ss = strstr(info, "title=");
		ssIndex = (int)(ss-info);

		i = 6; 
		
		while(ssIndex+i < strlen(info) && info[ssIndex+i] != '\n' && info[ssIndex+i] != '&'){
			strncat(title, &info[ssIndex+i], 1);
			i++;
		}

		if(vflag){
			std::cerr << "Parse out title value: " << title << " with length: " << strlen(title) << std::endl;
		}

		ss = strstr(info, "content=");
		ssIndex = (int)(ss-info);

		i = 8; 
		
		while(ssIndex+i < strlen(info) && info[ssIndex+i] != '\n' && info[ssIndex+i] != '&'){
			strncat(content, &info[ssIndex+i], 1);
			i++;
		}

		if(vflag){
			std::cerr << "Parse out content value: " << content << " with length: " << strlen(content) << std::endl;
		}

		int tempSocket = socket(PF_INET, SOCK_STREAM, 0);
		if(tempSocket < 0 && vflag){
			std::cerr << "Cannot open socket" << std::endl;
		}

		connect(tempSocket, (struct sockaddr*)&serverSock, sizeof(serverSock));

		bzero(outMessage, sizeof(outMessage));
		strcat(outMessage, "PUT`");
		strcat(outMessage, to);
		strcat(outMessage, "`e>");
		strcat(outMessage, title);
		strcat(outMessage, "`");
		strcat(outMessage, "from: ");
		strcat(outMessage, name);
		//strcat(outMessage, "\n");
		strcat(outMessage, " ");
		strcat(outMessage, "to: ");
		strcat(outMessage, to);
		//strcat(outMessage, "\n");
		strcat(outMessage, " ");
		strcat(outMessage, content);
		strcat(outMessage, "`\n");

		if(vflag){
			std::cerr << "Message send to server: " << inet_ntoa(serverSock.sin_addr) << " " << ntohs(serverSock.sin_port) << ", message: " << outMessage << std::endl;
		}

		doWrite(tempSocket, outMessage, strlen(outMessage));

		bzero(inMessage, sizeof(inMessage));
		read(tempSocket, inMessage, 512); 

		close(tempSocket);

		if(vflag){
			std::cerr << "Got compose returning message: " << inMessage << std::endl;
		}

		return mailserver(username, "/listpage", serverSock);
		
	}

}

void doWrite(int fd, char* buf, int len){
	int sentSize = 0; 
	
	while(sentSize < len){
		int newSent = write(fd, &buf[sentSize], len-sentSize);
		
		if(newSent < 0){
			return;
		}
		
		sentSize += newSent;
	}
}

// int main(){
// 	struct sockaddr_in serverSock; 
//  	bzero(&serverSock, sizeof(serverSock));
// 	mailserver("mazhiyu", "action=/listpage", serverSock);
// 	mailserver("mazhiyu", "action=/checkpage&&docheck=title1", serverSock);
// 	mailserver("mazhiyu", "action=/replypage&&doreply=title1", serverSock);
// 	mailserver("mazhiyu", "action=/forwardpage&&doforward=title1", serverSock);
// 	mailserver("mazhiyu", "action=/deletepage&&dodelete=title1", serverSock);
// 	mailserver("mazhiyu", "action=/composepage", serverSock);
//  	mailserver("mazhiyu", "action=/composesuccess&&to=linh&&title=title1&&content=123", serverSock);
//  	return 1;
// }

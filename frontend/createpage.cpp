#include <iostream>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "createpage.h"

std::string createPage(std::string temp){

	std::string returnString("");

	char* ttemp = new char[512]; // parse temp string
	bzero(ttemp, sizeof(ttemp)); 
	strcpy(ttemp, temp.c_str()); 

	char* ctemp = new char[512]; // read template html
	bzero(ctemp, sizeof(ctemp));
	
	char* count = new char[512]; 
	bzero(count, sizeof(count));
	int iCount; 

	std::vector<char*> titles; 

	int i = 1; 

	char* s; 
	if(s = strstr(ttemp, "listpage`")){ // listpage`3`e>title1`e>title2`e>title3

		while(8+i < strlen(ttemp) && ttemp[8+i] != '`' && ttemp[8+i] != '\r' && ttemp[8+i] != '\n'){

			strncat(count, &ttemp[8+i], 1);
			i++;
		}

		iCount = atoi(count);
		std::cerr << "count: " << iCount << std::endl;
		
		i++;

		for(int j = 0; j < iCount; j++){

			char* title = new char[512];
			bzero(title, sizeof(title));

			while(8+i < strlen(ttemp) && ttemp[8+i] != '`' && ttemp[8+i] != '\r' && ttemp[8+i] != '\n'){
				
				strncat(title, &ttemp[8+i], 1); 
				i++;
		
			}

			std::cerr << "title: " << title << std::endl;

			titles.push_back(title);

			i++;
		}

		if(FILE* composeFile = fopen("html/emailsTemplate.html", "r")){

			bzero(ctemp, sizeof(ctemp));

			while(fgets(ctemp, 512, composeFile)){

				if(ctemp[0] == '<' && ctemp[1] == 't' && ctemp[2] == 'a' && ctemp[3] == 'b' && ctemp[4] == 'l' && ctemp[5] == 'e'){

					returnString += std::string(ctemp);
					
					while(!titles.empty()){ // <tr><td>title1<br><form action="/checkpage" method="post"><button name="doget" value="title1">View email details</button></form><form action="/deletepage" method="post"><button name="dodelete" value="title1">Delelte this mail</button></tr>

						char* faketitle = new char[512];
						bzero(faketitle, sizeof(faketitle));
						strcpy(faketitle, titles.back()+2);

						returnString += "<tr><td>";

						returnString += faketitle;
						returnString += "<br>"; 

						returnString += "<form action=\"/checkpage\" method=\"post\"><button name=\"docheck\" value=\"";
						returnString += faketitle;
						returnString += "\">View email</button></form>"; 
						returnString += "\n";

						returnString += "<form action=\"/replypage\" method=\"post\"><button name=\"doreply\" value=\"";
						returnString += faketitle; 
						returnString += "\">Reply email</button></form>"; 
						returnString += "\n";

						returnString += "<form action=\"/forwardpage\" method=\"post\"><button name=\"doforward\" value=\"";
						returnString += faketitle; 
						returnString += "\">Forward email</button></form>"; 
						returnString += "\n";
 
						returnString += "<form action=\"/deletepage\" method=\"post\"><button name=\"dodelete\" value=\"";
						returnString += faketitle; 
						returnString += "\">Delete email</button></form>"; 
						returnString += "\n";

						returnString += "</tr>"; 
						

						titles.pop_back();
					}

					

				}

				else{
					
					returnString += std::string(ctemp);

				}

			}

		}
		else{
			std::cerr << "Cannot open emailsTemplate.html" << std::endl;
		}

		return returnString;
	}

	else if(s = strstr(ttemp, "checkpage`")){ // checkpage`content

		if(FILE* composeFile = fopen("html/checkpageTemplate.html", "r")){

			bzero(ctemp, sizeof(ctemp));

			while(fgets(ctemp, 512, composeFile)){

				if(ctemp[0] == '<' && ctemp[1] == 'b' && ctemp[2] == '>' && ctemp[3] == 'Y' && ctemp[4] == 'o' && ctemp[5] == 'u' && ctemp[6] == 'r'){

					returnString += std::string(ctemp);
					returnString += "<p>"; 

					char* content = new char[512];
					bzero(content, sizeof(content));
					strcpy(content, ttemp+10);

					returnString += content;

					returnString += "</p>";
				}

				else{
					
					returnString += std::string(ctemp);

				}

			}

		}

		else{
			std::cerr << "Cannot open checkpageTemplate.html" << std::endl;
		}

		return returnString;
	}

	else if(s = strstr(ttemp, "replypage`")){ //replypage`content

		if(FILE* composeFile = fopen("html/replyTemplate.html", "r")){

			bzero(ctemp, sizeof(ctemp));

			while(fgets(ctemp, 512, composeFile)){

				if(ctemp[0] == '<' && ctemp[1] == 't' && ctemp[2] == 'd' && ctemp[3] == '>' && ctemp[4] == 'T' && ctemp[5] == 'i' && ctemp[6] == 't' && ctemp[7] == 'l' && ctemp[8] == 'e'){

					returnString += "<td>Title:<br><input type=\"text\" name=\"title\" value=\"Re"; 

					char* content = new char[512];
					bzero(content, sizeof(content));
					strcpy(content, ttemp+10);

					returnString += content;

					returnString += "\"></td>";
				}

				else{
					
					returnString += std::string(ctemp);

				}

			}

		}

		else{
			std::cerr << "Cannot open replyTemplate.html" << std::endl;
		}

		return returnString;
	}

	else if(s = strstr(ttemp, "forwardpage`")){ //forwardpage`content

		if(FILE* composeFile = fopen("html/replyTemplate.html", "r")){

			bzero(ctemp, sizeof(ctemp));

			while(fgets(ctemp, 512, composeFile)){

				if(ctemp[0] == '<' && ctemp[1] == 't' && ctemp[2] == 'd' && ctemp[3] == '>' && ctemp[4] == 'T' && ctemp[5] == 'i' && ctemp[6] == 't' && ctemp[7] == 'l' && ctemp[8] == 'e'){

					returnString += "<td>Title:<br><input type=\"text\" name=\"title\" value=\"Fwd"; 

					char* content = new char[512];
					bzero(content, sizeof(content));
					strcpy(content, ttemp+12);

					returnString += content;

					returnString += "\"></td>";
				}

				else{
					
					returnString += std::string(ctemp);

				}

			}

		}

		else{
			std::cerr << "Cannot open replyTemplate.html" << std::endl;
		}

		return returnString;
	}

	else if(s = strstr(ttemp, "composepage`")){ // composepage`

		if(FILE* composeFile = fopen("html/compose.html", "r")){

			bzero(ctemp, sizeof(ctemp));

			while(fgets(ctemp, 512, composeFile)){
					
				returnString += std::string(ctemp);

			}

		}

		else{
			std::cerr << "Cannot open compose.html" << std::endl;
		}

		return returnString;
	}

}

// int main(){
// 	std::cerr << createPage(std::string("/listpage`3`e>title1`e>title2`e>title3"));
// 	//std::cerr << createPage(std::string("/checkpage`this is the email"));
// 	//std::cerr << createPage(std::string("/replypage`title1"));
// 	//createPage(std::string("ALLMAIL,3,title4,title5,title6"));
// }

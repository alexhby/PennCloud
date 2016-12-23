# PennCloud
A distributed Cloud Drive and Mail Service Application

## Team
[Boyi He](https://github.com/alexhby), Yizhou Luo, Zhiyu Ma, Zhenghao Yang
University of Pennsylvania

## Email Feature
There are two main parts related to the mail server design and implementation, which are client-mail server communication and mail server-backend server communication. 
In html pages related to mail server, the browser passes a command with a corresponding action name, attribute values, and the current username to the mail server. After parsing this command, mail server sets up a socket connection with the corresponding back-end storage server and uses the protocol defined in the project description (PUT, GET...) to perform actions.  
After getting responses from its backend server, the mail server calls a function to create a dynamic html page and returns it to the browser for displaying. 
Basic functions provided by the mail server are: 
	1. List all the email for the current user (ALLMAIL command)
	2. Compose new email (PUT command)
	3. Viewing certain email details of the current user (GET command) 
	4. Replying to a certain email (PUT command) 
	5. Forwarding a certain email (PUT command) 
	6. Deleting a certain email (Delete command). 


## Frontend Server
The frontend server (multithreaded) is responsible for the communication between the user (the browser) and the backend server; it is able to receive three commands from the browser, GET, HEAD, and POST.
GET commands will fetch any static webpage for the browser from the frontend server. All static html pages are stored in the frontend server, so there is no need to request the webpage from the backend. Therefore, GET commands will not have any communication between the frontend server and the backend server. 
HEAD commands functions exactly the same as GET commands, except that HEAD commands only return the header part of the message from the frontend server.
POST commands are used when the user wants to fetch data, change data, and send data. POST commands will get dynamic web pages as responses, since different users have different files and emails. When the browser sends a POST request, the frontend server will grab the POST body and pass it onto the backend server. Then, the backend server will respond with corresponding data. Next, the frontend server will create a webpage with the data from the backend and send it to the browser. If it is the user’s first time logging into the system or signing up, the response to the browser will also include a cookie message, so that the user will not need to login every time he makes a POST request.
In addition, the frontend server also has an admin page, which shows the status of frontend servers and backend servers, whether they are online or offline. If the admin clicks on an online backend server, the admin can also see the data and information stored on that server.


## Frontend RPCs, Drive Feature and Admin Console
The remote procedure calls from frontend together act as an interface between frontend and backend servers. The RPCs are implemented as well-encapsulated functions. Each of them is specific for a single frontend action that require interaction with the backend. The RPCs are location transparent for the frontend servers. Besides, the basic operations that are defined by the protocol for frontend-backend communication are implemented as separate functions, such as sentPut() and sentGet(). This design enables us to change and add new basic operations flexibly. When a RPC function is called, it typically create new socket to connect with the backend servers through TCP, and call the functions of basic operations to send messages. 

The RPC functions provided are (/frontend/frontend_internal.cpp):
* checkLogin(): Send request to master for the index of the node that contains the data of a given user and compare its password
* createNewUser(): Send request to master for a node to put data for a new user
* changePassword(): PUT a new password
* setSID(), getSID()
* listFiles(): GET the files and folders under a given path
* allPaths(): get all the available paths for moving, given the current path
* newfile(), newfolder(): PUT a new file/ folder
* deleteByPath(): delete a file
* renameFile()
* getFile(): get file content
* getStatus(): get the status of servers, given a vector of addresses
* getRaw(): get raw data of a backend node

The drive feature is implemented based on the corresponding RPC functions, and rendering the dynamic HTML pages (/frontend/render.cpp). When user enter the drive session, the root path will be shown with all available folders and files under the path. User can enter a folder, delete a folder; download, rename, move or delete a file; add new file and new folder.

The static HTML pages are stored within frontend/html/. These includes login, signup, home page and the pages with different notification messages. The frontend server calls renderPage() to read those HTML files and send to clients. The dynamic pages for drive and admin console are rendered based on the input data.

The admin console feature is implemented by getting the status of all the servers. When the admin page is requested by the client, the frontend server would call getStatus() once each for frontend and backend servers. getStatus() would try to connect with each server, if the connection failed, the server would be marked as down. Then the renderAdmin() is called to render the admin console page based on the statuses. For all the backend node that is not down, a button will be provided to check its raw data, through getRaw().


## Backend Server and Master
The backend servers are storage nodes that store the user’s information and data content. They have both some metadata and file on the disk. The master is responsible for storing the corresponding relation of the user’s account information and server ID. This makes it possible for the master to do load balancing and answer requests for providing the frontend server with the correct server ID number. 
Both the master and servers use multi-thread approach. Upon receiving request, it opens a new thread for answering that request.
They together forms a distributed storage system that provides service of key-value storage.

The command (API) that the master provides are:
	1. REQUEST command. The basic assumption master makes is that the user column in this command has already been created. Otherwise it will give an error message to the frontend server. It provides the server ID that corresponding to that user.
	2. CREATE command. When the frontend server want to create a new account for a user, it issues a create command to master. The master do load-balancing based on the existing account numbers on each of the servers it keeps track of. It then provides the frontend server with the server that has the least content stored.
	3. ALIVE command. Note that the master itself is also a server that we want to know the current status (dead or alive).

The command the storage server provides are:
	1. PUT command. Put the data corresponding to the user and file name specified. Note that it is also used by the frontend server to create folders and store emails.
	2. GET command. Get the data corresponding to the user and file name specified. Note that it is also possible that the value returned is the folder and the files contained in that path if that path is a folder instead of a file name.
	3. CPUT. It replaces the old version content by newer version provided the user know what the older version is. It is used in situations such as changing passwords.
	4. DELETE command. The delete command will first mark the files as deleted. When the deleted files have accumulated to a certain number, it flushes the files and do the real deletion. This saves space by eliminating fragments in the disk.
	5. RENAME or MOVE. This command enables the frontend server to change the name of the files or move the path.
	6. ALLMAIL command. This command provides the email server with all the mail name of the user it specifies. 
	7. RAW command. In the console we want to see the data stored on each of the node. So RAW will enable the frontend server see the data.
	8. ALLPATH. Returns all the path of that user.
	9. ALIVE. Sends the heartbeat message.

#include "client_header.h"

/*************************************************************/
/*           Handles different requests from peer client     */
/*************************************************************/
void handleClientRequest(int client_socket){
    string client_uid = "";

    writeLog("\nclient socket num: " + to_string(client_socket) + "\n");
    char inptline[1024] = {0}; 

    if(read(client_socket , inptline, 1024) <=0){
        close(client_socket);
        return;
    }
    
    writeLog("client request at server " + string(inptline));
    vector<string> inpt = splitString(string(inptline), "$$");
    writeLog(inpt[0]);

    if(inpt[0] == "get_chunk_vector"){
        writeLog("\nsending chunk vector..");
        string filename = inpt[1];
        vector<int> chnkvec = fileChunkInfo[filename];
        string tmp = "";
        for(int i: chnkvec) tmp += to_string(i);
        char* reply = &tmp[0];
        write(client_socket, reply, strlen(reply));
        writeLog("sent: " + string(reply));
    }
    else if(inpt[0] == "get_chunk"){
        //inpt = [get_chunk, filename, to_string(chunkNum), destination]
        writeLog("\nsending chunk...");
        string filepath = fileToFilePath[inpt[1]];
        ll chunkNum = stoll(inpt[2]);
        writeLog("filepath: "+ filepath);

        writeLog("sending " + to_string(chunkNum) + " from " + string(peer_ip) + ":" + to_string(peer_port));

        sendChunk(&filepath[0], chunkNum, client_socket);
        
    }
    else if(inpt[0] == "get_file_path"){
        string filepath = fileToFilePath[inpt[1]];
        writeLog("command from peer client: " +  string(inptline));
        write(client_socket, &filepath[0], strlen(filepath.c_str()));
    }
    close(client_socket);
    return;
}

/****************************************************************/
/*Connects to <serverPeerIP:serverPortIP> and sends it <command>*/
/****************************************************************/
string connectToPeer(char* serverPeerIP, char* serverPortIP, string command){
    int peersock = 0;
    struct sockaddr_in peer_serv_addr; 

    writeLog("\nInside connectToPeer");

    if ((peersock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {  
        printf("\n Socket creation error \n"); 
        return "error"; 
    } 
    writeLog("Socket Created");

    peer_serv_addr.sin_family = AF_INET; 
    uint16_t peerPort = stoi(string(serverPortIP));
    peer_serv_addr.sin_port = htons(peerPort); 
    writeLog("\n needs to connect to " + string(serverPeerIP) + ":" + to_string(peerPort));

    if(inet_pton(AF_INET, serverPeerIP, &peer_serv_addr.sin_addr) < 0){ 
        perror("Peer Connection Error(INET)");
    } 
    if (connect(peersock, (struct sockaddr *)&peer_serv_addr, sizeof(peer_serv_addr)) < 0) { 
        perror("Peer Connection Error");
    } 
    writeLog("Connected to peer " + string(serverPeerIP) + ":" + to_string(peerPort));
 
    string curcmd = splitString(command, "$$").front();
    writeLog("current command " + curcmd);

    if(curcmd == "get_chunk_vector"){
        if(send(peersock , &command[0] , strlen(&command[0]) , MSG_NOSIGNAL ) == -1){
            printf("Error: %s\n",strerror(errno));
            return "error"; 
        }
        writeLog("sent command to peer: " + command);
        char server_reply[10240] = {0};
        if(read(peersock, server_reply, 10240) < 0){
            perror("err: ");
            return "error";
        }
        writeLog("got reply: " + string(server_reply));
        close(peersock);
        return string(server_reply);
    }
    else if(curcmd == "get_chunk"){
        //"get_chunk $$ filename $$ to_string(chunkNum) $$ destination
        if(send(peersock , &command[0] , strlen(&command[0]) , MSG_NOSIGNAL ) == -1){
            printf("Error: %s\n",strerror(errno));
            return "error"; 
        }
        writeLog("sent command to peer: " + command);
        vector<string> cmdtokens = splitString(command, "$$");
        
        string despath = cmdtokens[3];
        ll chunkNum = stoll(cmdtokens[2]);
        writeLog("\ngetting chunk " + to_string(chunkNum) + " from "+ string(serverPortIP));

        writeChunk(peersock, chunkNum, &despath[0]);

        return "ss";
    }
    else if(curcmd == "get_file_path"){
        if(send(peersock , &command[0] , strlen(&command[0]) , MSG_NOSIGNAL ) == -1){
            printf("Error: %s\n",strerror(errno));
            return "error"; 
        }
        char server_reply[10240] = {0};
        if(read(peersock, server_reply, 10240) < 0){
            perror("err: ");
            return "error";
        }
        writeLog("server reply for get file path:" + string(server_reply));
        fileToFilePath[splitString(command, "$$").back()] = string(server_reply);
    }

    close(peersock);
    writeLog("terminating connection with " + string(serverPeerIP) + ":" + to_string(peerPort));
    return "aa";
}

/*****************************************************************************/
/* The peer acts as a server, continuously listening for connection requests */
/*****************************************************************************/
void* runAsServer(void* arg){
    int server_socket; 
    struct sockaddr_in address; 
    int addrlen = sizeof(address); 
    int opt = 1; 

    writeLog("\n" + to_string(peer_port) + " will start running as server");
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
    writeLog(" Server socket created.");

    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_port = htons(peer_port); 

    if(inet_pton(AF_INET, &peer_ip[0], &address.sin_addr)<=0)  { 
        printf("\nInvalid address/ Address not supported \n"); 
        return NULL; 
    } 
       
    if (bind(server_socket, (SA *)&address,  sizeof(address))<0) { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    writeLog(" Binding completed.");

    if (listen(server_socket, 3) < 0) { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    writeLog("Listening...\n");

    vector<thread> vThread;
    while(true){

        int client_socket;

        if((client_socket = accept(server_socket, (SA *)&address, (socklen_t *)&addrlen)) < 0){
            perror("Acceptance error");
            writeLog("Error in accept"); 
        }
        writeLog(" Connection Accepted");

        vThread.push_back(thread(handleClientRequest, client_socket));
    }
    for(auto it=vThread.begin(); it!=vThread.end();it++){
        if(it->joinable()) it->join();
    }
    close(server_socket);
}
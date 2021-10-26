#include "client_header.h"

int list_groups(int sock){
    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);

    char reply[3*SIZE];
    memset(reply, 0, sizeof(reply));
    read(sock, reply, 3*SIZE);
    writeLog("list of groups reply: " + string(reply));

    vector<string> grps = splitString(string(reply), "$$");

    for(size_t i=0; i<grps.size()-1; i++){
        cout << grps[i] << endl;
    }
    return 0;
}

int list_requests(int sock){
    writeLog("waiting for response");

    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);
    
    char reply[3*SIZE];
    memset(reply, 0, 3*SIZE);
    read(sock, reply, 3*SIZE);
    if(string(reply) == "**err**") return -1;
    if(string(reply) == "**er2**") return 1;
    writeLog("request list: " + string(reply));

    vector<string> requests = splitString(string(reply), "$$");
    writeLog("list request response size: "+ to_string(requests.size()));
    for(size_t i=0; i<requests.size()-1; i++){
        cout << requests[i] << endl;
    }
    return 0;
}

void accept_request(int sock){
    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);

    char buf[96];
    read(sock, buf, 96);
    cout << buf << endl;
}

void leave_group(int sock){
    writeLog("waiting for response");
    char buf[96];
    read(sock, buf, 96);
    cout << buf << endl;
}

void list_files(int sock){
    char dum[5];
    strcpy(dum, "test");
    write(sock, dum, 5);

    char buf[1024];
    bzero(buf, 1024);
    read(sock, buf, 1024);
    vector<string> listOfFiles = splitString(string(buf), "$$");

    for(auto i: listOfFiles)
        cout << i << endl;
}

void show_downloads(){
    for(auto i: downloadedFiles){
        cout << "[C] " << i.second << " " << i.first << endl;
    }
}

int processCMD(vector<string> inpt, int sock){
    char server_reply[10240]; 
    bzero(server_reply, 10240);
    read(sock , server_reply, 10240); 
    cout << server_reply << endl;
    writeLog("primary server response: " + string(server_reply));
 
    if(string(server_reply) == "Invalid argument count") return 0;
    if(inpt[0] == "login"){
        if(string(server_reply) == "Login Successful"){
            loggedIn = true;
            string peerAddress = peer_ip + ":" + to_string(peer_port);
            write(sock, &peerAddress[0], peerAddress.length());
        }
    }
    else if(inpt[0] == "logout"){
        loggedIn = false;
    }
    else if(inpt[0] == "upload_file"){
        if(string(server_reply) == "Error 101:"){
            cout << "Group doesn't exist" << endl;
            return 0;
        }
        else  if(string(server_reply) == "Error 102:"){
            cout << "You are not a member of this group" << endl;
            return 0;
        }
        else  if(string(server_reply) == "Error 103:"){
            cout << "File not found." << endl;
            return 0;
        }
        return uploadFile(inpt, sock);
    }
    else if(inpt[0] == "download_file"){
        if(string(server_reply) == "Error 101:"){
            cout << "Group doesn't exist" << endl;
            return 0;
        }
        else  if(string(server_reply) == "Error 102:"){
            cout << "You are not a member of this group" << endl;
            return 0;
        }
        else  if(string(server_reply) == "Error 103:"){
            cout << "Directory not found" << endl;
            return 0;
        }
        if(downloadedFiles.find(inpt[2])!= downloadedFiles.end()){
            cout << "File already downloaded" << endl;
            return 0;
        }
        return downloadFile(inpt, sock);
    }
    else if(inpt[0] == "list_groups"){
        return list_groups(sock);
    }
    else if(inpt[0] == "list_requests"){
        int t;
        if((t = list_requests(sock)) < 0){
            cout << "You are not the admin of this group\n";
        }
        else if(t>0){
            cout << "No pending requests\n";
        }
        else return 0;
    }
    else if(inpt[0] == "accept_request"){
        accept_request(sock);
    }
    else if(inpt[0] == "leave_group"){
        leave_group(sock);
    }
    else if(inpt[0] == "list_files"){
        list_files(sock);
    }
    else if(inpt[0] == "stop_share"){
        isUploaded[inpt[1]].erase(inpt[2]);
    }
    else if(inpt[0] == "show_downloads"){
        show_downloads();
    }
    return 0;
}
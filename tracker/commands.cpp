#include "tracker_header.h"

int createUser(vector<string> inpt){
    string user_id = inpt[1];
    string passwd = inpt[2];

    if(loginCreds.find(user_id) == loginCreds.end()){
        loginCreds.insert({user_id, passwd});
    }
    else{
        return -1;
    }
    return 0;
}

int validateLogin(vector<string> inpt){
    string user_id = inpt[1];
    string passwd = inpt[2];

    if(loginCreds.find(user_id) == loginCreds.end() || loginCreds[user_id] != passwd){
        return -1;
    }

    if(isLoggedIn.find(user_id) == isLoggedIn.end()){
        isLoggedIn.insert({user_id, true});
    }
    else{
        if(isLoggedIn[user_id]){
            return 1;
        }
        else{
            isLoggedIn[user_id] = true;
        }
    }
    return 0;
}

void uploadFile(vector<string> inpt, int client_socket, string client_uid){
    //inpt - upload_file​ <file_path> <group_id​>
    if(inpt.size() != 3){
        write(client_socket, "Invalid argument count", 22);
    }
    else if(groupMembers.find(inpt[2]) == groupMembers.end()){
        write(client_socket, "Error 101:", 10);
    }
    else if(groupMembers[inpt[2]].find(client_uid) == groupMembers[inpt[2]].end()){
        write(client_socket, "Error 102:", 10);
    }
    else if(!pathExists(inpt[1])){
        write(client_socket, "Error 103:", 10);
    }
    else{
        char fileDetails[524288] =  {0};
        write(client_socket, "Uploading...", 12);
        writeLog("uploading");

        if(read(client_socket , fileDetails, 524288)){
            if(string(fileDetails) == "error") return;

            vector<string> fdet = splitString(string(fileDetails), "$$");
            //fdet = [filepath, peer address, file size, file hash, piecewise hash] 
            string filename = splitString(string(fdet[0]), "/").back();

            string hashOfPieces = "";
            for(size_t i=4; i<fdet.size(); i++){
                hashOfPieces += fdet[i];
                if(i != fdet.size()-1) hashOfPieces += "$$";
            }
            
            piecewiseHash[filename] = hashOfPieces;
            
            if(seederList[inpt[2]].find(filename) != seederList[inpt[2]].end()){
                seederList[inpt[2]][filename].insert(client_uid);
            }
            else{
                seederList[inpt[2]].insert({filename, {client_uid}});
            }
            fileSize[filename] = fdet[2];
            
            write(client_socket, "Uploaded", 8);
        }
    }
}

void downloadFile(vector<string> inpt, int client_socket, string client_uid){
    // inpt - download_file​ <group_id> <file_name> <destination_path>
    if(inpt.size() != 4){
        write(client_socket, "Invalid argument count", 22);
    }
    else if(groupMembers.find(inpt[1]) == groupMembers.end()){
        write(client_socket, "Error 101:", 10);
    }
    else if(groupMembers[inpt[1]].find(client_uid) == groupMembers[inpt[1]].end()){
        write(client_socket, "Error 102:", 10);
    }
    else{
        if(!pathExists(inpt[3])){
            write(client_socket, "Error 103:", 10);
            return;
        }

        char fileDetails[524288] =  {0};
        // fileDetails = [filename, destination, group id]
        write(client_socket, "Downloading...", 13);

        if(read(client_socket , fileDetails, 524288)){
            vector<string> fdet = splitString(string(fileDetails), "$$");
            
            string reply = "";
            if(seederList[inpt[1]].find(fdet[0]) != seederList[inpt[1]].end()){
                for(auto i: seederList[inpt[1]][fdet[0]]){
                    if(isLoggedIn[i]){
                        reply += unameToPort[i] + "$$";
                    }
                }
                reply += fileSize[fdet[0]];
                writeLog("seeder list: "+ reply);
                write(client_socket, &reply[0], reply.length());

                char dum[5];
                read(client_socket, dum, 5);
                
                write(client_socket, &piecewiseHash[fdet[0]][0], piecewiseHash[fdet[0]].length());
            
                seederList[inpt[1]][inpt[2]].insert(client_uid);
            }
            else{
                write(client_socket, "File not found", 14);
            }
        }
        
    }
}

int create_group(vector<string> inpt, int client_socket, string client_uid){
    //inpt - [create_group gid] 
    if(inpt.size() != 2){
        write(client_socket, "Invalid argument count", 22);
        return -1;
    }
    for(auto i: allGroups){
        if(i == inpt[1]) return -1;
    }
    grpAdmins.insert({inpt[1], client_uid});
    allGroups.push_back(inpt[1]);
    groupMembers[inpt[1]].insert(client_uid);
    return 0;
}

void list_groups(vector<string> inpt, int client_socket){
    //inpt - [list_groups];
    if(inpt.size() != 1){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "All groups:", 11);

    char dum[5];
    read(client_socket, dum, 5);

    if(allGroups.size() == 0){
        write(client_socket, "No groups found$$", 18);
        return;
    }

    string reply = "";
    for(size_t i=0; i<allGroups.size(); i++){
        reply += allGroups[i] + "$$";
    }
    write(client_socket, &reply[0], reply.length());
}

void join_group(vector<string> inpt, int client_socket, string client_uid){
    //inpt - [join_group gid]
    if(inpt.size() != 2){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    writeLog("join_group function ..");

    if(grpAdmins.find(inpt[1]) == grpAdmins.end()){
        write(client_socket, "Invalid group ID.", 19);
    }
    else if(groupMembers[inpt[1]].find(client_uid) == groupMembers[inpt[1]].end()){
        grpPendngRequests[inpt[1]].insert(client_uid);
        write(client_socket, "Group request sent", 18);
    }
    else{
        write(client_socket, "You are already in this group", 30);
    }
    
}

void list_requests(vector<string> inpt, int client_socket, string client_uid){
    // inpt - [list_requests groupid]
    if(inpt.size() != 2){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "Fetching group requests...", 27);

    char dum[5];
    read(client_socket, dum, 5);

    writeLog("hereeee");
    if(grpAdmins.find(inpt[1])==grpAdmins.end() || grpAdmins[inpt[1]] != client_uid){
        writeLog("iffff");
        write(client_socket, "**err**", 7);
    }
    else if(grpPendngRequests[inpt[1]].size() == 0){
        write(client_socket, "**er2**", 7);
    }
    else {
        string reply = "";
        writeLog("pending request size: "+  to_string(grpPendngRequests[inpt[1]].size()));
        for(auto i = grpPendngRequests[inpt[1]].begin(); i!= grpPendngRequests[inpt[1]].end(); i++){
            reply += string(*i) + "$$";
        }
        write(client_socket, &reply[0], reply.length());
        writeLog("reply :" + reply);
    }
}

void accept_request(vector<string> inpt, int client_socket, string client_uid){
    // inpt - [accept_request groupid user_id]
    if(inpt.size() != 3){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "Accepting request...", 21);

    char dum[5];
    read(client_socket, dum, 5);

    if(grpAdmins.find(inpt[1]) == grpAdmins.end()){
        writeLog("inside accept_request if");
        write(client_socket, "Invalid group ID.", 19);
    }
    else if(grpAdmins.find(inpt[1])->second == client_uid){
        writeLog("inside accept_request else if with pending list:");
        for(auto i: grpPendngRequests[inpt[1]]){
            writeLog(i);
        }
        grpPendngRequests[inpt[1]].erase(inpt[2]);
        groupMembers[inpt[1]].insert(inpt[2]);
        write(client_socket, "Request accepted.", 18);
    }
    else{
        writeLog("inside accept_request else");
        //cout << grpAdmins.find(inpt[1])->second << " " << client_uid <<  endl;
        write(client_socket, "You are not the admin of this group", 35);
    }
    
}

void leave_group(vector<string> inpt, int client_socket, string client_uid){
    // inpt - [leave_group groupid]
    if(inpt.size() != 2){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "Leaving group...", 17);

    if(grpAdmins.find(inpt[1]) == grpAdmins.end()){
        write(client_socket, "Invalid group ID.", 19);
    }
    else if(groupMembers[inpt[1]].find(client_uid) != groupMembers[inpt[1]].end()){
        if(grpAdmins[inpt[1]] == client_uid){
            write(client_socket, "You are the admin of this group, you cant leave!", 48);
        }
        else{
            groupMembers[inpt[1]].erase(client_uid);
            write(client_socket, "Group left succesfully", 23);
        }
    }
    else{
        write(client_socket, "You are not in this group", 25);
    }
}

void list_files(vector<string> inpt, int client_socket){
    // inpt - list_files​ <group_id>
    if(inpt.size() != 2){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    write(client_socket, "Fetching files...", 17);

    char dum[5];
    read(client_socket, dum, 5);
    writeLog("dum read");

    if(grpAdmins.find(inpt[1]) == grpAdmins.end()){
        write(client_socket, "Invalid group ID.", 19);
    }
    else if(seederList[inpt[1]].size() == 0){
        write(client_socket, "No files found.", 15);
    }
    else{
        writeLog("in else of list files");

        string reply = "";

        for(auto i: seederList[inpt[1]]){
            reply += i.first + "$$";
        }
        reply = reply.substr(0, reply.length()-2);
        writeLog("list of files reply:" + reply);

        write(client_socket, &reply[0], reply.length());
    }
}

void stop_share(vector<string> inpt, int client_socket, string client_uid){
    // inpt - stop_share ​<group_id> <file_name>
    if(inpt.size() != 3){
        write(client_socket, "Invalid argument count", 22);
        return;
    }
    if(grpAdmins.find(inpt[1]) == grpAdmins.end()){
        write(client_socket, "Invalid group ID.", 19);
    }
    else if(seederList[inpt[1]].find(inpt[2]) == seederList[inpt[1]].end()){
        write(client_socket, "File not yet shared in the group", 32);
    }
    else{
        seederList[inpt[1]][inpt[2]].erase(client_uid);
        if(seederList[inpt[1]][inpt[2]].size() == 0){
            seederList[inpt[1]].erase(inpt[2]);
        }
        write(client_socket, "Stopped sharing the file", 25);
    }
}

//client connection handling thread
void handle_connection(int client_socket){
    string client_uid = "";
    string client_gid = "";
    writeLog("***********pthread started for client socket number " + to_string(client_socket));

    //for continuously checking the commands sent by the client
    while(true){
        char inptline[1024] = {0}; 

        if(read(client_socket , inptline, 1024) <=0){
            isLoggedIn[client_uid] = false;
            close(client_socket);
            break;
        }
        writeLog("client request:" + string(inptline));

        string s, in = string(inptline);
        stringstream ss(in);
        vector<string> inpt;

        while(ss >> s){
            inpt.push_back(s);
        }

        if(inpt[0] == "create_user"){
            if(inpt.size() != 3){
                write(client_socket, "Invalid argument count", 22);
            }
            else{
                if(createUser(inpt) < 0){
                    write(client_socket, "User exists", 11);
                }
                else{
                    write(client_socket, "Account created", 15);
                }
            }
        }
        else if(inpt[0] == "login"){
            if(inpt.size() != 3){
                write(client_socket, "Invalid argument count", 22);
            }
            else{
                int r;
                if((r = validateLogin(inpt)) < 0){
                    write(client_socket, "Username/password incorrect", 28);
                }
                else if(r > 0){
                    write(client_socket, "You already have one active session", 35);
                }
                else{
                    write(client_socket, "Login Successful", 16);
                    client_uid = inpt[1];
                    char buf[96];
                    read(client_socket, buf, 96);
                    string peerAddress = string(buf);
                    unameToPort[client_uid] = peerAddress;
                }
            }            
        }
        else if(inpt[0] ==  "logout"){
            isLoggedIn[client_uid] = false;
            write(client_socket, "Logout Successful", 17);
            writeLog("logout sucess\n");
        }
        else if(inpt[0] == "upload_file"){
            uploadFile(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "download_file"){
            downloadFile(inpt, client_socket, client_uid);
            writeLog("after down");
        }
        else if(inpt[0] == "create_group"){
            if(create_group(inpt, client_socket, client_uid) >=0){
                client_gid = inpt[1];
                write(client_socket, "Group created", 13);
            }
            else{
                write(client_socket, "Group exists", 12);
            }
        }
        else if(inpt[0] == "list_groups"){
            list_groups(inpt, client_socket);
        }
        else if(inpt[0] == "join_group"){
            join_group(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "list_requests"){
            list_requests(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "accept_request"){
            accept_request(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "leave_group"){
            leave_group(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "list_files"){
            list_files(inpt, client_socket);
        }
        else if(inpt[0] == "stop_share"){
            stop_share(inpt, client_socket, client_uid);
        }
        else if(inpt[0] == "show_downloads"){
            write(client_socket, "Loading...", 10);
        }
        else{
            write(client_socket, "Invalid command", 16);
        }
    }
    writeLog("***********pthread ended for client socket number " + to_string(client_socket));
    close(client_socket);
}
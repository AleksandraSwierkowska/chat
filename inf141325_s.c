//server
#include <stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include <stddef.h>
#include "structures.h"
#include<sys/shm.h>
#include<sys/sem.h>


user createUser(const char* userData, int k);
group createGroup(const char* groupData, int k);
void createStructures(user*, group*, int* n, int* m);
void sendMessage(user*, int ipcMSG, int n);
void sendMessageToGroup(group*, int ipcMSG, int m);
int log_in(user*, group*, int n);
int ipc;
struct sembuf sOpen = {0, 1, 0};
struct sembuf sClose = {0, -1, 0};


int main() {
    user *users;
    int shmidusers = shmget(1200, 30 * sizeof(user), 0666 | IPC_CREAT);
    users = (user *) shmat(shmidusers, 0, 0);
    int susers = semget(1300, 1, 0666 | IPC_CREAT);
    group *groups;
    int shmidgroups = shmget(1201, 10 * sizeof(group), 0666 | IPC_CREAT);
    groups = (group *) shmat(shmidgroups, 0, 0);
    int sgroups = semget(1301, 1, 0666 | IPC_CREAT);

    semop(susers, &sOpen, 1);
    semop(sgroups, &sOpen, 1);

    int f = 100;
    int n = 0; //user number
    int m = 0; //group number
    ipc = msgget(1024, 0666 | IPC_CREAT);
    createStructures(users, groups, &n, &m);


    while(1){
        int userID = log_in(users, groups, n);
        if(userID > 0){
            f = fork();
        }
        if (f == 0){
            int ipcMSG = msgget(1024 + userID, 0666 | IPC_CREAT);
            int ipcGet = msgget(1024 + 2*userID, 0666 | IPC_CREAT);
            clientRequest clientRequest1 = {};
            while(1) {
                msgrcv(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 5, 0);
                switch (clientRequest1.request){
                    case 1: //send to user
                        sendMessage(users, ipcMSG, n);
                        break;
                    case 2: //send to group
                        sendMessageToGroup(groups, ipcMSG, m);
                        break;
                    case 3: //show groups
                        break;
                    case 4: //show active users
                        break;
                    case 5: //join a group
                        break;
                    case 6: //leave a group
                        break;
                    case 7: //block a user or a group
                        break;
                }
            }
        }
    }





}
void createStructures(user* users, group* groups, int* n, int* m){
    char buf[1];
    char line[512];
    int k = 0;
    int fd = open("configuration_file", O_RDWR);
    if (fd == -1) {
        perror("The file cannot be open");
        exit(1);
    }
    //get the number of users
    while(read(fd, buf, 1)>0){
        if (buf[0]!='\n'){
            *n = *n*10 + buf[0] - '0';
        }
        else{
            break;
        }
    }

    for (int i = 0; i<*n; i++){
        while(read(fd, buf, 1)>0) {
            if (buf[0] != '\n') {
                line[k] = buf[0];
                k = k + 1;
            } else {
                users[i] = createUser(line, k);
                k = 0;
                memset(line, 0, 512);
                break;
            }
        }
    }

    while(read(fd, buf, 1)>0){
        if (buf[0]!='\n'){
            *m = *m*10 + buf[0] - '0';
        }
        else{
            break;
        }
    }


    for (int i = 0; i<*m; i++){
        while(read(fd, buf, 1)>0) {
            if (buf[0] != '\n') {
                line[k] = buf[0];
                k = k + 1;
            } else {
                groups[i] = createGroup(line, k);
                k = 0;
                memset(line, 0, 512);
                break;
            }

        }

    }
}

user createUser(const char* userData, int k){
    static user user;
    memset(user.login, 0, 30);
    memset(user.password, 0, 30);
    int iter = 0;
    int what = 0;
    user.active = 0;
    user.id = 0;
    user.loggingTries = 0;
    for (int i = 0; i < k; i ++){

        if (userData[i] == ' '){
            what = what + 1;
            iter = 0;
        }
        else{
            if (what == 0){
                user.login[iter] = userData[i];
                iter = iter + 1;
            }
            else if (what == 1){
                user.password[iter] = userData[i];
                iter = iter + 1;
            }
            else{
                user.id = user.id*10 + userData[i] - '0';
                iter = iter + 1;
            }
        }

    }

    return user;
}

group createGroup(const char* groupData, int k){
    static group group;
    memset(group.listOfUsers, 0, 25);
    memset(group.name, 0, 30);
    int iter = 0;
    int what = 0;
    group.id = 0;
    for (int i = 0; i < k; i ++){
        if (groupData[i] == ' '){
            if (what == 0 || what == 1) {
                iter = 0;
            }
            else{
                iter = iter + 1;
            }
            what = what + 1;
        }
        else{
            if (what == 0){
                group.name[iter] = groupData[i];
                iter = iter + 1;
            }
            else if (what == 1){
                group.id = group.id*10 + groupData[i] - '0';
            }
            else{
                group.listOfUsers[iter] = group.listOfUsers[iter]*10 + groupData[i] - '0';

            }
        }
    }
    return group;
}

int log_in(user* users, group* group, int n){
    send_login get_login = {};
    send_password get_password = {};
    id send_id = {4};
    int id = 0;
    while(1){
        msgrcv(ipc, &get_login, sizeof(send_login) - sizeof(long), 1, 0); //1 as mtype
        printf("%s\n", get_login.login);
        for (int i = 0; i < n; i++){
            if  (strcmp(users[i].login, get_login.login) == 0){
                id = users[i].id;
            }
        }
        if (id == 0){
            error error = {3, 1};
            msgsnd(ipc, &error, sizeof(error)-sizeof(long), 0);
            continue;
        }
        error error = {3, 0};
        msgsnd(ipc, &error, sizeof(error)-sizeof(long), 0);
        msgrcv(ipc, &get_password, sizeof(send_login) - sizeof(long), 2, 0);
        printf("%s\n", get_password.password);
        if (users[id-1].loggingTries > 1){
            error.error = 3;
            msgsnd(ipc, &error, sizeof(error)-sizeof(long), 0);
            continue;
        }
        else{
            if (strcmp(users[id-1].password,get_password.password)==0){
                users[id-1].active = 1;
                error.error = 0;
                msgsnd(ipc, &error, sizeof(error)-sizeof(long), 0);
                send_id.id = id;
                msgsnd(ipc, &send_id, sizeof(send_id)-sizeof(long), 0);
                break;
            }
            else{
                users[id-1].loggingTries++;
                error.error = 2;
                msgsnd(ipc, &error, sizeof(error)-sizeof(long), 0);

            }
        }
    }
    return id;
}

void sendMessage(user* users, int ipcMSG, int n){
    msg msg1 = {};
    error error1 = {};
    int id = 0;
    int alright = 1;
    memset(msg1.msg, 0, 248);
    memset(msg1.receiver, 0, 30);
    msgrcv(ipcMSG, &msg1, sizeof(msg1) - sizeof(long), 6, 0);
    for (int i = 0; i < n; i++){
        if (strcmp(users[i].login, msg1.receiver) == 0){
            id = users[i].id;
        }
    }
    if (users[id-1].active == 0){
        error1.mtype = 3;
        error1.error = 4;
        msgsnd(ipcMSG, &error1, sizeof(error1) - sizeof(long), 0);
        alright = 0;
    }
    else{
        for (int i = 0; i < sizeof(users[id-1].blocked)/sizeof(users[id-1].blocked[0]); i++){
            if (users[id-1].blocked[i] == ipcMSG - 1024){
                error1.mtype = 3;
                error1.error = 5;
                msgsnd(ipcMSG, &error1, sizeof(error1) - sizeof(long), 0);
                alright = 0;
                break;
            }
        }
    }
    if (alright){
        //strcpy(msg1.receiver, users[ipcMSG - 1025].login);
        printf("%s", "WYSYŁAM");
        msgsnd(1024 + 2*id, &msg1, sizeof(msg1) - sizeof(long), 0);
        error1.mtype = 3;
        error1.error = 0;
        msgsnd(ipcMSG, &error1, sizeof(error1) - sizeof(long), 0);
    }
}

void sendMessageToGroup(group* groups, int ipcMSG, int m){

}
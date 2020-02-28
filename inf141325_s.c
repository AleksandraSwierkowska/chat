//server
#include <stdio.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include "structures.h"
#include<sys/shm.h>
#include<sys/sem.h>
#include <signal.h>


user createUser(const char *userData, int k);

group createGroup(const char *groupData, int k);

void createStructures(user *, group *, int *n, int *m);

void sendMessage(user *, int ipcMSG, int n);

void sendMessageToGroup(group *, user *, int ipcMSG, int m, int n);

void showGroups(group *, int ipcMSG, int m);

void showUsersInGroup(group *, user *, int ipcMSG, int m);

void showActiveUsers(user *, int ipcMSG, int n);

void joinGroup(group *, int ipcMSG, int m);

void leaveGroup(group *, int ipcMSG, int m);

void blockUser(user *, int ipcMSG, int n);

void blockGroup(group *, user *, int ipcMSG, int m, int n);

int log_in(user *, group *, int n);

void log_out(int ipcMsg, int ipcGet);

void changeSignal();

int ipc;
int shmidusers;
int susers;
int shmidgroups;
int sgroups;
struct sembuf sOpen = {0, 1, 0};
struct sembuf sClose = {0, -1, 0};


int main() {
    signal(SIGINT, changeSignal);
    user *users;
    shmidusers = shmget(1200, 30 * sizeof(user), 0666 | IPC_CREAT);
    users = (user *) shmat(shmidusers, 0, 0);
    susers = semget(1300, 1, 0600 | IPC_CREAT);
    group *groups;
    shmidgroups = shmget(1201, 10 * sizeof(group), 0666 | IPC_CREAT);
    groups = (group *) shmat(shmidgroups, 0, 0);
    sgroups = semget(1301, 1, 0600 | IPC_CREAT);

    semop(susers, &sOpen, 1);
    semop(sgroups, &sOpen, 1);

    int f = 100;
    int n = 0; //user number
    int m = 0; //group number
    ipc = msgget(1024, 0600 | IPC_CREAT);
    createStructures(users, groups, &n, &m);


    while (1) {
        int userID = log_in(users, groups, n);
        if (userID > 0) {
            f = fork();
        }
        if (f == 0) {
            signal(SIGINT, SIG_DFL);
            int ipcMSG = msgget(1024 + 2 * userID, 0600 | IPC_CREAT);
            int ipcGet = msgget(1025 + 2 * userID, 0600 | IPC_CREAT);
            clientRequest clientRequest1 = {};
            while (1) {
                msgrcv(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 5, 0);
                switch (clientRequest1.request) {
                    case 1: //send to user
                        sendMessage(users, ipcMSG, n);
                        break;
                    case 2: //send to group
                        sendMessageToGroup(groups, users, ipcMSG, m, n);
                        break;
                    case 3: //show groups
                        showGroups(groups, ipcMSG, m);
                        break;
                    case 4: //show users in the given group
                        showUsersInGroup(groups, users, ipcMSG, m);
                        break;
                    case 5: //show active users
                        showActiveUsers(users, ipcMSG, n);
                        break;
                    case 6: //join a group
                        joinGroup(groups, ipcMSG, m);
                        break;
                    case 7: //leave a group
                        leaveGroup(groups, ipcMSG, m);
                        break;
                    case 8: //block a user
                        blockUser(users, ipcMSG, n);
                        break;
                    case 9: //block a group
                        blockGroup(groups, users, ipcMSG, m, n);
                        break;
                    case 10:
                        log_out(ipcMSG, ipcGet);
                        break;
                }
            }
        }
    }


}

void createStructures(user *users, group *groups, int *n, int *m) {
    char buf[1];
    char line[512];
    int k = 0;
    int fd = open("configuration_file", O_RDWR);
    if (fd == -1) {
        perror("The file cannot be open");
        exit(1);
    }
    //get the number of users
    while (read(fd, buf, 1) > 0) {
        if (buf[0] != '\n') {
            *n = *n * 10 + buf[0] - '0';
        } else {
            break;
        }
    }

    for (int i = 0; i < *n; i++) {
        while (read(fd, buf, 1) > 0) {
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

    while (read(fd, buf, 1) > 0) {
        if (buf[0] != '\n') {
            *m = *m * 10 + buf[0] - '0';
        } else {
            break;
        }
    }


    for (int i = 0; i < *m; i++) {
        while (read(fd, buf, 1) > 0) {
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

user createUser(const char *userData, int k) {
    static user user;
    memset(user.login, 0, 30);
    memset(user.password, 0, 30);
    int iter = 0;
    int what = 0;
    user.active = 0;
    user.id = 0;
    user.loggingTries = 0;
    user.nrBlocked = 0;
    user.nrBlockedGroups = 0;
    for (int i = 0; i < k; i++) {

        if (userData[i] == ' ') {
            what = what + 1;
            iter = 0;
        } else {
            if (what == 0) {
                user.login[iter] = userData[i];
                iter = iter + 1;
            } else if (what == 1) {
                user.password[iter] = userData[i];
                iter = iter + 1;
            } else {
                user.id = user.id * 10 + userData[i] - '0';
                iter = iter + 1;
            }
        }

    }

    return user;
}

group createGroup(const char *groupData, int k) {
    group group;
    memset(group.name, 0, 30);
    memset(group.listOfUsers, 0, 25 * sizeof(int));
    int iter = 0;
    int what = 0;
    group.id = 0;
    group.nrOfUsers = -1;
    for (int i = 0; i < k; i++) {
        if (groupData[i] == ' ') {
            group.nrOfUsers++;
            if (what == 0 || what == 1) {
                iter = 0;
            } else {
                iter = iter + 1;
            }
            what = what + 1;
        } else {
            if (what == 0) {
                group.name[iter] = groupData[i];
                iter = iter + 1;
            } else if (what == 1) {
                group.id = group.id * 10 + groupData[i] - '0';
            } else {
                group.listOfUsers[iter] = group.listOfUsers[iter] * 10 + groupData[i] - '0';

            }
        }
    }
    return group;
}

int log_in(user *users, group *group, int n) {
    send_login get_login = {};
    send_password get_password = {};
    id send_id = {4};
    int userNotLogged;
    int id = 0;
    while (1) {
        userNotLogged = 1;
        msgrcv(ipc, &get_login, sizeof(send_login) - sizeof(long), 1, 0);
        printf("%s\n", get_login.login);
        for (int i = 0; i < n; i++) {
            if (strcmp(users[i].login, get_login.login) == 0) {
                if (users[i].active == 1){
                    error error = {3, 4};
                    msgsnd(ipc, &error, sizeof(error) - sizeof(long), 0);
                    userNotLogged = 0;
                    break;
                }
                else {
                    id = users[i].id;
                }
            }
        }
        if(userNotLogged) {
            if (id == 0) {
                error error = {3, 1};
                msgsnd(ipc, &error, sizeof(error) - sizeof(long), 0);
                continue;
            }
            error error = {3, 0};
            msgsnd(ipc, &error, sizeof(error) - sizeof(long), 0);
            msgrcv(ipc, &get_password, sizeof(send_login) - sizeof(long), 2, 0);
            printf("%s\n", get_password.password);
            semop(susers, &sClose, 1);
            if (users[id - 1].loggingTries > 1) {
                error.error = 3;
                msgsnd(ipc, &error, sizeof(error) - sizeof(long), 0);
                semop(susers, &sOpen, 1);
                continue;
            } else {
                if (strcmp(users[id - 1].password, get_password.password) == 0) {
                    users[id - 1].active = 1;
                    error.error = 0;
                    msgsnd(ipc, &error, sizeof(error) - sizeof(long), 0);
                    send_id.id = id;
                    msgsnd(ipc, &send_id, sizeof(send_id) - sizeof(long), 0);
                    semop(susers, &sOpen, 1);
                    break;
                } else {
                    users[id - 1].loggingTries++;
                    error.error = 2;
                    msgsnd(ipc, &error, sizeof(error) - sizeof(long), 0);

                }
            }
            semop(susers, &sOpen, 1);
        }
    }
    return id;
}

void sendMessage(user *users, int ipcMSG, int n) {
    msg msg1 = {};
    error error1 = {};
    error1.mtype = 3;
    int id = 0;
    int alright = 1;
    memset(msg1.msg, 0, 2048);
    memset(msg1.receiver, 0, 30);
    msgrcv(ipcMSG, &msg1, sizeof(msg1) - sizeof(long), 6, 0);
    for (int i = 0; i < n; i++) {
        if (strcmp(users[i].login, msg1.receiver) == 0) {
            id = users[i].id;
        }
    }
    semop(susers, &sClose, 1);
    if (users[id - 1].active == 0) {
        error1.error = 4;
        msgsnd(ipcMSG, &error1, sizeof(error1) - sizeof(long), 0);
        alright = 0;
    } else {
        for (int i = 0; i < users[id - 1].nrBlocked; i++) {
            if (users[id - 1].blocked[i] == msg1.sender) {
                error1.error = 5;
                msgsnd(ipcMSG, &error1, sizeof(error1) - sizeof(long), 0);
                alright = 0;
                break;
            }
        }
    }
    if (alright) {
        memset(msg1.receiver, 0, 30);
        strcpy(msg1.receiver, users[msg1.sender - 1].login);
        int sendThere = msgget(1025 + 2 * id, 0666);
        msgsnd(sendThere, &msg1, sizeof(msg1) - sizeof(long), 0);
        error1.error = 0;
        msgsnd(ipcMSG, &error1, sizeof(error1) - sizeof(long), 0);
    }
    semop(susers, &sOpen, 1);
}

void sendMessageToGroup(group *groups, user *users, int ipcMSG, int m, int n) {
    msg msg1 = {};
    int id = 0;
    int alright = 1;
    memset(msg1.msg, 0, 2048);
    memset(msg1.receiver, 0, 30);
    msgrcv(ipcMSG, &msg1, sizeof(msg1) - sizeof(long), 6, 0);
    for (int i = 0; i < m; i++) {
        if (strcmp(groups[i].name, msg1.receiver) == 0) {
            id = groups[i].id;
        }
    }
    semop(sgroups, &sClose, 1);
    semop(susers, &sClose, 1);
    for (int i = 0; i < groups[id - 1].nrOfUsers; i++) {
        int receiverID = groups[id - 1].listOfUsers[i];
        if (users[receiverID - 1].active == 0) {
            alright = 0;
        } else {
            for (int j = 0; j < users[receiverID - 1].nrBlockedGroups; j++) {
                if (users[receiverID - 1].blockedGroups[j] == id) {
                    alright = 0;
                    break;
                }
            }
        }
        if (alright) {
            memset(msg1.receiver, 0, 30);
            strcpy(msg1.receiver, users[msg1.sender - 1].login);
            int sendThere = msgget(1025 + 2 * receiverID, 0666);
            msgsnd(sendThere, &msg1, sizeof(msg1) - sizeof(long), 0);
        }
        alright = 1;
    }
    semop(susers, &sOpen, 1);
    semop(sgroups, &sOpen, 1);

}

void showGroups(group *groups, int ipcMSG, int m) {
    sysMSG sysMsg = {};
    for (int i = 0; i < m; i++) {
        strcpy(sysMsg.all[i], groups[i].name);
    }
    sysMsg.nr = m;
    sysMsg.mtype = 7;
    msgsnd(ipcMSG, &sysMsg, sizeof(sysMsg) - sizeof(long), 0);
}

void showUsersInGroup(group *groups, user *users, int ipcMSG, int m) {
    sysMSG sysMsg = {};
    text text1 = {};
    error error1 = {3, 1};
    int id = 0;
    msgrcv(ipcMSG, &text1, sizeof(text1) - sizeof(long), 8, 0);
    for (int i = 0; i < m; i++) {
        if (strcmp(text1.name, groups[i].name) == 0) {
            error1.error = 0;
            id = groups[i].id;

        }
    }
    msgsnd(ipcMSG, &error1, sizeof(error1) - sizeof(long), 0);
    sysMsg.nr = 0;
    semop(sgroups, &sClose, 1);
    if (id != 0) {
        for (int i = 0; i < groups[id - 1].nrOfUsers; i++) {
            strcpy(sysMsg.all[i], users[groups[id - 1].listOfUsers[i] - 1].login);
            sysMsg.nr++;

        }
    }

    sysMsg.mtype = 7;
    msgsnd(ipcMSG, &sysMsg, sizeof(sysMsg) - sizeof(long), 0);
    semop(sgroups, &sOpen, 1);
}

void showActiveUsers(user *users, int ipcMSG, int n) {
    sysMSG sysMsg = {};
    sysMsg.nr = 0;
    sysMsg.mtype = 7;
    semop(susers, &sClose, 1);
    for (int i = 0; i < n; i++) {
        if (users[i].active == 1) {
            strcpy(sysMsg.all[sysMsg.nr], users[i].login);
            sysMsg.nr++;
        }
    }
    msgsnd(ipcMSG, &sysMsg, sizeof(sysMsg) - sizeof(long), 0);
    semop(susers, &sOpen, 1);
}

void joinGroup(group *groups, int ipcMSG, int m) {
    id id1 = {};
    text text1 = {};
    error error1 = {3, 1};
    msgrcv(ipcMSG, &text1, sizeof(text1) - sizeof(long), 8, 0);
    msgrcv(ipcMSG, &id1, sizeof(id1) - sizeof(long), 4, 0);
    semop(sgroups, &sClose, 1);
    for (int i = 0; i < m; i++) {
        if (strcmp(text1.name, groups[i].name) == 0) {
            error1.error = 0;
            for (int k = 0; k < groups[i].nrOfUsers; k++) {
                if (groups[i].listOfUsers[k] == id1.id) {
                    error1.error = 2;
                    break;
                }

            }
            if (error1.error == 0) {
                groups[i].listOfUsers[groups[i].nrOfUsers] = id1.id;
                groups[i].nrOfUsers++;
            }
            break;
        }
    }
    msgsnd(ipcMSG, &error1, sizeof(error1) - sizeof(long), 0);
    semop(sgroups, &sOpen, 1);
}

void leaveGroup(group *groups, int ipcMSG, int m) {
    int t = 0;
    id id1 = {};
    text text1 = {};
    int newList[25];
    error error1 = {3, 2};
    msgrcv(ipcMSG, &text1, sizeof(text1) - sizeof(long), 8, 0);
    msgrcv(ipcMSG, &id1, sizeof(id1) - sizeof(long), 4, 0);
    semop(sgroups, &sClose, 1);
    for (int i = 0; i < m; i++) {
        if (strcmp(text1.name, groups[i].name) == 0) {
            error1.error--;
            for (int k = 0; k < groups[i].nrOfUsers; k++) {
                if (groups[i].listOfUsers[k] == id1.id) {
                    error1.error--;
                } else {
                    newList[t] = groups[i].listOfUsers[k];
                    t++;
                }
            }
            for (int j = 0; j < t; j++) {
                groups[i].listOfUsers[j] = newList[j];
            }
            groups[i].listOfUsers[groups[i].nrOfUsers - 1] = 0;
            groups[i].nrOfUsers--;
            break;
        }
    }
    msgsnd(ipcMSG, &error1, sizeof(error1) - sizeof(long), 0);
    semop(sgroups, &sOpen, 1);
}

void blockUser(user *users, int ipcMSG, int n) {
    int idToBlock = 0;
    id id1 = {};
    text text1 = {};
    int nrUser = 0;
    error error1 = {3, 1};
    msgrcv(ipcMSG, &id1, sizeof(id1) - sizeof(long), 4, 0);
    msgrcv(ipcMSG, &text1, sizeof(text1) - sizeof(long), 8, 0);

    for (int i = 0; i < n; i++) {
        if (strcmp(users[i].login, text1.name) == 0) {
            idToBlock = users[i].id;
            error1.error--;
            break;
        }
    }

    if (id1.id == idToBlock){
        error1.error = 3;
        msgsnd(ipcMSG, &error1, sizeof(error1) - sizeof(long), 0);
        return;
    }

    semop(susers, &sClose, 1);
    for (int i = 0; i < n; i++) {
        if (users[i].id == id1.id) {
            nrUser = i;
            for (int j = 0; j < users[i].nrBlocked; j++) {
                if (users[i].blocked[j] == idToBlock) {
                    error1.error = 2;
                }
            }
            break;
        }
    }
    if (error1.error == 0) {
        users[nrUser].blocked[users[nrUser].nrBlocked] = idToBlock;
        users[nrUser].nrBlocked++;
    }
    msgsnd(ipcMSG, &error1, sizeof(error1) - sizeof(long), 0);
    semop(susers, &sOpen, 1);
}

void blockGroup(group *groups, user *users, int ipcMSG, int m, int n) {
    int idToBlock = 0;
    id id1 = {};
    text text1 = {};
    int nrUser = 0;
    error error1 = {3, 1};
    msgrcv(ipcMSG, &id1, sizeof(id1) - sizeof(long), 4, 0);
    msgrcv(ipcMSG, &text1, sizeof(text1) - sizeof(long), 8, 0);
    for (int i = 0; i < m; i++) {
        if (strcmp(groups[i].name, text1.name) == 0) {
            idToBlock = groups[i].id;
            error1.error--;
            break;
        }
    }
    semop(susers, &sClose, 1);
    for (int i = 0; i < n; i++) {
        if (users[i].id == id1.id) {
            nrUser = i;
            for (int j = 0; j < users[i].nrBlockedGroups; j++) {
                if (users[i].blockedGroups[j] == idToBlock) {
                    error1.error = 2;
                }
            }
            break;
        }
    }
    if (error1.error == 0) {
        users[nrUser].blockedGroups[users[nrUser].nrBlockedGroups] = idToBlock;
        users[nrUser].nrBlockedGroups++;

    }
    msgsnd(ipcMSG, &error1, sizeof(error1) - sizeof(long), 0);
    semop(susers, &sOpen, 1);
}

void log_out(int ipcMsg, int ipcGet) {
    error error1 = {};
    error1.mtype = 3;
    msgsnd(ipcMsg, &error1, sizeof(error1) - sizeof(long), 0);
    msg msg1 = {};
    msg1.mtype = 6;
    msg1.sender = -1;
    msgsnd(ipcGet, &msg1, sizeof(msg1) - sizeof(long), 0);
    exit(0);

}

void changeSignal() {
    msgctl(ipc, IPC_RMID, 0);
    shmctl(shmidusers, IPC_RMID, 0);
    semctl(susers, 1, IPC_RMID, 0);
    shmctl(shmidgroups, IPC_RMID, 0);
    semctl(sgroups, 1, IPC_RMID, 0);
    exit(0);
}
//
// Created by ola on 03.01.2020.
//

#ifndef WSPOLBIEZNE_STRUCTURES_H
#define WSPOLBIEZNE_STRUCTURES_H
typedef struct send_login{
    long mtype;
    char login[30];
}send_login;
typedef struct send_password{
    long mtype;
    char password[30];
}send_password;
typedef struct error{
    long mtype;
    int error; //0 - valid
}error;
typedef struct id{
    long mtype;
    int id;
}id;
typedef struct msg{
    long mtype;
    char receiver[30];
    char msg[248];
}msg;
typedef struct clientRequest{
    long mtype;
    int request;
}clientRequest;
typedef struct user {
    char login[30];
    char password[30];
    int active;
    int id;
    int blocked[30];
    int loggingTries;
} user;
//temporary users in groups are static - u can implement sth or not
typedef struct group {
    int id;
    char name[30];
    int listOfUsers[25];
} group;
#endif //WSPOLBIEZNE_STRUCTURES_// H
//client
#include <stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include <signal.h>
#include "structures.h"

int log_in(int ipc);
void log_out();
void changeSignal();

int ipc;
int ipcMSG;
int ipcGet;

int main(){
    signal(SIGINT, changeSignal);
    int id = -1;
    int request;
    ipc = msgget(1024, 0600 | IPC_CREAT);
    while(1) {
        while (id == - 1) {
            id = log_in(ipc);
        }
        ipcMSG = msgget(1024 + 2 * id, 0600 | IPC_CREAT);
        ipcGet = msgget(1025 + 2 * id, 0600 | IPC_CREAT);
        int f = fork();
        char buf[2048];
        int userActive = 1;

        if (f != 0) {
            clientRequest clientRequest1 = {5};
            while (userActive) {

                printf("%s",
                       "\nCo chcesz zrobić teraz?\n 1 - napisz wiadomość do użytkownika \n 2 - napisz wiadomość do grupy \n 3 - wyświetl listę grup\n "
                       "4 - wyświetl członków danej grupy \n 5 - wyświetl aktywnych użytkowników \n 6 - dołącz do grupy \n 7 - opuść grupę \n 8 - zablokuj użytkownika \n 9 - zablokuj grupę \n 10 - wyloguj się \n");
                read(1, buf, 4);
                fflush(stdin);
                int counter = 0;
                request = buf[(counter)++] - 48;
                while (buf[counter] >= '0' && buf[counter] <= '9') {
                    request = request * 10 + buf[(counter)++] - 48;
                }
                msg msg1 = {6};
                sysMSG sysMsg = {};
                error error1 = {};
                text text1 = {};
                struct id id1 = {};
                switch (request) {
                    case 1:
                        clientRequest1.request = 1;
                        msgsnd(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 0);
                        msg1.groupORuser = 0;
                        msg1.sender = id;
                        printf("%s", "Do kogo chcesz wysłać wiadomość? \n");
                        scanf("%s", msg1.receiver);
                        printf("%s", "Napisz wiadomość: \n");
                        read(1, buf, 2048);
                        strcpy(msg1.msg, buf);
                        msgsnd(ipcMSG, &msg1, sizeof(msg1) - sizeof(long), 0);
                        msgrcv(ipcMSG, &error1, sizeof(error1) - sizeof(long), 3, 0);
                        if (error1.error == 4) {
                            printf("%s", "Podanego użytkownika nie ma wśród użytkowników zalogowanych. Wiadomość nie została dostarczona. \n");
                        } else if (error1.error == 5) {
                            printf("%s", "Podany użytkownik Cię zablokował. Wiadomość nie została dostarczona. \n");
                        }
                        break;
                    case 2:
                        clientRequest1.request = 2;
                        msgsnd(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 0);
                        msg1.groupORuser = 1;
                        msg1.sender = id;
                        printf("%s", "Do jakiej grupy chcesz wysłać wiadomość? \n");
                        scanf("%s", msg1.receiver);
                        printf("%s", "Napisz wiadomość: \n");
                        read(1, buf, 2048);
                        strcpy(msg1.msg, buf);
                        msgsnd(ipcMSG, &msg1, sizeof(msg1) - sizeof(long), 0);
                        break;
                    case 3:
                        clientRequest1.request = 3;
                        msgsnd(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 0);
                        printf("%s", "Istniejące grupy: \n");
                        msgrcv(ipcMSG, &sysMsg, sizeof(sysMsg) - sizeof(long), 7, 0);
                        for (int i = 0; i < sysMsg.nr; i++) {
                            printf("%s \n", sysMsg.all[i]);
                        }
                        break;
                    case 4:
                        clientRequest1.request = 4;
                        msgsnd(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 0);
                        printf("%s", "Podaj nazwę grupy\n");
                        scanf("%s", text1.name);
                        text1.mtype = 8;
                        msgsnd(ipcMSG, &text1, sizeof(text1) - sizeof(long), 0);
                        msgrcv(ipcMSG, &error1, sizeof(error1) - sizeof(long), 3, 0);
                        if (error1.error == 0) {
                            printf("%s", "Członkowie: \n");
                            msgrcv(ipcMSG, &sysMsg, sizeof(sysMsg) - sizeof(long), 7, 0);
                            for (int i = 0; i < sysMsg.nr; i++) {
                                printf("%s \n", sysMsg.all[i]);
                            }
                        } else {
                            printf("%s", "Grupa o podanej nazwie nie istnieje \n");
                        }

                        break;

                    case 5:
                        clientRequest1.request = 5;
                        msgsnd(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 0);
                        printf("%s", "Aktywni użytkownicy:\n");
                        msgrcv(ipcMSG, &sysMsg, sizeof(sysMsg) - sizeof(long), 7, 0);
                        for (int i = 0; i < sysMsg.nr; i++) {
                            printf("%s \n", sysMsg.all[i]);
                        }
                        break;

                    case 6:
                        clientRequest1.request = 6;
                        msgsnd(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 0);
                        printf("%s", "Podaj nazwę grupy\n");
                        scanf("%s", text1.name);
                        text1.mtype = 8;
                        msgsnd(ipcMSG, &text1, sizeof(text1) - sizeof(long), 0);
                        id1.mtype = 4;
                        id1.id = id;
                        msgsnd(ipcMSG, &id1, sizeof(id1) - sizeof(long), 0);
                        msgrcv(ipcMSG, &error1, sizeof(error1) - sizeof(long), 3, 0);
                        if (error1.error == 0) {
                            printf("%s", "Dołączono \n");
                        } else if (error1.error == 2) {
                            printf("%s", "Już jesteś członkiem tej grupy \n");
                        } else {
                            printf("%s", "Grupa o podanej nazwie nie istnieje \n");
                        }
                        break;
                    case 7:
                        clientRequest1.request = 7;
                        msgsnd(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 0);
                        printf("%s", "Podaj nazwę grupy\n");
                        scanf("%s", text1.name);
                        text1.mtype = 8;
                        msgsnd(ipcMSG, &text1, sizeof(text1) - sizeof(long), 0);
                        id1.mtype = 4;
                        id1.id = id;
                        msgsnd(ipcMSG, &id1, sizeof(id1) - sizeof(long), 0);
                        msgrcv(ipcMSG, &error1, sizeof(error1) - sizeof(long), 3, 0);
                        if (error1.error == 0) {
                            printf("%s", "Nie jesteś już członkiem tej grupy \n");
                        } else if (error1.error == 1) {
                            printf("%s", "Nie należysz do tej grupy, nie możesz jej więc opuścić \n");
                        } else {
                            printf("%s", "Grupa o podanej nazwie nie istnieje \n");
                        }
                        break;
                    case 8:
                        clientRequest1.request = 8;
                        msgsnd(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 0);
                        id1.id = id;
                        id1.mtype = 4;
                        printf("%s", "Kogo chcesz zablokować? \n");
                        scanf("%s", text1.name);
                        text1.mtype = 8;
                        msgsnd(ipcMSG, &id1, sizeof(id1) - sizeof(long), 0);
                        msgsnd(ipcMSG, &text1, sizeof(text1) - sizeof(long), 0);
                        msgrcv(ipcMSG, &error1, sizeof(error1) - sizeof(long), 3, 0);
                        if (error1.error == 0) {
                            printf("%s", "Użytkownik został zablokowany \n");
                        } else if (error1.error == 1) {
                            printf("%s", "Podany użytkownik nie istnieje \n");
                        } else {
                            printf("%s", "Już zablokowałeś tego użytkownika \n");
                        }
                        break;

                    case 9:
                        clientRequest1.request = 9;
                        msgsnd(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 0);
                        id1.id = id;
                        id1.mtype = 4;
                        printf("%s", "Jaką grupę chcesz zablokować? \n");
                        scanf("%s", text1.name);
                        text1.mtype = 8;
                        msgsnd(ipcMSG, &id1, sizeof(id1) - sizeof(long), 0);
                        msgsnd(ipcMSG, &text1, sizeof(text1) - sizeof(long), 0);
                        msgrcv(ipcMSG, &error1, sizeof(error1) - sizeof(long), 3, 0);
                        if (error1.error == 0) {
                            printf("%s", "Grupa została zablokowana \n");
                        } else if (error1.error == 1) {
                            printf("%s", "Podana grupa nie istnieje \n");
                        } else {
                            printf("%s", "Już zablokowałeś tę grupę \n");
                        }
                        break;

                    case 10:
                        log_out();
                        userActive = 0;
                        break;


                    default:
                        printf("%s", "Wprowadzono niepoprawną komendę\n");
                        break;
                }
            }
        } else {
            while (1) {
                msg msg1 = {};
                msgrcv(ipcGet, &msg1, sizeof(msg1) - sizeof(long), 6, 0);
                printf("%s", msg1.receiver);
                if (msg1.sender == -1) {
                    msgctl(ipcGet, IPC_RMID, 0);
                    exit(0);
                }
                if (msg1.groupORuser == 0) {
                    printf("%s", " napisał/a:\n");
                } else {
                    printf("%s", " napisał/a do grupy:\n");
                }
                printf("%s \n", msg1.msg);
            }
        }
    }

}

int log_in(int ipc){
    while(1) {
        error error1 = {};
        char login[30];
        char password[30];
        printf("%s", "Podaj login \n");
        scanf("%s", login);
        send_login send_login1 = {1};
        strcpy(send_login1.login, login);
        msgsnd(ipc, &send_login1, sizeof(send_login) - sizeof(long), 0);
        msgrcv(ipc, &error1, sizeof(error) - sizeof(long), 3, 0);
        if (error1.error == 1) {
            printf("%s", "Podany login nie istnieje, spróbuj ponownie \n");
            continue;
        }
        else if (error1.error == 4){
            printf("%s", "Użytkownik jest już zalogowany. \n");
            continue;
        }
        printf("%s", "Podaj hasło \n");
        scanf("%s", password);
        send_password send_password1 = {2};
        strcpy(send_password1.password, password);
        msgsnd(ipc, &send_password1, sizeof(send_password) - sizeof(long), 0);
        msgrcv(ipc, &error1, sizeof(error) - sizeof(long), 3, 0);
        if (error1.error == 2) {
            printf("%s", "Podano błędne hasło, spróbuj ponownie \n");
            continue;
        } else if (error1.error == 3) {
            printf("%s", "Trzy razy podano błędne hasło. Konto zostaje zablokowane. \n");
            return -1;
        }
        break;
    }
    id userID = {};
    msgrcv(ipc, &userID, sizeof(error) - sizeof(long), 4, 0);
    return userID.id;
}

void log_out(){
    if (msgget(1024, 0600) == -1){
        msgctl(ipcMSG, IPC_RMID, 0);
        msgctl(ipcGet, IPC_RMID, 0);

    }
    else {
        clientRequest clientRequest1 = {5};
        clientRequest1.request = 10;
        error error1 = {3};
        msgsnd(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 0);
        msgrcv(ipcMSG, &error1, sizeof(error1) - sizeof(long), 3, 0);
        if (error1.error == 0) {
            msgctl(ipcMSG, IPC_RMID, 0);
        }
    }
}

void changeSignal(){
    log_out();
    exit(0);

}
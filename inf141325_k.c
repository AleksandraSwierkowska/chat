//client
#include <stdio.h>
#include<sys/types.h>
#include<sys/ipc.h>
#include<sys/msg.h>
#include<stdlib.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
#include "structures.h"

int log_in(int ipc);

int main(){
    int id;
    int request;
    int ipc = msgget(1024, 0666 | IPC_CREAT);
    id = log_in(ipc);
    int ipcMSG = msgget(1024+id, 0666 | IPC_CREAT);
    int f = fork();
    if (f == 0){
        clientRequest clientRequest1 = {5};
        while(1){
            fflush(stdin);
            printf("%s", "Co chcesz zrobić teraz?\n 1 - napisz wiadomość do użytkownika \n");
            scanf("%d", &request);
            switch (request){
                case 1:
                    clientRequest1.request = 1;
                    msgsnd(ipcMSG, &clientRequest1, sizeof(clientRequest1) - sizeof(long), 0);
                    msg msg1 = {6};
                    error error1 = {};
                    printf("%s", "Do kogo chcesz wysłać wiadomość? \n");
                    scanf("%s", msg1.receiver);
                    printf("%s", "Napisz wiadomość: \n");
                    scanf("%s", msg1.msg);
                    msgsnd(ipcMSG, &msg1, sizeof(msg1) - sizeof(long), 0);
                    msgrcv(ipcMSG, &error1, sizeof(error1) - sizeof(long), 3, 0);
                    if (error1.error == 4){
                        printf("%s", "Podany użytkownik jest nieaktywny. Wiadomość nie została dostarczona. \n");
                    }
                    else if (error1.error == 5){
                        printf("%s", "Podany użytkownik Cię zablokował. Wiadomość nie została dostarczona. \n");
                    }
                    break;
            }
        }
    }
    else{}
    return 0;

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
            break;
        }
        break;
    }
    id userID = {};
    msgrcv(ipc, &userID, sizeof(error) - sizeof(long), 4, 0);
    return userID.id;
}


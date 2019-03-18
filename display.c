#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "struct.h"

int main() {

    struct msg data;
    data.mtype = 10;
    data.misie = 0;
    data.robotnice = 0;
    data.wojownicy = 0;
    data.krolowe = 0;
    data.miodek = 0;
    data.koniec = 0;

    int msgid = msgget(93, IPC_CREAT | 0640);

    while(1) {
        perror("Waiting");
        msgrcv(msgid, &data, sizeof(data), 10, 0);
        perror("Received");
        system("clear");
        printf("Misie buszujące w lesie: %d\n", data.misie);
        printf("Miodek w magazynie: %d\n", data.miodek);
        printf("Pszczółki robotnice: %d\n", data.robotnice);
        printf("Pszczółki wojownice: %d\n", data.wojownicy);
        printf("Pszczółki królowe (WinPoints): %d\n", data.krolowe);
        if (data.koniec == 1) break;
    }

    msgctl(msgid, IPC_RMID, 0);
    return 0;
}

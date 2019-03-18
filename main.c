#include <stdio.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <memory.h>
#include <time.h>
#include "struct.h"

#define SemMiodek 0
#define SemRobotnica 1
#define SemWojownica 2
#define SemAtakMisia 3
#define SemMisiek 4
#define SemMagazyn 5
#define SemKrolowa 6


#define KOSZT_ROBOTNICY 10
#define CZAS_TWORZENIA_ROBOTNICY 10
#define CZAS_JEDZENIA_ROBOTNICY 11
#define CZAS_PRODUKCJI_ROBOTNICY 5

#define KOSZT_WOJOWNICY 10
#define CZAS_TWORZENIA_WOJOWNICY 10
#define CZAS_JEDZENIA_WOJOWNICY 11

#define KOSZT_KROLOWEJ 500
#define CZAS_TWORZENIA_KROLOWEJ 7
#define SZANSA_ROBOTNICA 25
#define SZANSA_WOJOWNICA 25
#define SZANSA_KROLOWA 50

#define SEN_MISIA 10
#define SZANSA_ATAK 10
#define SZANSA_WEZWANIE 10

/**
 * PRIMAL UTILITY.
 * Funkcja odpowiada za działanie na semaforze.
 */
void zmienSemafor(int semid, int group, int value) {
    struct sembuf zmiana;
        zmiana.sem_num = (unsigned short)group;
        zmiana.sem_op = (short)value;
        zmiana.sem_flg = 0;
    semop(semid, &zmiana, 1);
}

int* miodek;
int* robotnice;
int* wojownice;
int* koniecGry;
int* misie;
int* krolowe;

int msgid;

int semKey;

/**
 * Funkcja odpowiedzialna za koniec.
 */
void koniec() {
    *koniecGry = 1;
    printf("Dziękujemy za zagranie w Bee INC.!\n");
    sleep(4);
    printf("Trwa zamykanie systemu..");
    sleep(3);
    semctl(semKey, IPC_RMID, 0);
    shmctl(*miodek, IPC_RMID, 0);
    shmctl(*robotnice, IPC_RMID, 0);
    shmctl(*wojownice, IPC_RMID, 0);
    shmctl(*koniecGry, IPC_RMID, 0);
    shmctl(*misie, IPC_RMID, 0);
    shmctl(*krolowe, IPC_RMID, 0);
    msgctl(msgid, IPC_RMID, 0);
    exit(0);
}

int nowaRobotnica(int wait) {
    int pid = fork();
    if(pid == 0) { /* proces potomny */

        zmienSemafor(semKey, SemRobotnica, -1);
        *robotnice += 1;
        zmienSemafor(semKey, SemRobotnica, 1);

        if (wait == 1)
            sleep(CZAS_TWORZENIA_ROBOTNICY);
        if (wait == 2)
            sleep(CZAS_TWORZENIA_KROLOWEJ);

        short glod = 0;
        short produkcja = 0;

        while(1) {

            zmienSemafor(semKey, SemAtakMisia, -3); // Atak Misia
            zmienSemafor(semKey, SemAtakMisia, 2); // Atak Misia
            zmienSemafor(semKey, SemAtakMisia, 1); // Atak Misia

            if (*koniecGry == 1) exit(0);

            if (glod == CZAS_JEDZENIA_ROBOTNICY) {
                zmienSemafor(semKey, SemMiodek, -1);
                zmienSemafor(semKey, SemMagazyn, -1);
                if ((*miodek) >= 1) {
                    *miodek -= 1;
                    zmienSemafor(semKey, SemMagazyn, 1);
                    zmienSemafor(semKey, SemMiodek, 1);
                    glod = 0;
                }
                else {
                    // Śmierć;
                    printf("Zabrakło miodu, koniec gry..\n");
                    koniec();
                }
            }
            if (produkcja == CZAS_PRODUKCJI_ROBOTNICY) {
                zmienSemafor(semKey, SemMiodek, -1);
                zmienSemafor(semKey, SemMagazyn, -1);
                *miodek += 1;
                zmienSemafor(semKey, SemMagazyn, 1);
                zmienSemafor(semKey, SemMiodek, 1);
                produkcja = 0;
            }

            glod++;
            produkcja++;
            sleep(1);
        }
        exit(0);
    };
    return pid;
}

int nowaWojownica(int wait) {
    int pid = fork();
    if(pid == 0) { /* proces potomny */

        zmienSemafor(semKey, SemWojownica, -1);
        *wojownice += 1;
        zmienSemafor(semKey, SemWojownica, 1);

        if (wait == 1)
            sleep(CZAS_TWORZENIA_WOJOWNICY);
        if (wait == 2)
            sleep(CZAS_TWORZENIA_KROLOWEJ);

        short glod = 0;

        while(1) {

            zmienSemafor(semKey, SemAtakMisia, -3); // Atak Misia
            zmienSemafor(semKey, SemAtakMisia, 2); // Atak Misia
            zmienSemafor(semKey, SemAtakMisia, 1); // Atak Misia

            if (*koniecGry == 1) exit(0);

            if (glod == CZAS_JEDZENIA_WOJOWNICY) {
                zmienSemafor(semKey, SemMiodek, -1);
                zmienSemafor(semKey, SemMagazyn, -1);
                if (*miodek > 0) {
                    *miodek -= 1;
                    zmienSemafor(semKey, SemMagazyn, 1);
                    zmienSemafor(semKey, SemMiodek, 1);
                    glod = 0;
                } else {
                    printf("Zabrakło miodu, koniec gry..\n");
                    koniec();
                }
            }

            glod++;
            sleep(1);
        }
        exit(0);
    };
    return pid;
}

void nowaKrolowa() {
    if (fork() == 0) {

        printf("Jajeczko królowe!\n");
        sleep(CZAS_TWORZENIA_KROLOWEJ);
        zmienSemafor(semKey, SemKrolowa, -1);
        *krolowe += 1;
        zmienSemafor(semKey, SemKrolowa, 1);
        printf("Urodziła się królowa!\n");
        if (*krolowe >= 3) {
            printf("Wygrana! 3 królowe zostały stworzone!\n");
            koniec();
        }
        exit(0);
    }
}


// Tutaj obsługę poprawić
void obslugaPszczol() {
    if (fork() == 0) {

        int rozmiarRobotnic = 300;
        int rozmiarWojownic = 200;
        int* pidRobotnic = malloc(sizeof(int)*rozmiarRobotnic);
        int* pidWojownic = malloc(sizeof(int)*rozmiarRobotnic);

        struct akcja akcja;

        while(1) {
            if (*koniecGry == 1) exit(0);
            msgrcv(msgid, &akcja, sizeof(akcja), 1, 0);
            if (*koniecGry == 1) exit(0);


            switch(akcja.typ) {
                case 0: /* nowa pszczoła */
                    switch(akcja.typPszczoly) {
                        case 0: /* robotnica */
                            zmienSemafor(semKey, SemMiodek, -1);

                            if ((*miodek) > akcja.info * KOSZT_ROBOTNICY) {
                                *miodek -= akcja.info * KOSZT_ROBOTNICY;

                                for (int i = 0; i < akcja.info; i++) {
                                    int dodana = 0;
                                    for (int j = 0; j < rozmiarRobotnic; j++) {
                                        if (pidRobotnic[j] <= 0) {
                                            pidRobotnic[j] = nowaRobotnica(1);
                                            dodana = 1;
                                            break;
                                        }
                                    }
                                    if (dodana == 0) {
                                        /* zwiększ miejsce */
                                        int *temp = realloc(pidRobotnic, sizeof(int)*(rozmiarRobotnic+100));
                                        pidRobotnic = temp;
                                        pidRobotnic[rozmiarRobotnic] = nowaRobotnica(1);
                                        rozmiarRobotnic += 100;
                                    }
                                }
                            } else {
                                printf("Za mało miodku na produkcję.\n");
                            }
                            zmienSemafor(semKey, SemMiodek, 1);
                            break;
                        case 1: /* wojownica */
                            zmienSemafor(semKey, SemMiodek, -1);

                            if ((*miodek) > akcja.info * KOSZT_WOJOWNICY) {
                                *miodek -= akcja.info * KOSZT_WOJOWNICY;


                                for (int i = 0; i < akcja.info; i++) {
                                    int dodana = 0;
                                    for (int j = 0; j < rozmiarWojownic; j++) {
                                        if (pidWojownic[j] <= 0) {
                                            pidWojownic[j] = nowaWojownica(1);
                                            dodana = 1;
                                            break;
                                        }
                                    }
                                    if (dodana == 0) {
                                        /* zwiększ miejsce */
                                        int *temp = realloc(pidWojownic, sizeof(int)*(rozmiarWojownic+100));
                                        pidWojownic = temp;
                                        pidWojownic[rozmiarWojownic] = nowaWojownica(1);
                                        rozmiarWojownic += 100;
                                    }
                                }
                            }
                            else {
                                printf("Za mało miodku na produkcję.\n");
                            }
                            zmienSemafor(semKey, SemMiodek, 1);
                            break;
                        case 2: /* królownica */
                        {
                            zmienSemafor(semKey, SemMiodek, -1);
                            int jajeczko = 0;
                            if ((*miodek) > KOSZT_KROLOWEJ) {
                                *miodek -= KOSZT_KROLOWEJ;

                                int szansa = rand()%(SZANSA_ROBOTNICA+SZANSA_WOJOWNICA+SZANSA_KROLOWA);
                                if (szansa <=  SZANSA_ROBOTNICA) {
                                    jajeczko = 0;
                                } else if (szansa <=  SZANSA_ROBOTNICA + SZANSA_WOJOWNICA) {
                                    jajeczko = 1;
                                } else {
                                    jajeczko = 2;
                                }

                                switch(jajeczko) {
                                    case 0: /* robotnica */ {
                                        for (int i = 0; i < rozmiarRobotnic; i++) {
                                            if (pidRobotnic[i] == 0) {
                                                zmienSemafor(semKey, SemRobotnica, -1);
                                                *robotnice += 1;
                                                pidRobotnic[i] = nowaRobotnica(2);
                                                printf("Urodziła się robotnica.\n");
                                                zmienSemafor(semKey, SemRobotnica, 1);
                                                break;
                                            }
                                        }
                                        break;
                                    }
                                    case 1: /* wojownica */ {
                                        for (int i = 0; i < rozmiarWojownic; i++) {
                                            if (pidWojownic[i] == 0) {
                                                zmienSemafor(semKey, SemWojownica, -1);
                                                *wojownice += 1;
                                                pidWojownic[i] = nowaWojownica(2);
                                                printf("Urodziła się wojownica.\n");
                                                zmienSemafor(semKey, SemWojownica, 1);
                                                break;
                                            }
                                        }
                                        break;
                                    }
                                    case 2: /* krolowica */ {
                                        nowaKrolowa();
                                    }
                                    default:
                                        break;
                                }
                            } else { printf("Niewystarczająco miodku!\n"); }
                            zmienSemafor(semKey, SemMiodek, 1);
                            }


                            break;
                            default:
                                break;
                        }
                    break;

                case 1: /* zabicie pszczoły przez misia */
                    printf("Zabijanie %d pszczół przez misia, typu %d\n", akcja.info, akcja.typPszczoly);
                    int doZabicia = akcja.info;
                    switch(akcja.typPszczoly) {
                        case 0: /* robotnica */
                            for (int i = 0; i < rozmiarRobotnic; i++) {
                                if (pidRobotnic[i] != 0 && doZabicia != 0) {
                                    kill(pidRobotnic[i], 9);
                                    pidRobotnic[i] = 0;
                                    zmienSemafor(semKey, SemRobotnica, -1);
                                        *robotnice -= 1;
                                    printf("Zostało %d robotnic\n",*robotnice);
                                    zmienSemafor(semKey, SemRobotnica, 1);
                                    doZabicia -= 1;
                                }
                            }
                            break;
                        case 1: /* wojownica */
                            for (int i = 0; i < rozmiarWojownic; i++) {
                                if (pidWojownic[i] != 0 && doZabicia != 0) {
                                    kill(pidWojownic[i], 9);
                                    pidWojownic[i] = 0;
                                    zmienSemafor(semKey, SemWojownica, -1);
                                        *wojownice -= 1;
                                    printf("Zostało %d wojownic\n",*wojownice);
                                    zmienSemafor(semKey, SemWojownica, 1);
                                    doZabicia -= 1;
                                }
                            }
                            break;
                    }

                    break;

                    /* Nie używane */
                case 2: /* zabicie pszczoły przez głód */
                    printf("Zabijanie pszczoły(%d) z głodu, typu %d", akcja.info, akcja.typPszczoly);
                    switch(akcja.typPszczoly) {
                        case 0: /* robotnica */
                            for (int i = 0; i < rozmiarRobotnic; i++) {
                                if (pidRobotnic[i] == akcja.info) {
                                    kill(pidRobotnic[i], 9);
                                    pidRobotnic[i] = 0;
                                    zmienSemafor(semKey, SemRobotnica, -1);
                                        *robotnice -= 1;
                                    printf("Zabito, zostało - %d robotnic\n",*robotnice);
                                    zmienSemafor(semKey, SemRobotnica, 1);
                                    break;
                                }
                            }
                            break;
                        case 1: /* wojownica */
                            for (int i = 0; i < rozmiarWojownic; i++) {
                                if (pidWojownic[i] == akcja.info) {
                                    kill(pidWojownic[i], 9);
                                    pidWojownic[i] = 0;
                                    zmienSemafor(semKey, SemWojownica, -1);
                                        *wojownice -= 1;
                                    printf("Zabito, zostało - %d wojownic\n",*wojownice);
                                    zmienSemafor(semKey, SemWojownica, 1);
                                }
                                break;
                            }
                            break;
                        default:
                            break;
                    }
                    break;

                default:
                    break;
            }

        }

    }
}

void nowyMisiek() {
    if (fork() == 0) {
        struct akcja akcja;

        zmienSemafor(semKey, SemMisiek, -1);
        *misie += 1;
        zmienSemafor(semKey, SemMisiek, 1);

        int atak = SZANSA_ATAK;
        int ziomek = SZANSA_WEZWANIE;
        int random;

        while(1) {
            // rand losowanie Misia

            sleep(SEN_MISIA);
            
            if (*koniecGry == 1) exit(0);

            random = rand() % 100;

            /* ATAK */
            if (random < atak) {

                zmienSemafor(semKey, SemAtakMisia, -2); // Zapewnienie wykluczania pszczół
                zmienSemafor(semKey, SemAtakMisia, -1); // Zapewnienie wykluczania misiów

                for (int i = 0; i < 10; i++)
                    zmienSemafor(semKey, SemMagazyn, -1);

                printf("Atak Misia!\n");

                sleep(1);
                int wojowniceLocal = 0;
                int robotniceLocal = 0;

                zmienSemafor(semKey, SemRobotnica, -1);
                zmienSemafor(semKey, SemWojownica, -1);
                if (*wojownice >= 10)
                    wojowniceLocal = 10;
                else {
                    wojowniceLocal = *wojownice;
                    if (*robotnice >= 10 - *wojownice)
                        robotniceLocal = 10 - *wojownice;
                    else
                        robotniceLocal = *robotnice;

                    zmienSemafor(semKey, SemMiodek, -1);
                    *miodek -= 10;
                    if (*miodek < 0)
                        koniec(); // koniec gry, komunikat.
                    zmienSemafor(semKey, SemMiodek, 1);

                    }
                zmienSemafor(semKey, SemRobotnica, 1);
                zmienSemafor(semKey, SemWojownica, 1);


                akcja.mtype = 1;
                akcja.typ = 1; // zabij pszczołę przez Misia
                akcja.typPszczoly = 0; // robotnice
                akcja.info = robotniceLocal; // ile
                msgsnd(msgid, &akcja, sizeof(akcja), 0);


                akcja.typPszczoly = 1; // wojownice
                akcja.info = wojowniceLocal; // ile
                msgsnd(msgid, &akcja, sizeof(akcja), 0);

                zmienSemafor(semKey, SemMagazyn, 10);
                zmienSemafor(semKey, SemAtakMisia, 2);
                zmienSemafor(semKey, SemAtakMisia, 1);
            atak = 0;
            }

            random = rand() % 100;

            /* PRZYZWANIE */
            if (random < ziomek) {
                printf("Nowy miś\n");
                nowyMisiek();
                ziomek = 0;
            }
            
            atak += SZANSA_ATAK;
            ziomek += SZANSA_WEZWANIE;
        }
    }
    printf("Nowy misiek!\n");
}

/**
 * UTILITY.
 * Funkcja odpowiedzialna za pobieranie wyłącznie INTEGER'a.
 * @return
 */
int scanInteger() {
    char *end;
    char buf[100];
    int n;

    while (fgets(buf, sizeof(buf), stdin)) {
        n = (int)strtol(buf, &end, 10);
        if (end == buf || *end != '\n') {
            printf("Enter an integer\n");
        } else break;
    }
    return n;
}

void testSem() {
    for (int i = 0; i < 6; i++) {
        printf("Semafor[%d] = %d\n",i, semctl(semKey, i, GETVAL, 0));
    }
}

void testShm() {
    printf("Miodek = %d\n",*miodek);
    printf("Robotnice = %d\n",*robotnice);
    printf("Wojownice = %d\n",*wojownice);
    printf("Koniec = %d\n",*koniecGry);
    printf("Misie = %d\n",*misie);
    printf("Królowe = %d\n",*krolowe);
}

void mainMenu() {
    int wybor;
    printf("Witaj w Bee INC.\n");
    printf("1. Rozpocznij grę.\n");
    printf("2. Zakończ grę.\n");
    while(1) {

        switch(wybor = scanInteger()) {
            case 1: /* Rozpocznij grę */
                nowyMisiek();
                while (1) {
                    int wybor;
                    printf("Choose wisely:\n");
                    printf("1. Nowa robotnica.\n");
                    printf("2. Nowa wojownica.\n");
                    printf("3. Nowa królowa.\n");
                    printf("4. Zakończ grę.\n");

                    struct akcja msg;

                    switch(wybor = scanInteger()) {
                        case 1: /* Nowa robotnice */
                            printf("Ile robotnic chcesz stworzyć?\n");
                            msg.mtype = 1;
                            msg.typ = 0;
                            msg.typPszczoly = 0;
                            msg.info = scanInteger();
                            msgsnd(msgid, &msg, sizeof(msg), 0);
                            break;
                        case 2: /* Nowa wojownica */
                            printf("Ile wojownic chcesz stworzyć?\n");
                            msg.mtype = 1;
                            msg.typ = 0;
                            msg.typPszczoly = 1;
                            msg.info = scanInteger();
                            msgsnd(msgid, &msg, sizeof(msg), 0);
                            break;
                        case 3: /* Nowa królowa */
                            printf("Tworzenie królowej.\n");
                            msg.mtype = 1;
                            msg.typ = 0;
                            msg.typPszczoly = 2;
                            msg.info = 1;
                            msgsnd(msgid, &msg, sizeof(msg), 0);
                            break;
                        case 4: /* Zakończ grę */
                            koniec();
                            return;
                        case 997: /* Test semafora i pamięci współdzielonej */
                            testSem();
                            testShm();
                        default:
                            break;
                    }
                }
            case 2: /* Zakończ grę */
                koniec();
                return;
            default:
                break;
        }
    }
}

void sendDisplay() {
    if (fork() == 0) {
        int msgidDisplay = msgget(93, IPC_CREAT | 0640);
        struct msg data;
        data.mtype = 10;
        while(1) {
            data.misie = *misie;
            data.robotnice = *robotnice;
            data.wojownicy = *wojownice;
            data.krolowe = *krolowe;
            data.miodek = *miodek;
            data.koniec = *koniecGry;
            msgsnd(msgidDisplay, &data, sizeof(data), 0);
            sleep(1);
        }
    }
}

int main() {
    printf("Hello, World!\n");

    srand(time(NULL));

        /* Semafory */
    semKey = semget(151, 6, IPC_CREAT | 0640);
    semctl(semKey, 0, SETVAL, 1); // miodek
    semctl(semKey, 1, SETVAL, 1); // robotnice
    semctl(semKey, 2, SETVAL, 1); // wojownice
    semctl(semKey, 3, SETVAL, 3); // atak misia
    semctl(semKey, 4, SETVAL, 1); // Misie
    semctl(semKey, 5, SETVAL, 100); // magazyn
    semctl(semKey, 6, SETVAL, 1); // Królowice

        /* Pamięć współdzielona */
    int idMemory = shmget(203, sizeof(int)*3, IPC_CREAT | 0640);
    miodek = shmat(idMemory, 0, 0);
    robotnice = shmat(idMemory, 0, 0) + sizeof(int);
    wojownice = shmat(idMemory, 0, 0) + sizeof(int) * 2;
    koniecGry = shmat(idMemory, 0, 0) + sizeof(int) * 3;
    misie = shmat(idMemory, 0, 0) + sizeof(int) * 4;
    krolowe = shmat(idMemory, 0, 0) + sizeof(int) * 5;

    *miodek = 50000;
    *robotnice = 0;
    *wojownice = 0;
    *koniecGry = 0;
    *misie = 0;
    *krolowe = 0;

    msgid = msgget(101, IPC_CREAT| 0640);

    obslugaPszczol();

    sendDisplay();
    mainMenu();

    semctl(semKey, IPC_RMID, 0);
    shmctl(*miodek, IPC_RMID, 0);
    shmctl(*robotnice, IPC_RMID, 0);
    shmctl(*wojownice, IPC_RMID, 0);
    shmctl(*koniecGry, IPC_RMID, 0);
    shmctl(*misie, IPC_RMID, 0);
    shmctl(*krolowe, IPC_RMID, 0);
    msgctl(msgid, IPC_RMID, 0);

    return 0;
}

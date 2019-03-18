#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>


struct akcja {
    long mtype;
    int typ;
    int typPszczoly;
    int info;
};

struct msg {
    long mtype;
    int misie;
    int robotnice;
    int wojownicy;
    int krolowe;
    int miodek;
    int koniec;
};

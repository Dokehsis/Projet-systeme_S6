#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <pwd.h>
#include <features.h>
#include <errno.h>
#include <dirent.h>
#include <utime.h>

#define TAILLE_COM 500
#define NOMBRE_ARG 20
#define TAILLE_HIS 50
#define TAILLE_RD 512

#ifndef MY_SHELL_H
#define MY_SHELL_H

void lireCommande(char *c);
int execCommande(char *c, int descHist, int lastHistLine);
void parser(char *c, char **res);
void pathParser(char* envPath, char** paths);
int getHistorySize(int descHist);
void myHistory(char** tabCom, int descHist, int lastHistLine);
void myCat(char** tabCom);
void myExecn(int n, int descHist, int lastHistLine);
void myCp(char** tabCom);
void myTouch(char** tabCom);

#endif

#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <features.h>

#include "my_shell.h"
int main()
{
	char commande[TAILLE_COM];
	char invite[100], dirChar[100], dirShort[50], hostname[50], histFile[112], buf[1];
	int i, j, descHist, lastHistLine = 0, resRead;
	
	/*Obtention des informations a afficher dans l invite de commandes*/
	struct passwd *p = getpwuid(getuid());
	gethostname(hostname, sizeof(hostname));
	
	/*Creation/ouverture du fichier d historique des commandes*/
	getcwd(dirChar, 50);
	strcpy(histFile, dirChar);
	strcat(histFile, "/history.txt");
	descHist = open(histFile, O_RDWR | O_CREAT | O_APPEND);
	/*Calcul de l indice de la derniere ligne de l historique (pour history -n)*/
	while((resRead = read(descHist, buf, 1)) > 0)
	{
		if(buf[0] == '\n')
			lastHistLine++;
	}
	
	while(commande[0] != 0 && strcmp("exit\n", commande) != 0)
	{
		/*construction de la chaine de caracteres a afficher dans l invite de commandes*/
		memset(dirShort, 0, sizeof(dirShort));
		strcpy(invite, p->pw_name);
		strcat(invite, "@");
		strcat(invite, hostname);
		strcat(invite, ":");
		getcwd(dirChar, 50);
		/*obtention du nom du repertoire courant*/
		for(i = strlen(dirChar); dirChar[i] != '/' && i>0; i--);
		for(j = i+1; dirChar[j] != '\0'; j++){dirShort[j-i-1] = dirChar[j];}
		strcat(invite, dirShort);
		strcat(invite, "> ");
		strcpy(commande, "");
		printf("%s", invite);
		/*lecture, stockage et execution de la commande*/
		lireCommande(commande);
		write(descHist, commande, strlen(commande));
		execCommande(commande, descHist, lastHistLine);
	}
	printf("\n");
	close(descHist);
	return 0;
}

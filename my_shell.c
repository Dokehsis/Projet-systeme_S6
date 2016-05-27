#define _DEFAULT_SOURCE

#include "my_shell.h"

void lireCommande(char *c)
{
	fgets(c, TAILLE_COM, stdin);
}
/*
NOTE: 	le tableau qui contient les arguments d une commande apres passage dans le parser sera
		appele le TCP (Tableau de Commande Parsee)
*/

/*
Transforme une commande en un tableau d'arguments à traiter

Entree: la commande en chaine de caracteres
Sortie: le TCP
*/
void parser(char *c, char **res)
{
	int i, j=0, k=0;
	char *p;
	for(i=0; c[i]!='\0'; i++)
	{
		/*des qu un espace est rencontre, une nouvelle chaine de caracteres est stockee*/
		if(c[i] != ' ')
		{
			res[k][j] = c[i];
			j++;
		}
		else
		{
			res[k][j] = '\0';
			j=0;
			k++;
		}
	}
	/*remplacement du \n de fin de ligne par un \0 de fin de chaine*/
	p = strchr(res[k], '\n');
    if (p)
    {
        *p = '\0';
    }
    /*pointeur NULL de chaine de caractere en fin de tableau, necessaire pour execv()*/
	for(k = k+1; k<NOMBRE_ARG; k++){res[k] = (char*)NULL;}
}

/*
Transforme le PATH total en tableau des differents PATHs possibles

Entree: le PATH de l environnement
Sortie: le tableau de PATHs possibles
*/
void pathParser(char* envPath, char** paths)
{
	int i, j=0, k=0;
	for(i = 0; envPath[i]!='\0'; i++)
	{
		/*des que le caractere ':' est rencontre, une nouvelle chaine est stockee*/
		if(envPath[i] != ':')
		{
			paths[k][j++] = envPath[i];
		}
		else
		{
			paths[k++][j] = '\0';
			j=0;
		}
	}
	for(k = k+1; k<128; k++){paths[k] = (char*)NULL;}
}

/*
Retourne la taille (en lignes) du fichier de stockage de l historique des commandes

Entree: descripteur de fichier de l historique des commandes
Sortie: taille de l historique
*/
int getHistorySize(int descHist)
{
	int resRead, historySize = 0;
	char buf[1];
	lseek(descHist, 0, SEEK_SET);
	while((resRead = read(descHist, buf, 1)) > 0)
	{
		if(buf[0] == '\n')
			historySize++;
	}
	return historySize;
}

/*
Fonction de copie de fichiers

Entrees: descripteur de la source, nom de la destination
*/

void copyFile(int desc, char *file)
{
	int descCp, resRead, resWrite;
	struct stat sBuf;
	char buf[TAILLE_RD];
	descCp = open(file, O_WRONLY | O_CREAT);
	fstat(desc, &sBuf);
	fchmod(descCp, sBuf.st_mode);
	resRead = read(desc, buf, TAILLE_RD);
	while(resRead > 0)
	{
		/*On n'ecrit que le nombre de caracteres donnes par read()*/
		resWrite = write(descCp, buf, resRead);
		while(resWrite == EAGAIN || resWrite == EINTR)
			resWrite = write(descCp, buf, resRead);
		resRead = read(desc, buf, TAILLE_RD);
	}
	close(desc);
	close(descCp);
}

/*
Fonction de copie de repertoires

Entrees: nom de la source, nom de la destination
*/

void copyDir(char *sourceDir, char *destDir)
{
	struct dirent *curDir = NULL;
	struct stat sBuf;
	char dopeulDestDir[512];	/*Les dopeuls servent a stocker et concatener les chemins*/
	char dopeulSourceDir[512];
	const char special = '~';
	int desc;
	DIR *dir = NULL;
	DIR *dirCp = NULL;
	
	strcpy(dopeulDestDir, destDir);		/*On stock d abord les chemins de base*/
	strcpy(dopeulSourceDir, sourceDir);
	
	dir = opendir(dopeulSourceDir);
	dirCp = opendir(dopeulDestDir);
	
	stat(sourceDir, &sBuf);
	if(dirCp == NULL)
	{
		mkdir(dopeulDestDir, sBuf.st_mode);
	}
	
	curDir = readdir(dir);
	while(strcmp(curDir->d_name, "..")!=0){curDir = readdir(dir);}	/*On elimine les retours de . et ..*/
	while((curDir = readdir(dir)) != NULL)
	{
		/*Les dopeuls sont reinitialises, puis un /nomDeLaCopie leur est rajoute*/
		strcpy(dopeulDestDir, destDir);
		strcpy(dopeulSourceDir, sourceDir);
		strcat(dopeulSourceDir, "/");
		strcat(dopeulSourceDir, curDir->d_name);
		strcat(dopeulDestDir, "/");
		strcat(dopeulDestDir, curDir->d_name);
		stat(dopeulSourceDir, &sBuf);
		/*Test du type de fichier*/
		if(strchr(&special, dopeulSourceDir == NULL))
		{
			switch(sBuf.st_mode & S_IFMT)
			{
			case S_IFDIR:
				copyDir(dopeulSourceDir, dopeulDestDir);
				break;
			case S_IFREG:
				desc = open(dopeulSourceDir, O_RDONLY);
				copyFile(desc, dopeulDestDir);
				break;
			}
		}
	}
	closedir(dir);
	closedir(dirCp);
}

/*
Fonction qui retourne l historique des commandes

Entree: le TCP, descripteur de fichier de l historique, derniere ligne de l historique en fin de session precedente
Sortie: affichage (redirigeable) de l historique
*/
void myHistory(char** tabCom, int descHist, int lastHistLine)
{
	int i = 0, newLine = 1, resRead, historySize = getHistorySize(descHist), nbLines;
	char buf[1];
	
	lseek(descHist, 0, SEEK_SET);
	while((resRead = read(descHist, buf, 1)) > 0)
	{
		if(newLine) /*debut d'une nouvelle ligne*/
		{
			i++;
			if(tabCom[1] == NULL) /*absence de -n et de n*/
				printf("%4d ", i);
			else /*presence de -n ou de n*/
			{
				nbLines = strtol(tabCom[1], (char**)NULL, 10);
				if(strcmp("-n", tabCom[1]) == 0 && i>lastHistLine)
					printf("%4d ", i);
				if(nbLines != 0 && historySize - i < nbLines)
					printf("%4d ", i);
				
			}
			newLine = 0;
		}
		/*en cours de ligne*/
		if(tabCom[1] == NULL) /*absence de -n et de n*/
			printf("%c", buf[0]);
		else /*presence de -n ou de n*/
		{
			if(strcmp("-n", tabCom[1]) == 0 && i>lastHistLine)
				printf("%c", buf[0]);
			if(nbLines != 0 && historySize - i < nbLines)
				printf("%c", buf[0]);
		}
		if(buf[0] == '\n')
			newLine = 1;
	}
}

/*
Fonction de concatenation de fichiers

Entree: le TCP
Sortie: affichage (redirigeable) des fichiers concatenes
*/

void myCat(char** tabCom)
{
	char toRead[TAILLE_COM], buf[1];
	int numeroter = 0, count = 1, i, desc, resRead, newLine = 1;
	toRead[0] = 1;
	for(i=1; tabCom[i] != NULL; i++) /*verification de la presence de -n*/
	{
		if(strcmp("-n", tabCom[i]) == 0)
			numeroter = 1;
	}
	
	if(tabCom[1+numeroter] == NULL) /*aucun fichier n a ete indique*/
	{
		while(toRead[0] != 0)
		{
			strcpy(toRead, "");
			fgets(toRead, TAILLE_COM, stdin);
			if(numeroter && toRead[0] != 0)
				printf("%d ", count++);
			printf("%s", toRead);
		}
	}
	else /*au moins un fichier a ete indique > affichage de son contenu*/
	{
		for(i=1; tabCom[i] != NULL; i++)
		{
			desc = open(tabCom[i], O_RDONLY);
			if(desc != -1)
			{
				while((resRead = read(desc, buf, 1)) > 0)
				{
					if(newLine && numeroter)
					{
						printf("%4d ", count++);
						newLine = 0;
					}
					printf("%c", buf[0]);
					if(buf[0] == '\n')
						newLine = 1;
				}
			}
		}
	}
}

/*
Fonction de copie de fichiers et de dossiers

Entree: le TCP
*/
void myCp(char** tabCom)
{
	int desc, nbArgs;
	struct stat sBuf;

	for(nbArgs = 0; tabCom[nbArgs] != NULL; nbArgs++);
	if(nbArgs != 3)
	{	
		printf("Erreur: 2 parametres sont attendus. \nUsage: cp source destination\n");
	}
	else
	{
		stat(tabCom[1], &sBuf);
		/*Test du type de fichier*/
		if(S_ISDIR(sBuf.st_mode))
		{
			copyDir(tabCom[1], tabCom[2]);
		}
		else
		{
			desc = open(tabCom[1], O_RDONLY);
			if(desc == -1)
				printf("Il n existe ni dossier ni fichier de ce nom: %s\n", tabCom[1]);
			else
				copyFile(desc, tabCom[2]);
		}
	}
}

/*
Fonction d execution de l entree d indice n de l historique des commandes

Entree: indice de la commande, descripteur de fichier de l historique, derniere ligne de l historique en fin de session precedente
Sortie: execution de la commande
*/
void myExecn(int n, int descHist, int lastHistLine)
{
	int resRead, curLine = 1, i=0;
	char buf[1], newCommande[TAILLE_COM];
	lseek(descHist, 0, SEEK_SET);
	/*recherche de la ligne d indice n*/
	while(curLine < n && (resRead = read(descHist, buf, 1)) > 0)
	{
		if(buf[0] == '\n')
			curLine++;
	}
	if(curLine != n)
		printf("!%d: entree inexistante\n", n);
	else
	{
		/*copie de la ligne puis execution de la commande*/
		while((resRead = read(descHist, buf, 1)) > 0 && buf[0] != '\n')
			newCommande[i++] = buf[0];
		newCommande[i] = '\0';
	}
	execCommande(newCommande, descHist, lastHistLine);
}

/*
Fonction de creation et mise a jour de fichiers

Entree: le TCP
*/
void myTouch(char** tabCom)
{
	int i, m = 0, n;
	struct utimbuf timebuf;
	struct stat statbuf;
	for(i=1; tabCom[i] != NULL; i++)
	{
		if(strcmp("-m", tabCom[i]) == 0)
			m = 1;
	}
	if(!m) /*Sans specification, tentative de creation du fichier*/
	{
		close(open(tabCom[1], O_CREAT));
	}
	else
	{
		for(n=1; strcmp("-m", tabCom[n]) == 0; n++){}
		if(stat(tabCom[n], &statbuf) != -1)
		{
			/*Mise a jour de la date de modification uniquement*/
			timebuf.actime = statbuf.st_atime;
			timebuf.modtime = time(NULL);
			utime(tabCom[n], &timebuf);
		}
	}
}

/*
Fonction globale de gestion de l execution d une commande

Entree: commande en chaine de caracteres, descripteur de fichiers de l historique, derniere ligne de l historique en fin de session precedentes
*/
int execCommande(char *c, int descHist, int lastHistLine)
{
	int pid, i, j, flag = 0, hasPipe = 0, input = -1, output = -1, stdinput, stdoutput, pipefd[2], pid2;
	char **tabCom = (char**) malloc(NOMBRE_ARG*sizeof(char*));
	char **parsedPath = (char**) malloc(128*sizeof(char*));
	char fullPath[4096], currentPath[64], commandePipe1[TAILLE_COM], commandePipe2[TAILLE_COM];
	for(i = 0; i<NOMBRE_ARG; i++)
	{
		tabCom[i] = (char*) malloc(TAILLE_COM*sizeof(char));
	}
	
	for(i = 0; i<128; i++)
	{
		parsedPath[i] = (char*) malloc(64*sizeof(char));
	}

	/*Parsing du PATH puis de la commande*/
	strcpy(fullPath, getenv("PATH"));
	pathParser(fullPath, parsedPath);
	parser(c, tabCom);
	
	/*Verification de la presence de pipes*/
	for(i = 0; tabCom[i] != NULL; i++)
	{
		if(strcmp(tabCom[i], "|") == 0)
			hasPipe = 1;
	}
	
	/*Gestion des redirections*/
	stdinput = dup(0);
	stdoutput = dup(1);
	for(i=0; tabCom[i] != NULL; i++)
	{
		if(strcmp(">", tabCom[i]) == 0)
		{
			output = open(tabCom[++i], O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			/*La suppression de ces elements est necessaire au fonctionnement de execv()
			qui ne prend pas en compte les redirections*/
			tabCom[i] = NULL;
			free(tabCom[i]);
			tabCom[i-1] = NULL;
			free(tabCom[i-1]);
		}
		if(tabCom[i] != NULL && strcmp(">>", tabCom[i]) == 0)
		{
			output = open(tabCom[++i], O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IRGRP | S_IWGRP | S_IWUSR);
			tabCom[i] = NULL;
			free(tabCom[i]);
			tabCom[i-1] = NULL;
			free(tabCom[i-1]);
		}
		if(tabCom[i] != NULL && strcmp("<", tabCom[i]) == 0)
		{
			input = open(tabCom[++i], O_RDONLY);
			tabCom[i] = NULL;
			free(tabCom[i]);
			tabCom[i-1] = NULL;
			free(tabCom[i-1]);
		}
	}
	/*Si des redirections ont ete trouvees, les entrees et sorties standard sont redirigees*/
	if(input != -1)
	{
		dup2(input, 0);
		close(input);
	}
	if(output != -1)
	{
		dup2(output, 1);
		close(output);
	}
		
	/*Gestion de la commande cd*/
	if(!hasPipe && strcmp("cd", tabCom[0]) == 0)
	{
		chdir(tabCom[1]);
		flag = 1;
	}
	
	/*Gestion de la commande history*/
	if(!hasPipe && strcmp("history", tabCom[0]) == 0)
	{
		myHistory(tabCom, descHist, lastHistLine);
		flag = 1;
	}
	
	/*Gestion de la commande cat*/
	if(!hasPipe && strcmp("cat", tabCom[0]) == 0)
	{
		myCat(tabCom);
		flag = 1;
	}
	
	/*Gestion de la commande cp*/
	if(!hasPipe && strcmp("cp", tabCom[0]) == 0)
	{
		myCp(tabCom);
		flag = 1;
	}
	
	/*Gestion de la commande touch*/
	if(!hasPipe && strcmp("touch", tabCom[0]) == 0)
	{
		if(tabCom[1] != NULL)
			myTouch(tabCom);
		flag = 1;
	}

	/*Gestion de la commande !n*/
	if(!hasPipe && tabCom[0][0] == '!')
	{
		tabCom[0][0] = '0';
		i = strtol(tabCom[0], (char**)NULL, 10);
		if(i != 0)
		{
			myExecn(i, descHist, lastHistLine);
		}
		flag = 1; 
	}
	
	/*Gestion des autres commandes*/
	if(!flag)
	{
		/*Execution d une commande sans pipes*/
		if(!hasPipe)
		{
			pid = fork();
	
			if(pid == 0)
			{
				/*Boucle de recherche du PATH a utiliser suivie de l execution de la commande*/
				for(i = 0; parsedPath[i] != NULL; i++)
				{
					strcpy(currentPath, parsedPath[i]);
					strcat(currentPath, "/");
					strcat(currentPath, tabCom[0]);
					if(access(currentPath, F_OK) != -1)
					{
						execv(currentPath, tabCom);
						break;
					}
				}
				exit(0);
			}
			else
			{
				wait(0);
			}
		}
		else	/*Gestion des commandes avec pipes*/
		{
			/*La commande est separee en deux nouvelles commandes autour du premier pipe rencontre*/
			for(i = 0; !(c[i] == ' ' && c[i+1] == '|'); i++)
				commandePipe1[i] = c[i];
			for(j = i+3; c[j] != '\0'; j++)
				commandePipe2[j-i-3] = c[j];
			/*Creation du pipe*/
			pipe(pipefd);
			pid2 = fork();
			/*On execute ensuite les 2 nouvelles commandes par un appel recursif de execComande().
			Cela permet de gerer les pipelines.*/
			if(pid2 == 0)
			{
				/*Duplication de la sortie du pipe, puis fermeture des extremites*/
				dup2(pipefd[1], 1);
				close(pipefd[0]);
				close(pipefd[1]);
				execCommande(commandePipe1, descHist, lastHistLine);
				exit(0);
			}
			else
			{
				/*Duplication de l entree du pipe, puis fermeture des extremites*/
				dup2(pipefd[0], 0);
				close(pipefd[0]);
				close(pipefd[1]);
				execCommande(commandePipe2, descHist, lastHistLine);
				wait(0);
			}
		}
	}
	
	/*Liberation de la memoire et fermeture des descripteurs de fichiers*/
	for(i=0; i<NOMBRE_ARG; i++)
	{
		tabCom[i] = NULL;
		free(tabCom[i]);
	}
	tabCom = NULL;
	free(tabCom);
	
	for(i=0; i<128; i++)
	{
		parsedPath[i] = NULL;
		free(parsedPath[i]);
	}
	parsedPath = NULL;
	free(parsedPath);
	/*Retablissement de l entree et de la sortie standard de la console*/
	dup2(stdinput, 0);
	dup2(stdoutput, 1);
	close(stdinput);
	close(stdoutput);
	
	return 0;
}

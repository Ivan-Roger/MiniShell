/*--------------------------------------------------------------------------
 * headers à inclure afin de pouvoir utiliser divers appels systèmes
 * -----------------------------------------------------------------------*/
#include <stdio.h>     // pour printf() and co
#include <stdlib.h>    // pour exit()
#include <errno.h>     // pour errno and co
#include <unistd.h>    // pour pipe
#include <sys/types.h> // pour avoir pid_t
#include <signal.h>    // pour sigaction
#include <string.h>    // pour avoir strcmp and co

#include <sys/wait.h>  // pour avoir wait and co

#include "jobs.h"
#include "externes.h"

/*-------------------------------------------------------------------------------
 * Macro pour éviter le warning "unused parameter" dans une version intermédiaire
 * -----------------------------------------------------------------------------*/
//#define UNUSED(x) (void)(x)

// Macro pour la taille d'un tableau
//#define ARRAY_SIZE(x) (int)(sizeof(x)/sizeof(x[0]))


/*--------------------------------------------------------------------------
 * crée un fils pour exécuter la commande ligne_analysee->commandes[num_comm]
 * enregistre son pid dans job->pids[num_comm]
 * le fils définit ses handlers à différents signaux grâce à sig
 * -----------------------------------------------------------------------*/
static void execute_commande_dans_un_fils(job_t* job,int num_comm, ligne_analysee_t* ligne_analysee, struct sigaction* sig)
{
  if (num_comm+1 < ligne_analysee->nb_fils) { // On créér le tube uniquement si il y a plus d'une commande
    printf("    > Pipe crée\n");
    if (pipe(job->tubes[num_comm])==-1) {perror("Echec création tube"); exit(errno);}
  } else {
    printf("    > Pas de pipe\n");
  }

  pid_t res_f = fork(); // On crée le fils
  if (res_f==0) { // Si on est dans le fils :

    sig->sa_handler=SIG_DFL; // Remise a zero des handlers de signaux
    sigaction(SIGINT,sig,NULL);

    printf("    Execution commande %d/%d : '%s' (pid: %d)\n",num_comm+1,ligne_analysee->nb_fils,ligne_analysee->commandes[num_comm][0],getpid());/*
    if (ligne_analysee->nb_fils>1) { // On ne crée un tube que si il y a plus d'une commande
      if (num_comm == 0) { // On initialise le tube en fonction de la position de la commande dans la ligne
        //gerer_tube_premier_fils(job,num_comm);
          printf("    Tube debut OK\n");
          close(job->tubes[num_comm][0]); //le premier fils ne lit pas dans le tube suivant
          dup2(job->tubes[num_comm][1], STDOUT_FILENO); //le premier fils lit depuis stdin et écrit dans le tube suivant
      } else if (num_comm == (ligne_analysee->nb_fils)-1) {
        //gerer_tube_dernier_fils(job,num_comm);
          printf("    Tube fin OK\n");
          close(job->tubes[num_comm-1][1]); //le dernier fils n'écrit pas dans le tube
          dup2(job->tubes[num_comm-1][0],STDIN_FILENO); //le dernier fils lit depuis le tube et écrit dans stdout
      } else {
        //gerer_tube_fils_intermediaire(job,num_comm);
        if (pipe(job->tubes[num_comm])==-1) {perror("Echec création tube"); exit(errno);} // TEST
        printf("    Tube centre OK\n");
        close(job->tubes[num_comm-1][1]); // On n'ecris pas dans le tube précedent
        dup2(job->tubes[num_comm-1][0], STDIN_FILENO); // On lit depuis la sortie du tube précédent
        close(job->tubes[num_comm][0]); // On ne lit pas dans le tube suivant
        dup2(job->tubes[num_comm][1], STDOUT_FILENO); // On écrit dans l'entrée du tube suivant
      }
    }*/
    if (num_comm == 0) { // On initialise le tube en fonction de la position de la commande dans la ligne
      //gerer_tube_premier_fils(job,num_comm);
        printf("    Tube debut OK\n");
        close(job->tubes[0][0]); //le premier fils ne lit pas dans le tube suivant
        dup2(job->tubes[0][1], STDOUT_FILENO); //le premier fils lit depuis stdin et écrit dans le tube suivant
    } else if (num_comm == 1) {
      //gerer_tube_dernier_fils(job,num_comm);
        printf("    Tube fin OK\n");
        close(job->tubes[0][1]); //le dernier fils n'écrit pas dans le tube
        dup2(job->tubes[0][0],STDIN_FILENO); //le dernier fils lit depuis le tube et écrit dans stdout
    }

    int res_e = execvp(ligne_analysee->commandes[num_comm][0],ligne_analysee->commandes[num_comm]); // On execute la commande avec les arguments
    if (res_e==-1) {perror("Echec execvp"); exit(errno);}
  }

  job->pids[num_comm] = res_f; // On enregistre le numéro du fils;
}
/*--------------------------------------------------------------------------
 * Fait exécuter les commandes de la ligne par des fils
 * -----------------------------------------------------------------------*/
void executer_commandes(job_t* job, ligne_analysee_t *ligne_analysee, struct sigaction *sig)
{
  // recopie de la ligne de commande dans la structure job
  strcpy(job->nom,ligne_analysee->ligne);

  printf("Execution des %d commandes (ppid: %d)\n\n",ligne_analysee->nb_fils,getpid());

  for (int i=0; i<ligne_analysee->nb_fils; i++) {
    // on lance l'exécution de la commande dans un fils
    execute_commande_dans_un_fils(job,i,ligne_analysee, sig);
  }
  for (int i=0; i<ligne_analysee->nb_fils; i++) {
    pid_t res_w = waitpid(job->pids[i],NULL,0);
    if (res_w==-1) {perror("Echec wait"); exit(errno);}
    printf("    --> Commande %d/%d terminée (pid: %d)\n",i+1,ligne_analysee->nb_fils,(int)res_w);
    close (job->tubes[i][0]); // On ferme la lecture ...
    close (job->tubes[i][1]); // ... et l'ecriture une fois le fils terminé
  }
  printf("Execution terminée\n");
  // on ne se sert plus de la ligne : ménage
  *ligne_analysee->ligne='\0';
}

/*--------------------------------------------------------------------------
 * Gestion de la sortie du premier fils
 * -----------------------------------------------------------------------*/
void gerer_tube_premier_fils(job_t *job, int num_comm) {
  printf("    Tube debut OK\n");
  close(job->tubes[num_comm][0]); //le premier fils ne lit pas dans le tube suivant
  dup2(job->tubes[num_comm][1], STDOUT_FILENO); //le premier fils lit depuis stdin et écrit dans le tube suivant
  // DEBUG : Affiche l'entrée et la sortie
  FILE* finfo=fopen("/dev/pts/4","w");
  fprintf(finfo," IN: %d\r\nOUT: %d\r\n",STDIN_FILENO,STDOUT_FILENO);
  fclose(finfo);
}

/*--------------------------------------------------------------------------
 * Gestion de l'entrée et la sortie d'un fils intermédiaire
 * -----------------------------------------------------------------------*/
void gerer_tube_fils_intermediaire(job_t *job, int num_comm){
  printf("    Tube centre OK\n");
  dup2(job->tubes[num_comm-1][0], STDIN_FILENO); //un fils intermédaire lit depuis la sortie du tube précédent...
  dup2(job->tubes[num_comm][1], STDOUT_FILENO); //...et écrit dans l'entrée du tube suivant
  // DEBUG : Affiche l'entrée et la sortie
  FILE* finfo=fopen("/dev/pts/4","w");
  fprintf(finfo," IN: %d\r\nOUT: %d\r\n",STDIN_FILENO,STDOUT_FILENO);
  fclose(finfo);
}

/*--------------------------------------------------------------------------
 * Gestion de l'entrée du dernier fils
 * -----------------------------------------------------------------------*/
void gerer_tube_dernier_fils(job_t *job, int num_comm){
  printf("    Tube fin OK\n");
  close(job->tubes[num_comm-1][1]); //le dernier fils n'écrit pas dans le tube
  dup2(job->tubes[num_comm-1][0],STDIN_FILENO); //le dernier fils lit depuis le tube et écrit dans stdout
  // DEBUG : Affiche l'entrée et la sortie
  FILE* finfo=fopen("/dev/pts/4","w");
  fprintf(finfo," IN: %d\r\nOUT: %d\r\n",STDIN_FILENO,STDOUT_FILENO);
  fclose(finfo);
}

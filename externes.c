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
#define UNUSED(x) (void)(x)
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))


/*--------------------------------------------------------------------------
 * crée un fils pour exécuter la commande ligne_analysee->commandes[num_comm]
 * enregistre son pid dans job->pids[num_comm]
 * le fils définit ses handlers à différents signaux grâce à sig
 * -----------------------------------------------------------------------*/
static void execute_commande_dans_un_fils(job_t *job,int num_comm, ligne_analysee_t *ligne_analysee, struct sigaction *sig)
{
  sig->sa_flags=0;
  sigemptyset(&sig->sa_mask);

  if (num_comm < ligne_analysee->nb_fils) { // On créér le tube uniquement si le fils qui doit être crée n'est pas le dernier.
    if (pipe(job->tubes[num_comm])==-1)
      {perror("Echec création tube"); exit(errno);}
  }

  pid_t res_f = fork(); // On crée le fils
  if (res_f==0) { // Si on est dans le fils :
    sig->sa_handler=SIG_DFL;
    sigaction(SIGINT,sig,NULL);

    int res_e = execvp(*ligne_analysee->commandes[num_comm],*ligne_analysee->commandes); // On execute la commande avec les arguments
    if (res_e==-1) {perror("Echec execvp"); exit(errno);}
  }
  job->pids[num_comm] = res_f; // On enregistre le numéro du fils;
}
/*--------------------------------------------------------------------------
 * Fait exécuter les commandes de la ligne par des fils
 * -----------------------------------------------------------------------*/
void executer_commandes(job_t *job, ligne_analysee_t *ligne_analysee, struct sigaction *sig)
{
  // recopie de la ligne de commande dans la structure job
  strcpy(job->nom,ligne_analysee->ligne);

  for (int i=0; i<ARRAY_SIZE(ligne_analysee->commandes); i++) {

    // on lance l'exécution de la commande dans un fils
    execute_commande_dans_un_fils(job,i,ligne_analysee, sig);

    pid_t res_w = waitpid(job->pids[i],NULL,0);
    if (res_w==-1) {perror("Echec wait"); exit(errno);}

  }
  // on ne se sert plus de la ligne : ménage
  *ligne_analysee->ligne='\0';
}

/*--------------------------------------------------------------------------
 * Gestion de la sortie du premier fils
 * -----------------------------------------------------------------------*/
void gerer_tube_premier_fils(job_t *job, int num_comm) {

  close(job->tubes[num_comm][0]); //le premier fils ne lit pas dans le tube
  dup2(job->tubes[num_comm][1], STDOUT_FILENO); //le premier fils lit depuis stdin et écrit dans le tube
}

/*--------------------------------------------------------------------------
 * Gestion de l'entrée et la sortie d'un fils intermédiaire'
 * -----------------------------------------------------------------------*/
void gerer_tube_fils_intermediaire(job_t *job, int num_comm){

  dup2(job->tubes[num_comm-1][0], STDIN_FILENO); //un fils intermédaire lit depuis la sortie du tube précédent...
  dup2(job->tubes[num_comm][1], STDOUT_FILENO); //...et écrit dans l'entrée du tube suivant
}

/*--------------------------------------------------------------------------
 * Fait exécuter les commandes de la ligne par des fils
 * -----------------------------------------------------------------------*/
void gerer_tube_dernier_fils(job_t *job, int num_comm){
  close(job->tubes[num_comm-1][1]); // Je ne fais que lire, je ferme l'ecriture
  dup2(job->tubes[num_comm-1][0],STDIN_FILENO); // Je transforme la sortie du tube en entrée standard
}

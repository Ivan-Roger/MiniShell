/*--------------------------------------------------------------------------
 * headers à inclure afin de pouvoir utiliser divers appels systèmes
 * -----------------------------------------------------------------------*/
#include <sys/types.h> // pour pid_t
#include <unistd.h>    // pour pid_t
#include <signal.h>    // pour kill
#include <stdio.h>     // pour printf and co
#include <stdlib.h>    // pour exit

#include "jobs.h"

/*--------------------------------------------------------------------------
 * header à inclure afin de pouvoir exécuter des commandes externes
 * -----------------------------------------------------------------------*/
#include "externes.h"

/*-------------------------------------------------------------------------------
 * Macro pour éviter le warning "unused parameter" dans une version intermédiaire
 * -----------------------------------------------------------------------------*/
#define UNUSED(x) (void)(x)

/*--------------------------------------------------------------------------
 * fonction qui initialise les structures de contrôle des jobs
 * -----------------------------------------------------------------------*/
void initialiser_jobs(job_set_t *mes_jobs)
{
  for (int j=0; j<NB_MAX_JOBS; j++)
  {
    mes_jobs->jobs[j].nom[0]='\0';
    mes_jobs->jobs[j].suspendu = 0; // Le job n'est pas suspendu par défaut
    for (int p=0; p<NB_MAX_COMMANDES+1; p++)
      mes_jobs->jobs[j].pids[p]=-2; // -1 signifie n'importe quel pid pour waitpid()
  }
  mes_jobs->job_fg=-2;
}


/*--------------------------------------------------------------------------
 * action sur un job : envoi d'un signal sur tous ses fils vivants + mesg info
 * exemple d'utilisation : voir la commande interne mon_kill() dans internes.c
 * remarque : l'argument text peut être utilisé dans le message d'information
 *            en particulier pour différentier un réveil en avant-plan
 *            d'un réveil en arrière-plan.
 * -----------------------------------------------------------------------*/
void action_job(int j, job_t job, int signalT, const char*text)
{
  // Envoi du signal à tous les fils du job j...
  for (int p=0; job.pids[p]!=-2 ;p++)
  {
    //...qui n'ont pas encore terminé leur exécution
    if (job.pids[p]!=0) {
      kill(job.pids[p],signalT);
    }
  }

  // Affichage d'un message d'information sur stdout
  switch (signalT) {
  case SIGKILL :
    printf("\n[%i] Terminaison \t %s \n",j,job.nom);
    break;
  case SIGSTOP :
    printf("\n[%i] Suspension \t %s \n",j,job.nom);
    break;
  default :
    printf("Signal inconnu \n");
  }

  // A supprimer pour la version à multiples jobs
  UNUSED(text);
}


/*--------------------------------------------------------------------------
 * Renvoie l'adresse de la structure associée à l'exécution de ce job
 * et gère le statut d'avant-plan ou d'arrière-plan de l'exécution
 * -----------------------------------------------------------------------*/
job_t * preparer_nouveau_job(int isfg, char* ligne, job_set_t *mes_jobs)
{
  UNUSED(ligne);
  int num_job=0;
  while (num_job<NB_MAX_JOBS && mes_jobs->jobs[num_job].pids[0]==-2)
    num_job++;
  if (num_job>=NB_MAX_JOBS) {
    return NULL;
  }
  if (isfg!=0) // si le dernier mot de la ligne n'est pas "&" on exeute en avant plan
    mes_jobs->job_fg=num_job;
  return &(mes_jobs->jobs[num_job]);

}

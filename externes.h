#ifndef EXTERNES_H
#define EXTERNES_H

/*--------------------------------------------------------------------------
 * header à inclure pour les constantes et types spéciaux
 * -----------------------------------------------------------------------*/
#include "mini_shell.h"


/*--------------------------------------------------------------------------
 * Fait exécuter les commandes de la ligne par des fils
 * Utilisée dans mini_shell.c
 * -----------------------------------------------------------------------*/
void executer_commandes(job_t *job, ligne_analysee_t *ligne_analysee, struct sigaction *sig);
void gerer_tube_premier_fils();
void gerer_tube_fils_intermediaire();
void gerer_tube_dernier_fils();

#endif

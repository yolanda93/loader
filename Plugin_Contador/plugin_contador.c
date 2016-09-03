#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "../plugin_info.h"

#define TEMPORIZADOR 3
#define BANNER       "PluginContador"

/* Variable local con informacion del plugin */
static plugin_info_t* info;

/* Variable global del contador */
int contador=0;

static void loop_contador()
{
  struct timespec tic;

  /* Espero TEMPORIZADOR segundos */
  tic.tv_sec=TEMPORIZADOR;
  tic.tv_nsec=0;

  while(nanosleep(&tic,&tic));

  fprintf(stdout,"[%s]: %d\n",info->banner,++contador);
  fflush(stdout);
}

static void stop_contador()
{
  fprintf(stdout,"[%s]: Parado\n",info->banner);
  fflush(stdout);
}

static void resume_contador(int instancia)
{
  fprintf(stdout,"[%s]: Continua\n",info->banner);
  fflush(stdout);
}

static int finish_contador(int instancia)
{
  fprintf(stdout,"[%s]: Finalizado\n",info->banner);
  fflush(stdout);

  return (contador);
}

/* Funcion del interfaz, obtiene la estructura del modulo */
extern plugin_info_t* plugin_info()
{
  info=(plugin_info_t*)malloc(sizeof(plugin_info_t));

  info->banner             =strdup(BANNER);
  info->plugin_loop        =loop_contador;
  info->plugin_stop        =stop_contador;
  info->plugin_resume      =resume_contador;
  info->plugin_finish      =finish_contador;

  return info;
}

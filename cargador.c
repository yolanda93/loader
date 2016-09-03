#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>      //para señales
#include <sys/wait.h>    // para waitpid
#include <dlfcn.h>       //para el uso de dlopen
#include <time.h>        // para nanosleep
#include "./plugin_info.h" //interfaz de plugin_contador

/*...........................................................................................Declaración de funciones */
/* Funcion auxiliar para leer los mandatos */
void leer_mandatos(FILE*, int con_prompt, int procesos);                                                                                        
void mandato_load  (char* arg);
void mandato_start1 ();
void mandato_start2 ();
void mandato_stop1();
void mandato_stop2();
void mandato_sleep (char* arg);
void mandato_resume1();
void mandato_resume2(); //thread
void mandato_echo  (char* arg);
void mandato_finish1();
void mandato_finish2();
// tratamiento de señales
void HandlerFinalizar(int signo);
void HandlerStop(int signo);
void HandlerResume(int signo); 
void bucleThread();
void HandlerFinalizarThreads(int signo);

/*.................................................................................. Definición de variables globales */
#define VERDADERO 1
#define FALSO     0
#define PROCESS   1
#define THREADS   0
                                                    
plugin_info_t* (*plugin_info_func)();//plugin_info es un puntero a una función de tipo struct

struct sigaction act;
struct sigaction act1;   
struct sigaction act2; 
struct sigaction act3;// Threads 
int contInstancias = 0; //contador de instancias
int numInstancia; // numero de instancia
pid_t pidInstancia[100];
pthread_t pidInstanciaTh[100];
pid_t pid;
int finalizar; //Valor devuelto por plugin_finish
int finalizarThread;
int finalizado;
int valorRetorno; //Valor devuelto por exit
static plugin_info_t* info; // variable global
int instanciaStop;
int numeroInstanciaR; //numero de instancia de resume
int numeroInstanciaF;
void *puntero; //puntero a la biblioteca dinámica.
// Variables globales de los procesos hijos.
int instanciaStopChild;
int numeroInstanciaRChild;
int pauseThread;         
int status;    
int statusThread; 
int argumentoSleep;

pthread_mutex_t mutex;    /*mutex para controlar el acceso al bufer compartido*/
pthread_cond_t  parada;   /*esperar si han solicitado parada*/

/*................................................................................ FUNCION MAIN DEL PROGRAMA CARGADOR */
int main(int argc, char* argv[])
{
  // Armado de señales del padre
 act.sa_handler = HandlerFinalizar; /* función a ejecutar */
 act.sa_flags = 0;               /* ninguna acción especifica*/
 /* Se bloquean las señales */
 sigemptyset(&act.sa_mask);        
 
 act1.sa_handler = HandlerStop; /* función a ejecutar */
 act1.sa_flags = 0;               /* ninguna acción especifica*/
 /* Se bloquean las señales */
 sigemptyset(&act1.sa_mask);              
 
 act2.sa_handler = HandlerResume; /* función a ejecutar */
 act2.sa_flags = 0;               /* ninguna acción especifica*/
 /* Se bloquean las señales */
 sigemptyset(&act2.sa_mask);    
 
  act3.sa_handler = HandlerFinalizarThreads; /* función a ejecutar */
 act3.sa_flags = 0;               /* ninguna acción especifica*/
 /* Se bloquean las señales */
 sigemptyset(&act3.sa_mask);  
 
 
  FILE* fich;
  int valor;
   if(argc<2 || !strcmp(argv[1],"--help"))
   {
    fprintf(stderr,
    "%s --help: Muestra la ayuda\n"
    "%s [-t|-p] [fichero_ordenes]\n",argv[0],argv[0]);
    return 0;
   }
/* Aqui se deben incluir las funciones necesarias para resolver los
*      diferentes apartados de la practica. Se recomienda:
*              - Sea ordenado y metodico en la codificación de las
*                        funcionalidades
*                                - Si el programa va a ser complejo, utilice varios ficheros
*                                          de implementacion .c y defina los ficheros de cabecera .h
*                                                    con el prototipo de las funciones.
*                                                            - Antes de preguntar al profesor (en tutorias, correo
*                                                                      electronico o via el foro) depure bien el problema e intente
*                                                                                acotarlo. No pregunte "Esto no me funciona...", diga donde,
*                                                                                          por que y en que casos falla.
*                                                                                            */

   if(!strcmp(argv[1],"-p" ))     /*lanzamos Procesos*/
       valor = PROCESS;
    else                           /*lanzamos threads*/
       valor = THREADS;        
       
 if(argc==3)
 {   
   fich=fopen(argv[2],"r");
   if(fich==NULL)
   {
   fprintf(stderr,
   "%s --help: Muestra la ayuda\n"
   "%s [-t|-p] [ficher_ordenes]\n",argv[0],argv[0]);
   return 0;
   }
  leer_mandatos(fich,FALSO,valor);         /* Cargando fichero de ordenes */
 }
 else
  leer_mandatos(stdin,VERDADERO,valor);     /* Modo interactivo: prompt */
  
  dlclose(puntero);
  
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&parada);
  
 return 0;
 
}    //end main

/* Lee del fichero indicado (puede ser la entrada estandar [stdin])
*    una secuencia de mandatos. Por cada uno invoca a la funcion correspondiente */




void leer_mandatos(FILE* fich, int prompt, int procesos)
{
  int salir=0;
  char linea[1024];
  char mandato[1024];
  char argumento[1024];

 while(!salir)
 {
   if(prompt)//modo interactivo.
   fprintf(stdout,"cargador> ");

  if(fgets(linea,1024,fich))//leemos del fichero una linea, en el caso de modo interactivo el fich es la entrada estándar.
  {
   bzero(argumento,sizeof(argumento)); // como memset pero este los inicializa a cero
   sscanf(linea,"%s %[^\n]",mandato,argumento);//guardamos en variables el mandato y argumento.

   if     (!strcmp(mandato,"load" ))
    mandato_load(argumento);
   else if(!strcmp(mandato,"start"))
   {
    if(procesos) 
    mandato_start1();
    else
    mandato_start2();
   }
    else if(!strcmp(mandato,"stop"  ))
     {
      if(procesos) 
      mandato_stop1(argumento);
     else
      mandato_stop2(argumento);
      }
    else if(!strcmp(mandato,"sleep" ))
     mandato_sleep(argumento);
    else if(!strcmp(mandato,"resume"))
    {
      if(procesos)
     mandato_resume1(argumento);
      else
      mandato_resume2(argumento);
      }
    else if(!strcmp(mandato,"finish"))
    {
      if(procesos)
     mandato_finish1(argumento);
      else
      mandato_finish2(argumento);
     }
    else if(!strcmp(mandato,"echo"  ))
     mandato_echo(argumento);
    else if(!strcmp(mandato,"quit"  ))
     salir=1; /* Salimos de la linea de mandatos */
    else
     fprintf(stderr,"Mandato: '%s' no valido\n",mandato);
}
} //end while
}



/* Funciones que el alumno debe implementar */
/*........................................................................................................Mandato load*/
 void mandato_load  (char* arg)
{

   //abrimos la libreria.
   puntero = dlopen (arg, RTLD_LAZY);

  // Tramiento de errores: Error(1), No se puede cargar
  char *error;
  if (!puntero){
   fprintf (stderr, "%s: Error(1), No se puede cargar\n", arg);
   exit(1);
 }
  //Buscamos el símbolo plugin_info

plugin_info_func=dlsym(puntero, "plugin_info");

  // Tramiento de errores: Error(2), No se encuentra el simbolo
  if ((error = dlerror()) != NULL)  {
  fprintf (stderr, "%s: Error(2), No se encuentra el simbolo\n", arg);
  exit(1);
  }
  info = (*plugin_info_func)();  //ejecutamos la función. Reasignar la dirección de la variable global al puntero global ejecutando la función
  

fprintf(stderr,"Modulo %s cargado correctamente\n",arg);

}


/*..............................................................................................Mandato start(Process)*/
void mandato_start1()  
{
                                 
 pid_t pid;
 contInstancias++; // Cuántas instancias llevamos             
 numInstancia =contInstancias-1; // El número de instancia que hemos iniciado.
 // Obtenemos el pid del proceso que arranca la instancia.
     pid= fork();
 // Lo guardamos para poder mandar la señal a cada una de las instancias.
     pidInstancia[numInstancia] = pid;
   switch(pid)
   {
      case -1: 
            fprintf (stderr, "%d: Error(5), Error al crear la instancia\n", numInstancia);   
            exit(1);
      case 0:             
          //Variables locales de cada instancia.
          //  int instanciaStopChild;
          //  int numeroInstanciaRChild;
          
         
         while(finalizar==0)                       //Instancia arrancada como un proceso
         {  
          
             //Si recibe SIGTERM llamar a plugin_finish, el valor devuelto lo usamos en exit.
            // Recepción de señales enviadas por el padre  
            sigaction(SIGTERM, &act, NULL);
            sigaction(SIGCONT, &act1, NULL);
            sigaction(SIGUSR1, &act2, NULL);
            
          //Justo antes de detener un proceso, el proceso/thread que ejecuta la instancia debería llamar a la funcion plugin stop
          while((instanciaStopChild!=NULL)&&!(numeroInstanciaRChild!=NULL))    // llamamos a plugin_stop si hemos ejecutado como argumento stop
                                                                 // Miramos que numeroInstancia sea igual a NULL, ya que si no habremos ejecutado resume.
           {
           info->plugin_stop();              
           //hasta que numeroInstancia no tenga algún valor, es decir, se invoque resume no continuamos. Señal SIGCONT
             pause(); //paramos la instancia     
                        
           }
           if(numeroInstanciaRChild!=NULL) // Si hemos ejecutado resume, hay que llamar a plugin_resume()
           info->plugin_resume();     
           
         
                                              
             info->plugin_loop();
          
	  

         }
           
         
      default:
    
           fprintf(stderr,"Modulo iniciado: %d\n",numInstancia); // 0,1,2,3..  
           fprintf(stderr,"Numero de modulos iniciados: %d\n",contInstancias);  // 1,2,3,4,5...        
               
      }
      
       
   
}                                                                                       
/*..............................................................................................Mandato start(Threads)*/
/*En este caso el recurso compartido es el acceso a la función plugin_loop(), si paramos una instancia debemos ceder el turno*/
 void mandato_start2()                                                                    
{
 
  contInstancias++; // Cuántas instancias llevamos             
  numInstancia =contInstancias-1; // El número de instancia que hemos iniciado.
  pthread_mutex_init(&mutex,NULL);
  pthread_cond_init(&parada,NULL); 
  
  fprintf(stderr,"Modulo iniciado: %d\n",numInstancia);
  fprintf(stderr,"Numero de modulos iniciados: %d\n",contInstancias);
  
   pthread_create(&pid,NULL,bucleThread, NULL);
   
    if(numeroInstanciaR!=0)
       {
     
        pthread_cond_signal(&parada);  
       }  
   
   /*
    pthread_mutex_lock(&mutex); //accedemos en exclusión mutua
    while(finalizar==0){  
     fprintf("estamos dentro del while");
    sigaction(SIGTERM, &act3, NULL);
    while((instanciaStop!=0)&&!(numeroInstanciaR!=0))// hemos solicitado parada
    {  
      fprintf("cedemos el acceso en exclusión mútua");
      pthread_cond_wait(&parada,&mutex);                   // cedemos el acceso en exclusión mútua
      info->plugin_stop();
      pause(); //paramos la instancia

    }

    if(numeroInstanciaR!=0)
    {
    info->plugin_resume();
    pthread_cond_signal(&parada);  
    }
    if(argumentoSleep!=0)
    {
      mandato_sleep(argumentoSleep);    
    fprintf("no duerme %d", argumentoSleep);
    }
    pause();
    pthread_create(&pidInstancia[numInstancia],NULL,(void * (*)(void *))info->plugin_loop, NULL); //lanzamos el thread, hacemos casting
    }
  
   pthread_mutex_unlock(&mutex);  
   */
  
  


}
/*........................................................................................................Mandato stop*/
void mandato_stop1(char* arg)
{
  instanciaStop = atoi(arg);

  //Mandar la señal SIGUSR1 al proceso para que se pare.    
    if ( kill(pidInstancia[numeroInstanciaR], SIGUSR1)==-1) { 
    perror("fallo en kill");                                  
    exit(EXIT_FAILURE);                                       
    }                                                         
fprintf(stderr,"Modulo detenido: %d\n",instanciaStop);
 
}

//threads

void mandato_stop2(char* arg)
{
  instanciaStop = atoi(arg);

  //Mandar la señal SIGUSR1 al proceso para que se pare.    
    if ( kill(pidInstancia[numeroInstanciaR], SIGUSR1)==-1) { 
    perror("fallo en kill");                                  
    exit(EXIT_FAILURE);                                       
    }                                                         
fprintf(stderr,"Modulo detenido: %d\n",instanciaStop);
 
}
/*.......................................................................................................Mandato sleep*/
void mandato_sleep(char* arg)
{
  // Tratamiento de errores a sleep le pasamos un argumento NULL o que no es un entero.
  int second=atoi(arg);
  if (second!=0)
  {
  struct timespec ts;  
   // strtol
  if (arg!=NULL) {
   ts.tv_sec=second;      
   ts.tv_nsec=0;
   while(nanosleep(&ts, &ts));   
   }
  }
  else
  fprintf (stderr, "%s: Error(2), Parametro de mandato no valido\n", arg);
  
}
/*......................................................................................................Mandato resume*/
void mandato_resume1(char* arg)
 {
   // Tratamiento de errores a sleep le pasamos un argumento NULL o que no es un entero.
  numeroInstanciaR=atoi(arg);
    
  //Mandar la señal SIGCONT al proceso para que continue. 
    if ( kill(pidInstancia[numeroInstanciaR], SIGCONT)==-1) {    
    perror("fallo en kill");                                     
    exit(EXIT_FAILURE);                                          
    }                                                            
    fprintf(stderr,"Modulo reiniciado: %d\n", numeroInstanciaR);
 }
 
 void mandato_resume2(char* arg)
 {
   // Tratamiento de errores a sleep le pasamos un argumento NULL o que no es un entero.
  numeroInstanciaR=atoi(arg);
    
  //Mandar la señal SIGCONT al proceso para que continue. 
    if ( pthread_kill(pidInstancia[numeroInstanciaF], SIGCONT)==-1) {    
    perror("fallo en kill");                                     
    exit(EXIT_FAILURE);                                          
    }                                                            
    fprintf(stderr,"Modulo reiniciado: %d\n", numeroInstanciaR);
 }
/*.........................................................................................................Mandato echo*/
void mandato_echo  (char* arg)
 {
  fprintf(stderr,"%s\n",arg);
 }
/*.......................................................................................................Mandato finish*/
   //Se ha ejecutado con -p
void mandato_finish1(char* arg)
 {
   numeroInstanciaF=atoi(arg);
   if(numeroInstanciaF==0)
   numeroInstanciaF = 0;
  //Mandar la señal SIGTERM al proceso que arranca la instancia
   
 
    if ( kill(pidInstancia[numeroInstanciaF], SIGTERM)==-1) {
    perror("fallo en kill");
    exit(EXIT_FAILURE);
    }
     waitpid(pidInstancia[numeroInstanciaF],&status,0);
 
    
  fprintf(stderr,"Modulo finalizado: %d (%d)\n",numeroInstanciaF,WEXITSTATUS(status));
 }
  // Se ha ejecutado con -t          THREADS
 void mandato_finish2(char* arg)
 {
   numeroInstanciaF=atoi(arg);
   if(numeroInstanciaF==0)
   numeroInstanciaF = 0;
   finalizar=1; // variable global compartida con el thread
  //Mandar la señal SIGTERM al proceso que arranca la instancia
   
    //printf("mandato_finish2 \n"); 
     if(pthread_kill(pidInstanciaTh[numeroInstanciaF], SIGTERM)==-1) {
    perror("fallo en kill");
    exit(EXIT_FAILURE);
    }
    //printf("join\n");
    if(pthread_join(pidInstanciaTh[numeroInstanciaF], &statusThread))
    {
      printf("error joining thread.");
      abort();
    }
      if (WIFEXITED(statusThread)) {
     printf("Thread- valor de exit: RC=%d\n",WEXITSTATUS(statusThread));
    }

      
      
      fprintf(stderr,"Modulo finalizado: %d (%d)\n",numeroInstanciaF,WEXITSTATUS(statusThread));    

 }
/*......................................................................................................Funciones Handler*/


  void HandlerFinalizar(int signo) //Manejador de la señal
  {
   //Si recibe SIGTERM llamar a plugin_finish, el valor devuelto lo usamos en exit.
       finalizar=1;
   exit(info->plugin_finish());//En finalizar nos devuelve el contador.

   
  }
  
  // Se ha ejecutado con -t
   void HandlerFinalizarThreads(int signo)// Manejador de la señal
  {
   
   //Si recibe SIGTERM llamar a plugin_finish, el valor devuelto lo usamos en exit.
   finalizarThread=1;

   pthread_exit(info->plugin_finish());
  

  }
  
  void HandlerStop(int signo)
  {
    instanciaStopChild=instanciaStop;
  }       
  
  void HandlerResume(int signo)
  {
    numeroInstanciaRChild=numeroInstanciaR;
  }     
   
        
  void bucleThread()
  { 
   pthread_mutex_lock(&mutex); //accedemos en exclusión mutua
    pidInstanciaTh[numInstancia]= pthread_self();
    
    while(finalizarThread==0){
     sigaction(SIGTERM, &act3, NULL);  // Tratamos la señal SIGTERM
     while((instanciaStop!=0)&&!(numeroInstanciaR!=0))// hemos solicitado parada
      {
        pthread_cond_wait(&parada,&mutex);                   // cedemos el acceso en exclusión mútua
        info->plugin_stop();
        pause(); //paramos la instancia
      }
       if(numeroInstanciaR!=0)
       {
         info->plugin_resume(); 
       }
       
 
     info->plugin_loop();
  
   }
   pthread_mutex_unlock(&mutex); //accedemos en exclusión mutua
  
  }
  

  
// ERROR  4 Finalizacion anomala del plugin

/*--------------------------------------------------------------------------------------------//
PROJETO: PingPongOS - PingPong Operating System
PROPOSTO POR: Prof. Carlos A. Maziero, DINF UFPR
AUTOR DO ARQUIVO: Mateus Felipe de Cássio Ferreira (GRR20176123)

ASSUNTO: Preempção por tempo e Contabilização
//--------------------------------------------------------------------------------------------*/

#include "ppos.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#define STACKSIZE 32768 //tamanho da pilha
//#define DEBUG //descomentar quando precisar fazer depuração

//----------------------------------------------VARIÁVEIS GLOBAIS----------------------------------------------//
task_t taskMain;                //tarefa da main (descritor da tarefa que aponta para o programa principal)
task_t *taskCurrent;            //apontador para a tarefa corrente
task_t taskDispatcher;          //o dispatcher deve ser uma tarefa!
task_t *queueREADY;             //apontador para uma lista da tarefa de prontas
task_t *taskNext;               //apontador para a próxima tarefa a pegar o processador
int id = 0;                     //id da tarefa
int userTasks = 0;              //contador de tarefas de usuário (dispatcher não entra na conta)
unsigned int ticksCounter;      //contador de ticks
unsigned int processorCounter;  //contador usado para calcular o tempo de processamento
//--------------------------------------------------------------------------------------------------------------//


//-------------------------------------------ESTRUTURAS/TIPOS GLOBAIS-------------------------------------------//
// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicialização to timer
struct itimerval timer;
//--------------------------------------------------------------------------------------------------------------//


//--------------------------------------------DECLARAÇÕES DE FUNÇÕES--------------------------------------------//
void dispatcher ();
void ticks_handler ();
unsigned int systime ();
//--------------------------------------------------------------------------------------------------------------//


//--------------------------------------------IMPLEMENTAÇÃO DE FUNÇÕES--------------------------------------------//
void ppos_init () //INICIALIZA AS ESTRUTURAS INTERNAS DO SISTEMA OPERACIONAL
{
  setvbuf (stdout, 0, _IONBF, 0); //COLOCAR: desativa o buffer da saída padrão (stdout), usado pela função printf
  taskMain.prev = NULL;
  taskMain.next = NULL;
  taskMain.id = 0; //por default, a taskMain recebe zero!
  taskMain.taskType = USER; //a tarefa main é uma tarefa de usuário!
  queue_append ((queue_t **) &queueREADY, (queue_t *) &taskMain);
  taskMain.taskState = READY;
  userTasks ++;
  
  task_create (&taskDispatcher, (void*)(dispatcher), NULL); //cria a tarefa do dispatcher 
  
  taskCurrent = &taskMain;

  // registra a ação para o sinal de timer SIGALRM
  action.sa_handler = ticks_handler;
  sigemptyset (&action.sa_mask);
  action.sa_flags = 0 ;
  if (sigaction (SIGALRM, &action, 0) < 0)
  {
    perror ("Erro em sigaction: ");
    exit (1);
  }

  // ajusta valores do temporizador
  timer.it_value.tv_usec = 90000;      // primeiro disparo, em micro-segundos (90000 microssegundos = 0,09 segundos)
  timer.it_value.tv_sec  = 0;          // primeiro disparo, em segundos
  timer.it_interval.tv_usec = 1000;    // disparos subsequentes, em micro-segundos (1000 microssegundos = 1 milissegundo)
  timer.it_interval.tv_sec  = 0;       // disparos subsequentes, em segundos

  // arma o temporizador ITIMER_REAL (vide man setitimer)
  if (setitimer (ITIMER_REAL, &timer, 0) < 0)
  {
    perror ("Erro em setitimer: ") ;
    exit (1) ;
  }

  task_yield ();
}; //TERMINADO


void ticks_handler ()
{
  ticksCounter ++;
  if (taskCurrent->taskType == USER)
  {
    if (taskCurrent->quantum > 0)
      taskCurrent->quantum--;
    else
    {
      task_yield ();
    }
  }
}; //TERMINADO


int task_create (task_t *task, void(*start_routine)(void *), void *arg)
{
  task->taskState = NEW;
  
  char *stack;

  getcontext (&(task->context)); //equivalente ao (&ContextPing), que nesse caso já passava o contexto direto
  stack = malloc (STACKSIZE);
  if (stack)
  {
    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;
    makecontext (&(task->context), (void*)(*start_routine), 1, arg);
    task->id = ++id;

    #ifdef DEBUG
    printf ("task_create: criou tarefa %d\n", task->id);
    #endif

    if (task != &taskDispatcher)
    {
      task->taskType = USER;
      task->static_priority = 0;
      task->dynamic_priority = 0;
      queue_append ((queue_t **) &queueREADY, (queue_t *) task);
      task->taskState = READY;
      userTasks ++;
    }
    else
    {
      task->taskType = SYSTEM;
      task->taskState = READY; //o escalonador, em teoria, está sempre pronto para executar
    }

    task->executionTime = systime (); //marca o início da tarefa
    task->processorTime = 0; //marca o tempo de processamento da tarefa
    task->activations = 0;
    task->stack = stack;
    return task->id;
  }
  else
  {
    perror ("Erro na criação da pilha: ");
    return -1;
  }
}; //TERMINADO


int task_switch (task_t *task)
{
  task_t *task_aux;

  task_aux = taskCurrent;
  taskCurrent = task;

  #ifdef DEBUG
  printf ("task_switch: trocando contexto %d -> %d\n", task_aux->id, task->id);
  #endif

  if (swapcontext (&(task_aux->context), &(task->context)) == -1)
  {
    fprintf (stderr, "Erro na troca de contexto!\n");
    return -1;
  }

  return 0; //task_switch "não retorna nada" em caso de sucesso na troca
}; //TERMINADO


void task_exit (int exit_code)
{
  #ifdef DEBUG
  printf ("task_exit: tarefa %d sendo encerrada\n", taskCurrent->id);
  #endif
  
  if (!userTasks) //caso não exista nenhuma tarefa de usuário
  { 
    taskDispatcher.taskState = TERMINATED; 
    task_switch (&taskMain);
  }
  else
  {
    taskCurrent->taskState = TERMINATED;
    userTasks --;
    task_switch (&taskDispatcher);
  }
}; //TERMINADO


int task_id ()
{
  return taskCurrent->id;
}; //TERMINADO


task_t *scheduler () //POLÍTICA FCFS
{
  task_t *task_priority, *task_aux;

  task_priority = queueREADY;
  task_aux = queueREADY;
  
  //procurar a tarefa com maior prioridade dinâmica entre as tarefas prontas
  do
  {
    if (task_priority->dynamic_priority > task_aux->dynamic_priority)
      task_priority = task_aux;

    task_aux = task_aux->next;
  } while (task_aux != queueREADY);

  task_aux = queueREADY;
  
  //envelhecer as demais tarefas, respeitando o limite de (-20) do UNIX, com alfa = -1
  do
  {
    if (task_aux->dynamic_priority > -20)
    {
      if (task_aux != task_priority)
        task_aux->dynamic_priority --;
    }
    
    task_aux = task_aux->next;
  } while (task_aux != queueREADY);
  

  task_priority->dynamic_priority = task_priority->static_priority;
  return (task_priority); //retorna a tarefa de prioridade a ser executada
}; //TERMINADO


void dispatcher () 
{
  while (userTasks > 0)
  {
    taskDispatcher.activations ++;
    taskNext = scheduler ();
    if (taskNext)
    {
      taskNext->quantum = 20;
      
      //CONTABILIZAÇÃO
      taskNext->activations ++;
      processorCounter = systime (); //guarda o momento atual do relógio antes de passar o controle para a tarefa
      task_switch (taskNext); // transfere o controle para a tarefa "next"
      taskNext->processorTime += (systime() - processorCounter);
      
      switch (taskNext->taskState)
      {
        
        case 0: //CASE: NEW
        {
          break;
        }

        case 1: //CASE: READY
        {
          break;
        }

        case 2: //CASE: RUNNING
        {
          break;
        }

        case 3: //CASE: SUSPENDED
        {
          break;
        }

        case 4: //CASE: TERMINATED
        {
          taskNext->executionTime = (systime() - taskNext->executionTime);
          printf("Task %d exit: execution time %d ms, cpu time %d ms, %d activations\n", taskNext->id, taskNext->executionTime, taskNext->processorTime, taskNext->activations);
          queue_remove ((queue_t **) &queueREADY, (queue_t *) taskNext);
          
          if (taskNext != &taskMain)
            free (taskNext->stack); //libera a pilha da tarefa, caso ela seja diferente da taskMain
          break;
        }
      }
    }
  }
  
  taskDispatcher.executionTime = (systime() - taskDispatcher.executionTime);
  printf("Task 1 exit: execution time %d ms, cpu time %d ms, %d activations\n", taskDispatcher.executionTime, taskDispatcher.processorTime, taskDispatcher.activations);
  task_exit (0) ; // encerra a tarefa dispatcher
}; //TERMINADO


void task_yield () //devolve o processador ao dispatcher
{
  task_switch (&taskDispatcher);
}; //TERMINADO


void task_setprio (task_t *task, int prio)
{
  if (task)
  {
    task->static_priority = prio;
    task->dynamic_priority = prio;

  }
  else
  {
    taskCurrent->static_priority = prio;
    taskCurrent->dynamic_priority = prio;
  }
}; //TERMINADO


int task_getprio (task_t *task)
{
  return (task) ? task->static_priority : taskCurrent->static_priority;
}; //TERMINADO


unsigned int systime ()
{
  return ticksCounter;
}; //TERMINADO
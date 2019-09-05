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
#include <unistd.h>
//#include <pthread.h>
#define STACKSIZE 32768 //tamanho da pilha
//#define DEBUG //descomentar quando precisar fazer depuração

//-----------------------------------------------VARIÁVEIS GLOBAIS------------------------------------------------//
task_t taskMain;                //tarefa da main (descritor da tarefa que aponta para o programa principal)
task_t *taskCurrent;            //apontador para a tarefa corrente
task_t taskDispatcher;          //o dispatcher deve ser uma tarefa!
task_t *queueREADY;             //apontador para uma lista de tarefas prontas
task_t *queueSLEEP;             //apontador para uma lista de tarefas suspensas
task_t *taskNext;               //apontador para a próxima tarefa a pegar o processador
int id = 0;                     //id da tarefa
int userTasks = 0;              //contador de tarefas de usuário (dispatcher não entra na conta)
int size_queueSLEEP = 0;        //contador do tamanho da fila de tarefas em sleep
unsigned int ticksCounter;      //contador de ticks
unsigned int processorCounter;  //contador usado para calcular o tempo de processamento
unsigned int processorCounterD; 
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
void awake_join_tasks (task_t *queueJOIN, int tamanho);
void awake_sleep_tasks (int tamanho);
//--------------------------------------------------------------------------------------------------------------//


//-------------------------------------------IMPLEMENTAÇÃO DE FUNÇÕES-------------------------------------------//
void ppos_init () //INICIALIZA AS ESTRUTURAS INTERNAS DO SISTEMA OPERACIONAL
{
  setvbuf (stdout, 0, _IONBF, 0); //COLOCAR: desativa o buffer da saída padrão (stdout), usado pela função printf
  taskMain.prev = NULL;
  taskMain.next = NULL;
  taskMain.queueJOIN = NULL;
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
  timer.it_value.tv_usec = 1000;       // primeiro disparo, em micro-segundos (1000 microssegundos = 1 milissegundos)
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

  getcontext (&(task->context));
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

    task->queueJOIN = NULL;
    task->contJOIN = 0;
    task->executionTime = systime (); //marca o início da tarefa
    task->processorTime = 0; //inicializa o tempo de processamento da tarefa
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
    taskDispatcher.exitCode = exit_code; 
    task_switch (&taskMain);
  }
  else
  {
    taskCurrent->taskState = TERMINATED;
    userTasks --;
    taskCurrent->exitCode = exit_code; 
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
    processorCounterD = systime ();
    if (size_queueSLEEP > 0) //se houver tarefas suspensas (em sleep), verificar se alguma precisa acordar neste exato momento
      awake_sleep_tasks (size_queueSLEEP);
    
    if (!queueREADY) //se NÃO houver tarefas na fila de tarefas prontas, faça:
    {
      //suspende a tarefa do dispatcher até que surja uma nova interrupção (tick a cada milissegundo)
      sleep (1);
    }
    else //se HOUVER tarefas na fila, faça:
    {
      taskDispatcher.activations ++;
      taskNext = scheduler ();
      if (taskNext)
      {
        taskNext->quantum = 20;
        
        //CONTABILIZAÇÃO
        taskNext->activations ++;
        processorCounter = systime (); //guarda o momento atual do relógio antes de passar o controle para a tarefa
        taskDispatcher.processorTime += (systime() - processorCounterD);
        task_switch (taskNext); // transfere o controle para a tarefa "next"
        taskNext->processorTime += (systime() - processorCounter);
        processorCounterD = systime ();

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
            if (taskNext->contJOIN)
              awake_join_tasks (taskNext->queueJOIN, taskNext->contJOIN);

            //CONTABILIZAÇÃO
            taskNext->executionTime = (systime() - taskNext->executionTime);
            printf ("Task %d exit: execution time %d ms, cpu time %d ms, %d activations\n", taskNext->id, taskNext->executionTime, taskNext->processorTime, taskNext->activations);
            
            queue_remove ((queue_t **) &queueREADY, (queue_t *) taskNext);
            
            if (taskNext != &taskMain)
              free (taskNext->stack); //libera a pilha da tarefa, caso ela seja diferente da taskMain
            break;
          }
        }
      }
    }
    taskDispatcher.processorTime += (systime() - processorCounterD);
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


int task_join (task_t *task)
{
  if ((task->taskState == TERMINATED) || (task == NULL))
    return -1; 

  queue_remove ((queue_t **) &queueREADY, (queue_t *) taskCurrent);
  queue_append ((queue_t **) &(task->queueJOIN), (queue_t *) taskCurrent);
  taskCurrent->taskState = SUSPENDED;
  task->contJOIN ++;
  task_yield ();

  return task->exitCode;
}; //TERMINADO


void awake_join_tasks (task_t *queueJOIN, int tamanho)
{
  task_t *auxq_join;
  task_t *auxq_remove;
  auxq_join = queueJOIN;
  
  //percorre lista de tarefas que fizeram JOIN, remove dessa lista e coloca na lista de tarefas prontas
  for (int i = 0; i < tamanho; ++i)
  {
    auxq_remove = auxq_join;
    auxq_join = auxq_join->next;
    queue_remove ((queue_t **) &queueJOIN, (queue_t *) auxq_remove);
    queue_append ((queue_t **) &queueREADY, (queue_t *) auxq_remove);
  }
}; //TERMINADO


void task_sleep (int t)
{
  queue_remove ((queue_t **) &queueREADY, (queue_t *) taskCurrent);
  queue_append ((queue_t **) &queueSLEEP, (queue_t *) taskCurrent);
  taskCurrent->awake_time = systime () + t;
  taskCurrent->taskState = SUSPENDED;
  size_queueSLEEP ++;
  
  #ifdef DEBUG
  printf ("task_sleep: tarefa %d entrando em SLEEP. Ela deve dormir por %d milissegundos\n", taskCurrent->id, taskCurrent->awake_time);
  #endif

  task_yield ();
}; //TERMINADO


void awake_sleep_tasks (int tamanho)
{
  task_t *auxq_sleep;
  task_t *auxq_remove;
  auxq_sleep = queueSLEEP;
  
  //percorre lista de tarefas em SLEEP. Se houver alguma que precisa ser acordada, será colocada na fila de tarefas prontas
  for (int i = 0; i < tamanho; ++i)
  {
    if (auxq_sleep->awake_time <= systime()) 
    {
      auxq_remove = auxq_sleep;
      auxq_sleep = auxq_sleep->next;
      queue_remove ((queue_t **) &queueSLEEP, (queue_t *) auxq_remove);
      queue_append ((queue_t **) &queueREADY, (queue_t *) auxq_remove);
      size_queueSLEEP --;
    }
    else
    {
      auxq_sleep = auxq_sleep->next;
    }
  }
}; //TERMINADO


//---------------------MANIPULAÇÃO DE SEMÁFOROS---------------------//
int sem_create (semaphore_t *s, int value)
{
  if (!s)
    return -1;
  s->sem_queue = NULL;
  s->counter = value;
  return 0;
}; //TERMINADO


int sem_down (semaphore_t *s)
{
  if (!s)
    return -1;

  s->counter --;
  if (s->counter < 0)
  {
    taskCurrent->taskState = SUSPENDED;
    queue_remove ((queue_t **) &queueREADY, (queue_t *) taskCurrent);
    queue_append ((queue_t **) &(s->sem_queue), (queue_t *) taskCurrent);
    s->size ++;
    task_yield ();
  }
  return 0;
}; //TERMINADO


int sem_up (semaphore_t *s)
{
  task_t *first;
  
  if (!s)
    return -1;
  
  s->counter ++;
  if (s->counter <= 0)
  {
    first = s->sem_queue;
    queue_remove ((queue_t **) &(s->sem_queue), (queue_t *) first);
    queue_append ((queue_t **) &queueREADY, (queue_t *) first);
    s->size --;
  }
  return 0;
}; //TERMINADO


int sem_destroy (semaphore_t *s)
{
  if (!s)
    return -1;

  task_t *auxq_s;
  task_t *auxq_remove;
  auxq_s = s->sem_queue;

  //percorre lista de tarefas da fila do semáforo e acorda todas elas
  for (int i = 0; i < (s->size); ++i)
  {
    auxq_remove = auxq_s;
    auxq_s = auxq_s->next;
    queue_remove ((queue_t **) &(s->sem_queue), (queue_t *) auxq_remove);
    queue_append ((queue_t **) &queueREADY, (queue_t *) auxq_remove);
  }
  if (!s->sem_queue)
    return 0;
  return -1;
}; //TERMINADO
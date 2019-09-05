#include "ppos.h"
#include <stdio.h>
#include <stdlib.h>
#define STACKSIZE 32768 //tamanho da pilha
//#define DEBUG //descomentar quando precisar fazer depuração



//----------------------------------------------VARIÁVEIS GLOBAIS----------------------------------------------//
task_t taskMain;        //tarefa da main (descritor da tarefa que aponta para o programa principal)
task_t *taskCurrent;    //apontador para a tarefa corrente
task_t taskDispatcher;  //o dispatcher deve ser uma tarefa!
task_t *queueREADY;     //apontador para uma lista da tarefa de prontas
task_t *taskNext;       //apontador para a próxima tarefa a pegar o processador
int id = 0;             //id da tarefa
int userTasks = 0;      //contador de tarefas de usuário (dispatcher não entra na conta)
//--------------------------------------------------------------------------------------------------------------//



//---------------------------------------------DECLARAÇÃO DE FUNÇÕES---------------------------------------------//
void dispatcher_body ();
//--------------------------------------------------------------------------------------------------------------//


//--------------------------------------------IMPLEMENTAÇÃO DE FUNÇÕES--------------------------------------------//

//P2: GESTÃO DE TAREFAS
void ppos_init () //por enquanto, conterá apenas algumas inicializações de variáveis
{
  setvbuf (stdout, 0, _IONBF, 0); //COLOCAR: desativa o buffer da saída padrão (stdout), usado pela função printf
  taskMain.prev = NULL;
  taskMain.next = NULL;
  taskMain.id = 0; //por default, a taskMain recebe zero!

  
  task_create (&taskDispatcher, (void*)(dispatcher_body), NULL); //cria a tarefa do dispatcher
  taskCurrent = &taskMain;
}; //TERMINADO


int task_create (task_t *task, void(*start_routine)(void *), void *arg)
{
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
    printf("task_create: criou tarefa %d\n", task->id);
    #endif

    if (task != &taskDispatcher)
    {
      task->static_priority = 0;
      task->dynamic_priority = 0;
      queue_append ((queue_t **) &queueREADY, (queue_t *) task);
      userTasks ++;
    }
    
    task->stack = stack;
    return task->id;
  }
  else
  {
    perror("Erro na criação da pilha: ");
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

  if (swapcontext(&(task_aux->context), &(task->context)) == -1)
  {
    fprintf (stderr, "Erro na troca de contexto!\n");
    return -1;
  }

  return 0; //task_switch "não retorna nada" em caso de sucesso na troca
}; //TERMINADO


void task_exit (int exit_code)
{
  #ifdef DEBUG
  printf("task_exit: tarefa %d sendo encerrada\n", taskCurrent->id);
  #endif
  
  if (userTasks == 0) //caso não exista nenhuma tarefa de usuário
  { 
    free (taskDispatcher.stack); //libera a pilha do Dispatcher
    task_switch (&taskMain);
  }
  else
  {
    queue_remove ((queue_t **) &queueREADY, (queue_t *) taskCurrent);
    if (queueREADY != NULL)
      free (taskCurrent->stack); //libera a pilha da tarefa
    userTasks --;
    task_switch (&taskDispatcher);
  }
}; //TERMINADO


int task_id ()
{
  return taskCurrent->id;
}; //TERMINADO


//P3: DISPATCHER
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
    if (task_aux->dynamic_priority > -20){
      if (task_aux != task_priority){
        task_aux->dynamic_priority --;
      }
    }
    task_aux = task_aux->next;
  } while (task_aux != queueREADY);
  

  task_priority->dynamic_priority = task_priority->static_priority;
  return (task_priority); //retorna a tarefa de prioridade a ser executada
}; //TERMINADO


void dispatcher_body () 
{
  while (userTasks > 0)
  {
    taskNext = scheduler();
    if (taskNext)
    {
      task_switch (taskNext); // transfere controle para a tarefa "next"
    }
  }
  task_exit(0) ; // encerra a tarefa dispatcher
}; //TERMINADO


void task_yield () //devolve o processador ao dispatcher
{
  task_switch (&taskDispatcher);
}; //TERMINADO


//P4: ESCALONAMENTO POR PRIORIDADES
void task_setprio (task_t *task, int prio)
{
  if (task != NULL)
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
  return (task != NULL) ? task->static_priority : taskCurrent->static_priority;
}; //TERMINADO



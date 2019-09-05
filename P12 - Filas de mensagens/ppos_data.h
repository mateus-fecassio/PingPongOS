/*--------------------------------------------------------------------------------------------//
PROJETO: PingPongOS - PingPong Operating System
PROPOSTO POR: Prof. Carlos A. Maziero, DINF UFPR
AUTOR DO ARQUIVO: Mateus Felipe de Cássio Ferreira (GRR20176123)

ASSUNTO: Preempção por tempo e Contabilização
//--------------------------------------------------------------------------------------------*/

#ifndef __PPOS_DATA__
#define __PPOS_DATA__
#include <ucontext.h>				// biblioteca POSIX de trocas de contexto
#include "queue.h"					// biblioteca de filas genéricas

//-------------------------------------------ESTRUTURAS / TIPOS-------------------------------------------//

typedef enum 
{
  SYSTEM,
  USER
}task_type;

typedef enum 
{
  NEW,
  READY,
  RUNNING,
  SUSPENDED,
  TERMINATED
}task_state;

typedef enum 
{
  UNAVAILABLE,
  AVAILABLE
}message_state;


// Estruturas de dados internas do sistema operacional

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
   struct task_t *prev, *next;   //ponteiros para usar em filas
   int id;                       //identificador da tarefa
   ucontext_t context;           //contexto armazenado da tarefa (VER contexts.c)
   void *stack;                  //aponta para a pilha da tarefa
   task_type taskType;           //define o tipo de tarefa criada
   task_state taskState;         //define o estado da tarefa
   int static_priority;          //guarda o valor da prioridade estática da tarefa
   int dynamic_priority;         //guarda o valor da prioridade dinâmica da tarefa
   int quantum;                  //define o valor de quantum que cada tarefa no sistema receberá
   unsigned int executionTime;   //guarda o tempo de execução da tarefa
   unsigned int processorTime;   //guarda o tempo de processamento da tarefa 
   int activations;              //guarda o número de ativações realizadas pela tarefa
   int exitCode;                 //guarda o exit code de uma tarefa
   struct task_t *queueJOIN;     //ponteiro para uma fila de tarefas que estão suspensas pelo JOIN
   int contJOIN;                 //contador de tarefas que estão suspensas pelo JOIN
   int awake_time;               //marca o tempo, em milissegundos, que uma tarefa suspensa deve voltar à fila de tarefas prontas

} task_t ;


// estrutura que define um semáforo
typedef struct
{
  int counter;                   //contador interno do semáforo   
  int size;                      //tamanho da fila do semáforo
  task_t *sem_queue;             //fila do semáforo
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;
// estrutura que define uma fila de mensagens
typedef struct
{
  int message_bsize;             //tamanho em BYTES de cada mensagem
  int queue_size;                //tamanho da fila de mensagens
  int message_counter;           //número de mensagens presentes na fila
  int start;                     //marcador do início do buffer circular
  int end;                       //marcador do final do buffer circular
  void *buffer;                  //buffer de mensagens
  message_state msgState;        //estado da fila de mensagens
  semaphore_t s_vaga;
  semaphore_t s_buffer;
  semaphore_t s_item;

} mqueue_t ;

#endif

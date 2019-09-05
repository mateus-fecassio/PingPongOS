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
} task_t ;


// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
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
  // preencher quando necessário
} mqueue_t ;

#endif

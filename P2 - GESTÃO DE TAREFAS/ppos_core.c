#include "ppos.h"
#include <stdio.h>
#include <stdlib.h>
//#include <ucontext.h> //para poder utilizar as funções de getcontext, makecontext, swapcontext e setcontext (caso necessário)
//#define DEBUG //descomentar quando precisar fazer depuração
#define STACKSIZE 32768 //tamanho da pilha 



//----------------------------------------------VARIÁVEIS GLOBAIS----------------------------------------------//
task_t taskMain; 		  //tarefa da main (descritor da tarefa que aponta para o programa principal)
task_t *taskCurrent; 	//apontador para a tarefa corrente
int id = 0; 			    //id da tarefa
//--------------------------------------------------------------------------------------------------------------//


//----------------------------------------------IMPLEMENTAÇÃO DE FUNÇÕES----------------------------------------------//
void ppos_init () //por enquanto, conterá apenas algumas inicializações de variáveis
{
	setvbuf (stdout, 0, _IONBF, 0); //COLOCAR: desativa o buffer da saída padrão (stdout), usado pela função printf
	taskMain.prev = NULL;
	taskMain.next = NULL;
	taskMain.id = 0; //por default, a taskMain recebe zero!

	taskCurrent = &taskMain; //necessário pois, depois de inicializado, a tarefa corrente tem que apontar para a taskMain
}; //TERMINADO


int task_create (task_t *task, void (*start_routine)(void *),  void *arg)
{
  char *stack;
	
	getcontext (&(task->context)); //equivalente ao (&ContextPing), que nesse caso já passava o contexto direto
  stack = malloc (STACKSIZE);

   	if (stack)
   	{  
    	task->context.uc_stack.ss_sp = stack ;
    	task->context.uc_stack.ss_size = STACKSIZE ;
    	task->context.uc_stack.ss_flags = 0 ;
    	task->context.uc_link = 0 ;
    	makecontext (&(task->context), (void*)(*start_routine), 1, arg);
    	task->id = ++id;

    	#ifdef DEBUG
        printf ("task_create: criou tarefa %d\n", task->id) ;
        #endif

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

    if (swapcontext(&(task_aux->context), &(task->context)) == -1)
    {
    	fprintf(stderr, "Erro na troca de contexto!\n");
    	return -1;
    }

    return 0; //task_switch "não retorna nada" em caso de sucesso na troca
}; //TERMINADO


void task_exit (int exit_code)
{
	#ifdef DEBUG
    printf ("task_exit: tarefa %d sendo encerrada\n", taskCurrent->id) ;
    #endif
    //como o parâmetro exit_code ainda não será usado, quando uma tarefa se encerra, passa-se o controle para a tarefa main
	task_switch (&taskMain);
}; //TERMINADO


int task_id ()
{
	return taskCurrent->id;
}; //TERMINADO
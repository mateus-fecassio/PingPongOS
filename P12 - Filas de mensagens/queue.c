/*--------------------------------------------------------------------------------------------//
PROJETO: PingPongOS - PingPong Operating System
PROPOSTO POR: Prof. Carlos A. Maziero, DINF UFPR
AUTOR DO ARQUIVO: Mateus Felipe de Cássio Ferreira (GRR20176123)

ASSUNTO: Preempção por tempo e Contabilização
//--------------------------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include "queue.h"

//--------------------------------------------IMPLEMENTAÇÃO DE FUNÇÕES--------------------------------------------//
void queue_append (queue_t **queue, queue_t *elem)
{
	queue_t *auxiliar = NULL;
	queue_t *aux_cabeca = NULL;
	queue_t *aux = NULL;

	if (queue == NULL){ //CASO A FILA NÃO EXISTA
		fprintf (stderr, "Erro! Essa fila não existe.\n");
		return;
	}

	if (elem == NULL){ //CASO O ELEMENTO A SER INSERIDO NÃO EXISTA
		fprintf (stderr, "Erro! Esse elemento não existe.\n");
		return;
	} else if (elem->prev != NULL || elem->next != NULL){ //CASO O ELEMENTO A SER INSERIDO JÁ ESTÁ CONTIDO EM UMA OUTRA LISTA
		fprintf (stderr, "Erro! Esse elemento já existe em uma fila.\n");
		return;
	}

	
	if (*queue == NULL){ //PRIMEIRA INSERÇÃO NA LISTA (VAZIA)
		auxiliar = elem;
		auxiliar->prev = elem;
		auxiliar->next = elem;
		*queue = auxiliar; //a cabeça da lista aponta para o primeiro elemento inserido
		return;
	} else {
		auxiliar = elem;
		aux_cabeca = *queue;
		aux = *queue;

		while (aux->next != aux_cabeca){ //procura até achar a posição certa para inserir (no final da fila)
			aux = aux->next;
		}
		aux->next = auxiliar;
		auxiliar->prev = aux;
		auxiliar->next = aux_cabeca;
		aux_cabeca->prev = auxiliar;
	}
	//printf("queue está em %p (endereço) e guarda %p (endereço). *queue vale: %p e **queue vale: %d\n", &queue, queue, *queue, **queue);
	//printf("elem está em %p (endereço) e guarda %p. *elem vale: %d \n\n\n", &elem, elem, *elem);
}; //TERMINADO


queue_t *queue_remove (queue_t **queue, queue_t *elem)
{
	queue_t *auxiliar = NULL;
	queue_t *aux_cabeca = NULL;

	if (queue == NULL){ //CASO A FILA NÃO EXISTA
		fprintf (stderr, "Erro! Essa fila não existe.\n");
		return NULL;
	}
	if (*queue == NULL){ //CASO A FILA ESTEJA VAZIA
		fprintf (stderr, "Erro! Essa fila se encontra vazia.\n");
		return NULL;
	}
	if (elem == NULL){ //CASO O ELEMENTO A SER INSERIDO NÃO EXISTA
		fprintf (stderr, "Erro! Esse elemento não existe.\n");
		return NULL;
	}
	if (elem->prev == NULL && elem->next == NULL){ //CASO O ELEMENTO A SER INSERIDO NÃO PERTENÇA A NENHUMA LISTA
		fprintf (stderr, "Erro! Esse elemento não pertence a nenhuma fila.\n");
		return NULL;
	}
	
	aux_cabeca = *queue;
	auxiliar = *queue;
	//CASO 1: REMOÇÃO DE UM ELEMENTO DA PRIMEIRA POSIÇÃO
	if (elem == aux_cabeca){
		if (queue_size(*queue) == 1){ //caso o elemento a ser removido seja o primeiro e único da fila
			*queue = NULL;
		} else { //caso o elemento a ser removido seja o primeiro mas não o único da fila
			queue_t *ultimo = aux_cabeca->prev;
			queue_t *primeiro = aux_cabeca->next;

			primeiro->prev = ultimo;
			ultimo->next = primeiro;
			*queue = primeiro;

		}
		elem->prev = NULL;
		elem->next = NULL;
		return elem;
	}
	//CASO 2 e 3: REMOÇÃO DE UM ELEMENTO NO MEIO DA FILA OU QUE ESTEJA NO FINAL DELA
	auxiliar = auxiliar->next;
	while (auxiliar != aux_cabeca){ //procura até achar a posição certa para remover (ou não)
		if (elem == auxiliar){ 
			queue_t *anterior = auxiliar->prev;
			queue_t *posterior = auxiliar->next;

			anterior->next = posterior;
			posterior->prev = anterior;

			elem->prev = NULL;
			elem->next = NULL;
			return elem;
		}
		auxiliar = auxiliar->next;
	}
	
	fprintf (stderr, "Erro! Esse elemento não pertence a essa fila.\n");
	return NULL; //CASO O ELEMENTO A SER REMOVIDO NÃO PERTENÇA A ESSA FILA.
}; //TERMINADO


int queue_size (queue_t *queue)
{
	if (queue == NULL) //se a fila está vazia
		return 0;

	queue_t *aux = queue;
	queue_t *aux_cabeca = queue;
	int tamanho = 1;
	
	while (aux->next != aux_cabeca){
		tamanho ++;
		aux = aux->next;
	}
	return tamanho;
}; //TERMINADO


void queue_print (char *name, queue_t *queue, void print_elem (void*))
{
	if (queue == NULL){ //caso a fila se encontre vazia
		printf("%s []\n", name);
		return;
	}

	queue_t *auxiliar = queue;
	queue_t *aux_cabeca = queue;
	
	printf("%s [", name);
	while (auxiliar->next != aux_cabeca){
		print_elem ((void *) auxiliar);
		auxiliar = auxiliar->next;
	}
	print_elem ((void *) auxiliar);
	printf("]\n");
}; //TERMINADO

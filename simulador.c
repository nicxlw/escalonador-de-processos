#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>


// Definições e constantes
#define MAX_PROCESSOS 5
#define FATIA_TEMPO 2
#define TEMPO_IO_DISCO 2
#define TEMPO_IO_FITA 3
#define TEMPO_IO_IMPRESSORA 4

// Estados dos processos
#define PRONTO 0
#define EXECUTANDO 1
#define BLOQUEADO 2
#define CONCLUIDO 3

int i, soma_tempo_espera = 0, soma_turnaround = 0;

// Estrutura do processo
typedef struct Processo {
    int pid;
    int tempo_chegada;
    int burst_time;
    int tempo_restante;
    int estado;
    int proximo_io;
    int tipo_io;
    int tempo_io_restante;
    int prioridade;
    int tempo_espera;
    int turnaround;
    struct Processo* prox;
} Processo;

// Estrutura da fila
typedef struct Fila {
    Processo* inicio;
    Processo* fim;
} Fila;

// Prototipos das funções
void inicializar_fila(Fila* fila);
void adicionar_na_fila(Fila* fila, Processo* processo);
Processo* remover_da_fila(Fila* fila);
bool fila_vazia(Fila* fila);
void gerar_processos_aleatorios(Fila* fila_alta, int* total_processos);
void executar_escalonador(Fila* alta, Fila* baixa, Fila* io, int* tempo_global, int* processos_concluidos);
void processar_io(Fila* io, Fila* alta, Fila* baixa, int tempo_global);
void exibir_estado(Fila* alta, Fila* baixa, Fila* io);

// Inicializa uma fila
void inicializar_fila(Fila* fila) {
    fila->inicio = NULL;
    fila->fim = NULL;
}

// Adiciona um processo na fila
void adicionar_na_fila(Fila* fila, Processo* processo) {
    processo->prox = NULL;
    if (fila->fim == NULL) {
        fila->inicio = fila->fim = processo;
    } else {
        fila->fim->prox = processo;
        fila->fim = processo;
    }
}

// Remove um processo da fila
Processo* remover_da_fila(Fila* fila) {
    if (fila_vazia(fila)) return NULL;
    Processo* removido = fila->inicio;
    fila->inicio = fila->inicio->prox;
    if (fila->inicio == NULL) fila->fim = NULL;
    return removido;
}

// Verifica se a fila está vazia
bool fila_vazia(Fila* fila) {
    return fila->inicio == NULL;
}

// Gera processos aleatórios
void gerar_processos_aleatorios(Fila* fila_alta, int* total_processos) {
	*total_processos = (rand() % (MAX_PROCESSOS-1)) + 2;
    for (i = 0; i < *total_processos; i++) {
        Processo* novo = (Processo*)malloc(sizeof(Processo));
        novo->pid = i + 1;
        novo->tempo_chegada = 0;
        novo->burst_time = (rand() % 10) + 5;
        novo->tempo_restante = novo->burst_time;
        novo->estado = PRONTO;
        novo->proximo_io = (rand() % (novo->burst_time / 2)) + 1;
        novo->tipo_io = (rand() % 3) + 2; // 2: Disco, 3: Fita, 4: Impressora
        novo->tempo_io_restante = (novo->tipo_io == 2) ? TEMPO_IO_DISCO : 
                                   (novo->tipo_io == 3) ? TEMPO_IO_FITA : TEMPO_IO_IMPRESSORA;
        novo->prioridade = 0; // Alta prioridade
        novo->tempo_espera = 0;
        novo->turnaround = 0;
        adicionar_na_fila(fila_alta, novo);
    }
}

void executar_escalonador(Fila* alta, Fila* baixa, Fila* io, int* tempo_global, int* processos_concluidos) {

	Processo* atual = NULL;
	
	while(alta->inicio == NULL && baixa->inicio == NULL){ //se todos os processos estiverem em io
		(*tempo_global)++;
	    printf("\nTempo global: %d", *tempo_global);
    	printf("\nTodos os processos estao em io\n\n");
		if(io->inicio != NULL)processar_io(io, alta, baixa, *tempo_global);
		exibir_estado(alta, baixa, io);
	}
		
	if(alta->inicio != NULL)
		atual = alta->inicio;
	else if (baixa->inicio != NULL) //se a fila alta estiver vazia
		atual = baixa->inicio;
	
    if(atual == NULL)
		return; //se a fila baixa tambem estiver vazia

    if(atual->estado == PRONTO)
		atual->estado = EXECUTANDO;

    for (i = 0; i < FATIA_TEMPO && atual->tempo_restante > 0; i++) {
    	(*tempo_global)++;
	    printf("\nTempo global: %d", *tempo_global);
    	printf("\nProcesso: %d   Uso do quantum: %d/%d\n\n", atual->pid, i+1, FATIA_TEMPO);
		if(!fila_vazia(io)){
        	processar_io(io, alta, baixa, *tempo_global);
        }
        if(atual->estado == BLOQUEADO){
			exibir_estado(alta, baixa, io);
		}

    	if(atual->estado == EXECUTANDO){
			atual->tempo_restante--;
			exibir_estado(alta, baixa, io);
			if (atual->tempo_restante > 0 && atual->proximo_io == (atual->burst_time - atual->tempo_restante)) {
	        	atual->prioridade == 0? remover_da_fila(alta): remover_da_fila(baixa);
	            atual->estado = BLOQUEADO;
	            adicionar_na_fila(io, atual);
	            printf("\n[P%d] foi para a fila de IO\n", atual->pid);
	    	}
	        
	    	if (atual->tempo_restante == 0) {
		        atual->estado = CONCLUIDO;
		        atual->prioridade == 0? remover_da_fila(alta): remover_da_fila(baixa);
		        atual->turnaround = *tempo_global - atual->tempo_chegada;
		        atual->tempo_espera = atual->turnaround - atual->burst_time;
		        soma_tempo_espera += atual->tempo_espera; //para estatisticas
				soma_turnaround += atual->turnaround; //para estatisticas
		        printf("\nProcesso %d concluido. Turnaround: %d\n", atual->pid, atual->turnaround);
		        free(atual);
		        (*processos_concluidos)++;
		    }			 
		}
    }
    if (atual->estado == EXECUTANDO) {   // Preempsao
	    atual->estado = PRONTO;	  
	    if (atual->prioridade == 0) { // Estava na fila alta
	        remover_da_fila(alta);
	        atual->prioridade = 1; // Vai para baixa prioridade
	        adicionar_na_fila(baixa, atual);
	    } else { // Estava na fila baixa
	        remover_da_fila(baixa);
	        adicionar_na_fila(baixa, atual); // Reinsere na fila de baixa prioridade
	    }
	}


}

// Processa a fila de I/O
void processar_io(Fila* io, Fila* alta, Fila* baixa, int tempo_global) {

        Processo* atual = io->inicio;
        atual->tempo_io_restante--;
		
        if (atual->tempo_io_restante == 0) {
        	remover_da_fila(io);

            atual->estado = PRONTO;
            if (atual->tipo_io == TEMPO_IO_DISCO){
            	atual->prioridade = 1;
            	adicionar_na_fila(baixa, atual); // Disco
            	printf("\n[P%d] voltou para a fila baixa\n", atual->pid);
			}
            else{
            	atual->prioridade = 0;
			    adicionar_na_fila(alta, atual); // Fita ou Impressora
			    printf("\n[P%d] voltou para a fila alta\n", atual->pid);
        	}
		}
}

// Exibe o estado das filas
void exibir_estado(Fila* alta, Fila* baixa, Fila* io) {
	Processo* p;
    printf("Fila Alta Prioridade: ");
    for (p = alta->inicio; p != NULL; p = p->prox) printf("[P%d](%d/%d) ", p->pid, p->burst_time - p->tempo_restante, p->burst_time);
    printf("\nFila Baixa Prioridade: ");
    for (p = baixa->inicio; p != NULL; p = p->prox) printf("[P%d](%d/%d) ", p->pid, p->burst_time - p->tempo_restante, p->burst_time);
    printf("\nFila I/O: ");
    for (p = io->inicio; p != NULL; p = p->prox) printf("[P%d](%d/%d) ", p->pid, p->tipo_io - p->tempo_io_restante, p->tipo_io);
    printf("\n---------------------\n");
}

// Programa principal
int main() {
	int total_processos;
	int cont = 1;
    srand(time(NULL));

    Fila fila_alta, fila_baixa, fila_io;
    inicializar_fila(&fila_alta);
    inicializar_fila(&fila_baixa);
    inicializar_fila(&fila_io);

    gerar_processos_aleatorios(&fila_alta, &total_processos);
    
	// Exibir informações dos processos criados
	printf("Processos Criados:\nPID\tChegada\tServico\tProx. IO\tTipo IO\n");
	Processo* atual = fila_alta.inicio;
	while (atual != NULL) {
	    char* tipo_io_str;
	    switch (atual->tipo_io) {
	        case TEMPO_IO_DISCO: tipo_io_str = "Disco"; break;
	        case TEMPO_IO_FITA: tipo_io_str = "Fita"; break;
	        case TEMPO_IO_IMPRESSORA: tipo_io_str = "Impressora"; break;
	        default: tipo_io_str = "Desconhecido"; break;
	    }
	    printf("%d\t%d\t%d\t%d\t\t%s\n", atual->pid, atual->tempo_chegada, atual->burst_time, atual->proximo_io, tipo_io_str);
	    atual = atual->prox;
	}
	printf("\n");
	printf("Caracteristica temporal de cada operacao de E/S: Disco leva 2 u.t., Fita leva 3 u.t. e Impressora leva 4 u.t.\n");
	printf("Fatia de tempo: 2 u.t.\n");
    int tempo_global = 0, processos_concluidos = 0;
    
    //Estado inicial das filas
	printf("\nINICIO\n");
    exibir_estado(&fila_alta, &fila_baixa, &fila_io);
    
    //loop principal
    while (processos_concluidos < total_processos) {
        executar_escalonador(&fila_alta, &fila_baixa, &fila_io, &tempo_global, &processos_concluidos);
    }
	
    printf("\nTodos os processos foram concluidos.\n");
    
    // Estatisticas
	printf("\nEstatisticas:");
    printf("\nTempo medio de espera: %.2f u.t.\n", (float)soma_tempo_espera / total_processos);
	printf("Tempo medio de turnaround: %.2f u.t.\n", (float)soma_turnaround / total_processos);

    return 0;
}

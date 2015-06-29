/***********************************************************************************************
**--------------------------------------------------------------------------------------------**
**||						SSC0641 - Redes de Computadores									||**
**||									Projeto												||**
**||			Alunos: Cassiano Zaghi de Oliveira		: 7987400							||**
**||					João Victor Almeida de Aguiar	: 8503986							||**
**||					Lucas Tomazela 					: 8124271							||**
**--------------------------------------------------------------------------------------------**
************************************************************************************************/

#ifndef MESSENGER_HH
#define MESSENGER_HH

#include <pthread.h>
#include <vector>
#include <map>

#include "contact.hh"
#include "group.hh"

using namespace std;

#define LISTEN_PORT 9666

#define MAX_MSG_SIZE 2048

#define MAX_USERNAME_SIZE 512

#define MAX_CONNECTIONS 50




/************************** FUNÇÕES AUXILIARES **************************/
// Retrona se o contato está no map de contatos
bool exist_contact (string name, map<string, contact*> *contacts, pthread_mutex_t *contacts_mutex);

bool exist_group(string gname, map<string, group*> groups);

// Cria um contato, inicializa seus campos, e retorna-o
contact* create_contact (string name, map<string, contact*> *contacts);

// Cria um sockaddr_in através do seu IP
struct sockaddr_in* create_addr_by_ip(string peer_ip);

// Insere o contato no vetor de contatos, tratanto a região crítica
void insert_contact (string name, contact* cntct, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex);

// Insere uma mensagem no histórico de mensagens de um contato, tratando a região crítica
void insert_contact_hystory (contact* cntct, string msg, map<string, contact*> *contacts, pthread_mutex_t *contacts_mutex);

void start_messenger (map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex, pthread_t *listen_server_thread);
void finish_messenger (map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex, pthread_t *listen_server_thread);



/* Eu */
void add_contact (string name, string peer_ip, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex, string my_name);


struct receiver_thread_args
{
	contact* my_contact;
	map<string, contact*> *contacts;
	map<string, group*> *groups;
	pthread_mutex_t *contacts_mutex;
};
//args = receiver_thread_args
void* receiver_thread (void* args);



/* Tomazellis */
void open_conversation(string name, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex);
void list_contacts (map<string, contact*> *contacts);
void del_contact(string name, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex);
void close_conversation(string name, map<string, contact*> *contacts);


int set_socket_port (int socket, int port);

/* Cassiano */
void send_msg (string name, string msg, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex);
void send_msg_contact (contact* cntct, string msg, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex);
void send_group_msg (string gname, string msg, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex);


struct listen_server_args
{
	map<string, contact*> *contacts;
	map<string, group*> *groups;
	pthread_mutex_t *contacts_mutex;
};
//args = listen_server_args
void* listen_server (void *);

void main_menu(map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex, string my_name);


#endif
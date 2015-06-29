/***********************************************************************************************
**--------------------------------------------------------------------------------------------**
**||                             SSC0641-Redes de Computadores                              ||**
**||                                       Projeto                                          ||**
**||            Alunos: Cassiano Zaghi de Oliveira      : 7987400                           ||**
**||                    João Victor Almeida de Aguiar   : 8503986                           ||**
**||                    Lucas Tomazela                  : 8124271                           ||**
**--------------------------------------------------------------------------------------------**
************************************************************************************************/

#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <pthread.h>
#include <map>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <sstream>
#include <cstdio>

#include "contact.hh"
#include "group.hh"
#include "messenger.hh"

using namespace std;


int main(int argc, char const *argv[])
{
	map<string, contact*> contacts;
	map<string, group*> groups;

	pthread_mutex_t contacts_mutex;

	pthread_t listen_server_thread;

	string my_name;


	cout << "Qual o seu username ?? ";
	getline(cin, my_name);

	start_messenger(&contacts, &groups, &contacts_mutex, &listen_server_thread);

	
	main_menu(&contacts, &groups, &contacts_mutex, my_name);


	finish_messenger(&contacts, &groups, &contacts_mutex, &listen_server_thread);

	return 0;
}
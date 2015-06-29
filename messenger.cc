/***********************************************************************************************
**--------------------------------------------------------------------------------------------**
**||						SSC0641 - Redes de Computadores									||**
**||									Projeto												||**
**||			Alunos: Cassiano Zaghi de Oliveira		: 7987400							||**
**||					João Victor Almeida de Aguiar	: 8503986							||**
**||					Lucas Tomazela 					: 8124271							||**
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

#include "messenger.hh"

using namespace std;




bool exist_contact (string name, map<string, contact*> *contacts, pthread_mutex_t* contacts_mutex)
{
	pthread_mutex_lock(contacts_mutex);
	bool result = (*contacts).find(name) != (*contacts).end();
	pthread_mutex_unlock(contacts_mutex);

	return result;
}

bool exist_group(string gname, map<string, group*> *groups)
{
	bool result = groups->find(gname) != groups->end();

	return result;
}


contact* create_contact (string name, map<string, contact*> *contacts)
{
	contact *new_contact = (contact *) malloc (sizeof (contact));

	new_contact->name = new string(name);
	new_contact->skt_my = socket(AF_INET, SOCK_STREAM, 0);
	new_contact->n_new_msgs = 0;
	new_contact->history = new vector<string*>();

	return new_contact;
}


struct sockaddr_in* create_addr_by_ip(string peer_ip)
{
	struct hostent peer_addr_ip = *gethostbyname(peer_ip.c_str());

	struct sockaddr_in* peer_addr = (struct sockaddr_in*) malloc (sizeof(sockaddr_in));
	peer_addr->sin_family = AF_INET;
	peer_addr->sin_port   = htons(LISTEN_PORT);
	peer_addr->sin_addr   = *((struct in_addr*) peer_addr_ip.h_addr);

	return peer_addr;
}


void insert_contact (string name, contact* cntct, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t* contacts_mutex)
{
	// O MAP DE CONTATOS É UMA REGIÃO CRÍTICA!!!!
	pthread_mutex_lock(contacts_mutex);

	contacts->insert(make_pair(name, cntct));

	// CRIA A THREAD QUE RECEBE AS MENSAGENS

	struct receiver_thread_args *args = (struct receiver_thread_args *) malloc (sizeof(struct receiver_thread_args));

	args->my_contact = cntct;
	args->contacts = contacts;
	args->groups = groups;
	args->contacts_mutex = contacts_mutex;


	pthread_create(&cntct->receiver, NULL,
    				receiver_thread, (void*) args);

	pthread_mutex_unlock(contacts_mutex);
}


void insert_contact_hystory (contact* cntct, string msg, map<string, contact*> *contacts, pthread_mutex_t* contacts_mutex)
{
	// OS DADOS DO CONTATO SÃO UMA REGIÃO CRÍTICA!
	pthread_mutex_lock(contacts_mutex);

	if (((*contacts).find(*(cntct->name)) != (*contacts).end()) && (!msg.empty()))
	{
		std::stringstream ss;
		ss << *(cntct->name) << " : " << msg;

		cntct->history->push_back(new string(ss.str()));
		cntct->n_new_msgs ++;
	}

	pthread_mutex_unlock(contacts_mutex);
}


void add_contact (string name, string peer_ip, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex, string my_name)
{
	if (!exist_contact(name, contacts, contacts_mutex))
	{
		// CRIA UM NOVO CONTATO
		contact *new_contact = create_contact(name, contacts);

		// CRIA O ENDEREÇO DO SERVIDOR DO PEER
		struct sockaddr_in* peer_addr = create_addr_by_ip(peer_ip);

		cout << "Tentando conectar com " << name << " ..." << endl;
		
		// A função connect possui um timeout, logo, se não for possível conectar, esta retorna -1
		int connect_value = connect(new_contact->skt_my, (struct sockaddr*) peer_addr, sizeof(*peer_addr));
		
		if (connect_value == -1)
		{
			cout << "Nao foi possivel estabelecer conexao com " << name << endl;
			return;
		}

		cout << "Conexao com " << name << " iniciada com sucesso!" << endl;

		// Quando estabelida uma conexão, envia o seu nome de contato para quem pediu a conexão
		send(new_contact->skt_my, my_name.c_str(), my_name.length()*sizeof(char), 0);


		free(peer_addr);


		// INSERE O CONTATO NA LISTA DE CONTATOS E INICIA SUA THREAD
		insert_contact(name, new_contact, contacts, groups, contacts_mutex);
	}
	else
	{
		cout << "Contato já existe!\n";
	}
}

//args[0] = contact!!!
void* receiver_thread (void* args)
{
	struct receiver_thread_args func_args = *((struct receiver_thread_args *) args);

	contact* my_contact = func_args.my_contact;
	map<string, contact*> *contacts = func_args.contacts;
	map<string, group*> *groups = func_args.groups;
	pthread_mutex_t *contacts_mutex = func_args.contacts_mutex;

	while (my_contact != NULL)
	{
		char *msg = (char *) malloc (MAX_MSG_SIZE*sizeof(char));

		// como o socket nunca muda, este não é uma região crítica
		// pois todos apenas lêem dele, e nunca o alteram

		pthread_mutex_lock(contacts_mutex);
		int skt = my_contact->skt_my;
		pthread_mutex_unlock(contacts_mutex);

		int msgsize = recv(skt, msg, MAX_MSG_SIZE, 0);


		// Se o contato tiver sido apagado durante o recv, a thread já vai ter sido fechada, logo
		// não é necessário verificar a região crítica após o recv!


		if (msgsize == 0)
		{
			cout << "Contato " << *(my_contact->name) << " desconectado" << endl;
			del_contact(*(my_contact->name), contacts, groups, contacts_mutex);
			pthread_exit(NULL);
		}
		else if (msgsize == -1)
		{
			cout << "-> ERROR ON RECV MSG!!" << endl;
		}
		else
		{
			// ENVIAR AS NOTIFICAÇÕES AQ!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

			// INSERE UMA MENSAGEM NO HISTÓRICO DO CONTATO
			insert_contact_hystory(my_contact, msg, contacts, contacts_mutex);
		}
	}
}




void start_messenger (map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex, pthread_t *listen_server_thread)
{
	pthread_mutex_init (contacts_mutex, NULL);

	struct listen_server_args *args = (struct listen_server_args *) malloc (sizeof(struct listen_server_args));
	args->contacts = contacts;
	args->groups = groups;
	args->contacts_mutex = contacts_mutex;


	pthread_create(listen_server_thread, NULL, listen_server, (void*) args);

}	

void finish_messenger (map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex, pthread_t *listen_server_thread)
{
	pthread_cancel(*listen_server_thread);

	for (map<string, contact*>::iterator it = (*contacts).begin(); it != (*contacts).end(); it++)
	{
		del_contact(it->first, contacts, groups, contacts_mutex);
	}
}





void list_contacts (map<string, contact*> *contacts)
{
	map<string, contact*>::iterator it;
	cout << "\t\t# Listando contatos\n \tNome" << endl;
	for(it = (*contacts).begin(); it != (*contacts).end(); ++it)
	{
		cout << "-" << it->first << endl;
	}
}

void open_conversation(string name, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex)
{
	string msg, clear;
	bool quit = false;
	map<string, contact*>::iterator it;


	it = (*contacts).find(name);
	contact* my_contact;

	if (it == (*contacts).end())
	{
		quit = true;
	}
	else
	{
	 	my_contact = it->second;
	}
	
	//Looping 
	while(quit == false)
	{
		//Finding the contact

		system("clear");
		
		cout << " # \t Conversa com " << name << "\n\n";
		//Listing the last thirty messages sent by this contact if the history isn't empty
		if(!my_contact->history->empty())
		{
			for(int i = 0; i < my_contact->history->size(); i++)
			{
				cout << *my_contact->history->at(i) << endl;
			}
		}
		else{
			cout << "Sem histórico de conversa" << endl;
		}

		//Waiting to send new messages
		cout << "\n\n\n\n" << "eu : ";
		getline(cin, msg);
		//Testing if the message is to leave the conversation screen
		if(msg == "-s")
		{
			quit = true;
		}
		//Testing if the message is to refresh the screen
		else if(msg == "-r")
		{
			system("clear");
		}
		//
		else
		{
			//Sending message to the contact and cleaning the buffer
			send_msg_contact (my_contact, msg, contacts, groups, contacts_mutex);
			msg.clear();
		}
		while(!msg.empty())
		{
			msg.clear();
		}
		system("clear");
	}

	system("clear");

	//Closing conversation
	close_conversation(name, contacts);
}

void close_conversation(string name, map<string, contact*> *contacts)
{
	map<string, contact*>::iterator it;

	//Storing only the last thirty messages on the history
	it = (*contacts).find(name);

	if (it == (*contacts).end())
		return;

	while(it->second->history->size() > 30)
	{
		it->second->history->erase(it->second->history->begin());
	}

}

void del_contact(string name, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex)
{
	if(exist_contact(name, contacts, contacts_mutex))
	{
		map<string, contact*>::iterator it_contact;
		map<string, group*>::iterator it_groups;
		int status, skt, i;
		
		//Entering critical section
		pthread_mutex_lock(contacts_mutex);

		
		//Searching for the contact on (*contacts) map
		it_contact = (*contacts).find(name);
		
		contact* my_contact = it_contact->second;
		skt = my_contact->skt_my;


		//Taking the contact off from (*contacts) map


		//Closing conexion
		status = shutdown(skt, SHUT_RDWR);
		if(status == -1)
			cout << "Error closing conexion" <<endl;
		

		//Free history contact stored
		for(i = 0; i < my_contact->history->size(); i++)
		{
			delete (my_contact->history->at(i));
		}
		delete(my_contact->history);

		//Free name string
		delete(my_contact->name);

		//Searching for the contact on groups map		
		for(it_groups = groups->begin(); it_groups != groups->end(); ++it_groups)
		{
			for(i = 0; i < it_groups->second->contacts->size(); i++)
			{
				//If the contact searched is on any group
				if(*(it_groups->second->contacts->at(i)->name) == name)
					it_groups->second->contacts->erase(it_groups->second->contacts->begin() + i);
			}
		}

		//Erase the contact from the map!
		contacts->erase(it_contact);

		//Free contact's memory
		free(my_contact);

		//Leaving critical section
		pthread_mutex_unlock(contacts_mutex);

		cout << "> Contato removido com sucesso!\n";
		cout << "> ";
	}
	else
	{
		cout << "> Contato nao existe!\n";
		cout << "> ";
	}
}



int set_socket_port (int socket, int port)
{
	struct sockaddr_in  socket_addr;
	socklen_t  socket_addr_sz = sizeof(struct sockaddr_in);

	getsockname(socket,										// COMO O SOCKET É CRIADO COM OS
				(struct sockaddr*) &socket_addr,			// DADOS DO SISTEMA, APENAS É NECESSÁRIO
				&socket_addr_sz);							// MUDAR O VALOR DA PORTA!!

	socket_addr.sin_port = htons(port);

	return bind(socket,										// O SERVIDOR NECESSITA DE UM SOCKET (ADDR,PORT) FIXO
				(struct sockaddr *) &socket_addr,			// PARA ESPERAR AS CONEXÕES!
				socket_addr_sz);
}


void* listen_server (void* args)
{
	struct sockaddr_in listen_addr;
	int sock_listen;
	string msg;


	struct listen_server_args func_args = *((struct listen_server_args*) args);


	map<string, contact*> *contacts = func_args.contacts;
	map<string, group*> *groups = func_args.groups;
	pthread_mutex_t *contacts_mutex = func_args.contacts_mutex;

	// Cria socket listen
	if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		cout << "> Erro no Socket\n";
		exit(1);
	}

	// Configura socket
	// Atribui endereco ao socket criado
	if (set_socket_port(sock_listen, LISTEN_PORT) == -1)
	{
		cout << "> Nao foi possivel realizar o bind (SAINDO!!)" << endl;
		exit(1);
	}


	if (listen(sock_listen, MAX_CONNECTIONS) == -1)
	{
		cout << "Erro em definir socket de espera de conexões" << endl;
		exit(1);
	}



	struct sockaddr client_addr;
	socklen_t client_addr_sz;

	int n_connections = 0;


	while (n_connections < MAX_CONNECTIONS)
	{
		contact* new_contact = (contact*) malloc(sizeof(contact));
		new_contact->n_new_msgs = 0;
		new_contact->history = new vector<string*>();

		new_contact->skt_my = accept(sock_listen,
										(struct sockaddr*) &client_addr,
										&client_addr_sz);



		int size_recv;
		int his_name_size = MAX_USERNAME_SIZE;
		char *his_name = (char *) malloc (his_name_size*sizeof(char));

		size_recv = recv(new_contact->skt_my, his_name, his_name_size*sizeof(char), 0);

		if (!exist_contact(his_name, contacts, contacts_mutex))
			new_contact->name = new string(his_name);
		else
		{
			stringstream ss;
			ss << his_name << "+";
			new_contact->name = new string(ss.str());
		}

		insert_contact(string(his_name), new_contact, contacts, groups, contacts_mutex);
	}
}


void send_msg (string name, string msg, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex)
{	
	//Sends the message to "name" and verify if it was delivered
	if (!exist_contact(name, contacts, contacts_mutex))
	{
		cout << "> Contato " << name << " nao existe" << endl;
		cout << "> ";
		return;
	}

	
	pthread_mutex_lock(contacts_mutex);
	
	int send_value = send((*contacts)[name]->skt_my, msg.c_str(), msg.length()*sizeof(char), 0);
	

	if (send_value == -1) {
		cout << "> Erro ao enviar mensagem para " << name << "\n";
		cout << "> ";

		//If it wasn't delivered, delete the contact
		del_contact(name, contacts, groups, contacts_mutex);
	}

	/*VERIFICAR SE PRECISA POR .C_STR() EM MSG*/
	if(!msg.empty())
	{
		stringstream ss;
		ss << "eu : " << msg;
		(*contacts)[name]->history->push_back(new string(ss.str()));
	}

	pthread_mutex_unlock(contacts_mutex);
}

void send_msg_contact (contact* cntct, string msg, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex)
{

	//send_msg(cntct.name, msg);

	pthread_mutex_lock(contacts_mutex);
	int send_value = send(cntct->skt_my, msg.c_str(), strlen(msg.c_str()), 0);

	//Sends the message to "name" and verify if it was delivered
	if (send_value == -1)
	{
		cout << "> Erro ao enviar mensagem para " << *(cntct->name) << "\n";
		cout << "> ";

		//If it wasn't delivered, delete the contact
		del_contact(*(cntct->name), contacts, groups, contacts_mutex);
	}

	/*VERIFICAR SE PRECISA POR .C_STR() EM MSG*/
	if(!msg.empty())
	{
		stringstream ss;
		ss << "eu : " << msg;
		cntct->history->push_back(new string(ss.str()));
	}

	pthread_mutex_unlock(contacts_mutex);
}

void send_group_msg (string gname, string msg, map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex)
{
	if (!exist_group(gname, groups))
	{
		cout << "> Grupo " << gname << " nao exite" << endl;
		cout << "> ";
		return;
	}

	vector<contact*> *ctcts = (*groups)[gname]->contacts;

	for (vector<contact*>::iterator it = ctcts->begin() ; it != ctcts->end(); ++it)
		send_msg_contact (*it, msg, contacts, groups, contacts_mutex);
}

void list_groups (map<string, group*> *groups)
{
	map<string, group*>::iterator it;
	cout << "\t\t# Listando grupos\n \tNome" << endl;
	for(it = (*groups).begin(); it != (*groups).end(); ++it)
	{
		cout << "-" << it->first << endl;
	}
}

void add_group (string gname, map<string, group*> *groups, map<string, contact*> *contacts, pthread_mutex_t* contacts_mutex)
{	
	// If already there's a group with this name, exit this function
	if (exist_group(gname, groups))
	{
		cout << "> Grupo " << gname << " ja existe!\n";
		cout << "> ";
		return;
	}
	else
	{
		bool flag;
		vector<contact*> *aux_contacts = new vector<contact*>();
		string cname;

		cout << "> Digite o nome do contato que deseja adicionar ao grupo " << gname << " ou exit_add_group para sair: ";
		cout << "> ";
		cin >> cname;
		
		while (cname != "exit_add_group")
		{
			flag = false;
			if (!exist_contact(cname, contacts, contacts_mutex))
			{
				cout << "> Contato nao existe!\n";
				cout << "> ";
			}
			else
			{	
				if(!aux_contacts->empty())
				{
					int i;
					for(i = 0; (i < aux_contacts->size()) && (flag == false); i++)
					{
						if(*(aux_contacts->at(i)->name) == cname)
							flag = true;
					}
				}
				if(flag == false)
				{
					aux_contacts->push_back((*contacts)[cname]);
				}

			}
			cout << "Digite o nome do contato que deseja adicionar ao grupo " << gname << " ou exit_add_group para sair: ";
			cin >> cname;
		}

		group *g = (group*) malloc (sizeof(group));

		g->name = &gname;
		g->contacts = aux_contacts;

		groups->insert(make_pair(gname, g));
	}
}

void rem_gruop (string gname, map<string, group*> *groups)
{
	if (!exist_group(gname, groups))
	{
		cout << "> Grupo " << gname << " nao existe!\n";
		cout << "> ";
		return;
	}
	else
	{
		map<string, group*>::iterator it;

		//Search for the group named "gname" and delete it from the map of groups
		for (it = (*groups).begin(); it != (*groups).end(); ++it)
		{
			//When the group is found, erase it from the map
			if (it->first == gname)
			{
				groups->erase(it);
				break;
			}
		}
	}
}

void main_menu(map<string, contact*> *contacts, map<string, group*> *groups, pthread_mutex_t *contacts_mutex, string my_name)
{
	string option;

	do
	{
		cout << endl << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~" << endl << endl;

		cout << "adicionar : Adicionar um contato" << endl;
		cout << "listar    : Listar contatos" << endl;
		cout << "remover   : Remover um contato" << endl;
		cout << "open_conv : Enviar uma mensagem privada" << endl;
		cout << "add_grupo : Adicionar grupo" << endl;
		cout << "list_grupo: Listar grupos" << endl;
		cout << "rem_grupo : Remover grupo" << endl;
		cout << "env_grupo : Enviar uma mensagem em grupo" << endl;
		cout << "sair      : Sair do programa" << endl;

		cout << "> ";
		cin >> option;


		if (option == "adicionar")
		{
			string name;
			string ip;

			cout << "Nome do contato: ";
			cin >> name;

			cout << "IP do contato: ";
			cin >> ip;

			add_contact(name, ip, contacts, groups, contacts_mutex, my_name);
		}
		else if (option == "listar")
		{
			system("clear");

			list_contacts(contacts);
		}
		else if (option == "remover")
		{
			string name;
			string ip;

			cout << "Nome do contato: ";
			cin >> name;
			
			if (exist_contact(name, contacts, contacts_mutex))
			{
				pthread_cancel((*contacts)[name]->receiver);
				del_contact(name, contacts, groups, contacts_mutex);
			}
			else
			{
				cout << "Contato nao existe!\n";
			}
		}
		else if (option == "open_conv")
		{
			string who;

			cout << "De quem? ";
			cin >> who;

			open_conversation(who, contacts, groups, contacts_mutex);
		}
		else if (option == "add_grupo")
		{	
			string gname;

			cout << "Digite o nomedo grupo: ";
			cin >> gname;
		
			add_group(gname, groups, contacts, contacts_mutex);
		}
		else if (option == "list_grupo")
		{
			system("clear");
			
			list_groups (groups);
		}
		else if (option == "rem_grupo")
		{
			string gname;

			cout << "Digite o nomedo grupo: ";
			cin >> gname;

			rem_gruop(gname, groups);
		}
		else if (option == "env_grupo")
		{
			string gname;
			string msg;

			cout << "Nome do grupo: ";
			cin >> gname;

			cout << "Mensagem a ser enviada: ";
			cin >> msg;

			send_group_msg (gname, msg, contacts, groups, contacts_mutex);

		}
		else if (option == "sair")
		{
			cout << "> O pograma esta finalizando..." << endl;
		}
		else
		{
			cout << "> Opção inválida!!!" << endl;
		}

	}while(option != "sair");
}

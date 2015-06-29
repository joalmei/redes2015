#ifndef CONTACT_HH
#define CONTACT_HH

#include <iostream>
#include <sys/socket.h>
#include <string>
#include <vector>

using namespace std;

typedef struct s_contact
{
	string* name;
	
	int skt_my;

	int n_new_msgs;	
	vector<string*> *history;

	pthread_t receiver;
}contact;

#endif
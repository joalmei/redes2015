#ifndef GROUP_HH
#define GROUP_HH

#include <iostream>
#include <string.h>
#include <vector>

#include "contact.hh"

using namespace std;

typedef struct s_group
{
	string* name;
	vector<contact*> *contacts;
}group;

#endif
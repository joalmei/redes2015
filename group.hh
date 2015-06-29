/***********************************************************************************************
**--------------------------------------------------------------------------------------------**
**||                             SSC0641-Redes de Computadores                              ||**
**||                                       Projeto                                          ||**
**||            Alunos: Cassiano Zaghi de Oliveira      : 7987400                           ||**
**||                    João Victor Almeida de Aguiar   : 8503986                           ||**
**||                    Lucas Tomazela                  : 8124271                           ||**
**--------------------------------------------------------------------------------------------**
************************************************************************************************/

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
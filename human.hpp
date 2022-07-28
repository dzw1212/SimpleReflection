#pragma once
#include "Reflection.hpp"


CLASS(Human, WhiteList)
{
public:
	int Age;
	META(Disable)
	int Gender;
	char* Name;
	char* Tel;
};

STRUCT(Puppy, BlackList)
{
	int Age;
	char* Name;
	META(Enable)
	int Color;
	META(Enable)
	Human Dom
}

#pragma once
#include<memory>
#include<string>
#include"Footballer.h"

class FootballerFactory {
public:
	static std::unique_ptr<Footballer> create(const std::string& name, int age, const std::string& position, const std::string& team, int s1, int s2, int s3, int s4, int s5);
};
#pragma once
#include <string>
#include <exception>
#include <vector>
#include <map>

using namespace std;

//typedefs
using opcode		= unsigned short;
using instruction	= unsigned short;

//singleton class of the vector compiler
class vcomp
{
private:
	vcomp() = default;
	vcomp(vcomp&) = delete;
	vcomp(vcomp&&) = delete;

public:
	auto compile(string, bool from_file) -> void;
	auto generate_binary(string out_path) -> void;

	static auto get_compiler()->vcomp&;

	


private:
	auto __compile_from_disk(string path) -> void;
	auto __compile_from_string(string text) -> void ;
	
private:
	
	static map<string, opcode> ISA;
	vector<instruction> binary = {};
};


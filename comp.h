#pragma once
#include <string>
#include <exception>
#include <vector>
#include <map>
#include <tuple>
using namespace std;

//typedefs
using opcode		= unsigned short;
using instruction	= unsigned short;
using memword		= short;
using address		= unsigned short;

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
	auto __compile_data(vector<string>&) -> void;
	auto __compile_text(vector<string>&) -> void;
	auto __analyze_inst(string line, unsigned line_no) -> void;
	auto __inst_type(string inst) -> unsigned;
	auto __sum_metadata(vector<string>&, unsigned)->tuple<opcode, unsigned int, vector<unsigned short>,pair<bool, short>>;
	auto __remove_comments(string& cmd_line) -> void;
	auto __discover_loops(vector<string>&) -> void;
	
private:
	
	static map<string, opcode> ISA;
	vector<instruction>	binary = {};
	map<string, pair<memword, address>> memlocations = {};
	map<string, unsigned> looplocations = {};
};


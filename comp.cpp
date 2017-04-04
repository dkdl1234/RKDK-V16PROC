#include <sstream>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <bitset>
#include <algorithm>
#include <iterator>
#include "comp.h"

#define INSTRUCTION_LENGTH 16
#define OPCODE_BITLENGTH 4

//helper methods
auto __read_stream(ifstream& stream) -> string {
	string program;
	stream.seekg(0, ios::end);
	program.reserve(stream.tellg());
	stream.seekg(0, ios::beg);

	//read the entire file data
	program.assign(	istreambuf_iterator<char>(stream), 
					istreambuf_iterator<char>());

	return program;
}


auto __break_inst(string inst_line) -> vector<string> {
	istringstream istream(inst_line);
	return vector<string>{ istream_iterator<string>{ istream }, istream_iterator<string>{} };
}


//---------------------------------------------------------------------------------
//Compiler definition

map<string, opcode> vcomp::ISA = {	{"NOP",		0x0},
									{"MUL.V",	0x1},
									{"ADD.V",	0x2},
									{"ADD.S",	0x3},
									{"REDADD.V", 0x4},
									{"ADDI.S",	0x5},
									{"MULI.S",	0x6},
									{"NANDS.S", 0x7},
									{"LV",		0x8},
									{"SV",		0x9},
									{"LW",		0xA},
									{"SW",		0xB},
									{"BNE",		0xC},
									{"JMP",		0xD},
									{"SNEVS.D", 0XE},
									{"CVM",		0XF} };


auto vcomp::get_compiler() -> vcomp& {
	static vcomp Compiler;
	return Compiler;
}

//two options - compile a saved file, or compile a direct string
auto vcomp::compile(string str, bool from_file) -> void
{
	if (from_file)	this->__compile_from_disk(str);
	else			this->__compile_from_disk(str);
}



auto vcomp::generate_binary(string out_path) -> void
{
	ofstream out_stream(out_path);
	if (!out_stream.is_open())
		throw std::exception("Bad file path");

	else {
		//convert short to bits, then write the data to the file
		for (auto const& inst : this->binary){
			auto bin_inst = bitset<INSTRUCTION_LENGTH>(inst);
			out_stream << bin_inst.to_string() << std::endl;
		}
	}
	out_stream.close();
}

auto vcomp::__compile_from_disk(string path) -> void
{
	std::ifstream in_stream(path);
	if (!in_stream.is_open()) 
		throw std::exception("Bad file path");

	else {
		//save enough space in the program string memory
		string program = __read_stream(in_stream);

		//compile the string program
		this->__compile_from_string(program);
	}

	in_stream.close();
}

auto vcomp::__compile_from_string(string text) -> void
{
	istringstream istream(text);
	//break the whole program to lines
	vector<string> lines;
	string line;
	while (getline(istream, line, '\n')) {
		lines.push_back(line);
	}
	
	auto text_line = -1;
	for (auto i = 0; i < lines.size(); i++) {
		if (lines[i].find(".text") != string::npos) {
			text_line = i;
			break;
		}
	}

	if (text_line < 0)
		throw exception("No instruction found!");

	//remove all the lines that are not instructions (e.g data section etc.)
	lines.erase(begin(lines), begin(lines) + text_line);
	
	//compile the text lines
	for (auto const& line : lines) {
		auto tokens = __break_inst(line);
		if (".text" == tokens[0]) continue;
		//else
		try
		{
			//for now, calculate only the opcode and save it as the whole instruction
			string inst = tokens[0];
			unsigned short opcode = vcomp::ISA.at(inst);
			bitset<INSTRUCTION_LENGTH> set(opcode);
			set <<= INSTRUCTION_LENGTH - OPCODE_BITLENGTH;
			this->binary.push_back(static_cast<unsigned short>(set.to_ulong()));
		}
		catch (...)
		{
			throw;
		}
	}
}




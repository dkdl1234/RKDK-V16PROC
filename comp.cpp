#include <sstream>
#include <fstream>
#include <iostream>
#include <streambuf>
#include <bitset>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <cctype>
#include "comp.h"

#define BYTE_BITLENGTH 8
#define INSTRUCTION_BITLENGTH 16
#define INSTRUCTION_BYTELENGTH INSTRUCTION_BITLENGTH / BYTE_BITLENGTH
#define OPCODE_BITLENGTH	4
#define REGISTER_BITLENGTH	4
#define IMMEDIATE_MASK		0x000F
#define OPCODE_MASK			0xF000

typedef enum ISA {	NOP = 0, 
					MULV,
					ADDV,
					ADDS,
					REDADDV,
					ADDIS,
					MULIS,
					NANDS,
					LV,
					SV,
					LW,
					SW,
					BNE,
					JMP,
					SNEVSD,
					CVM} ISA;

typedef enum INSTYPE {RTYPE=0, ITYPE, JTYPE, CTYPE} INSTYPE;

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


auto __break_line(string inst_line) -> vector<string> {
	istringstream istream(inst_line);
	return vector<string>{ istream_iterator<string>{ istream }, istream_iterator<string>{} };
}


auto __break_program(string text) -> vector<string> {
	istringstream istream(text);
	//break the whole program to lines
	vector<string> lines;
	string line;
	while (getline(istream, line, '\n')) {
		lines.push_back(line);
	}

	return lines;
}


auto __get_inst_type(ISA opcode) -> INSTYPE {
	switch (opcode)
	{
	case NOP:		return RTYPE;	break;
	case MULV:		return RTYPE;	break;
	case ADDV:		return RTYPE;	break;
	case ADDS:		return RTYPE;	break;
	case REDADDV:	return RTYPE;	break;
	case ADDIS:		return ITYPE;	break;
	case MULIS:		return ITYPE;	break;
	case NANDS:		return RTYPE;	break;
	case LV:		return RTYPE;	break;
	case SV:		return RTYPE;	break;
	case LW:		return ITYPE;	break;
	case SW:		return RTYPE;	break;
	case BNE:		return JTYPE;	break;
	case JMP:		return JTYPE;	break;
	case SNEVSD:	return CTYPE;	break;
	case CVM:		return CTYPE;	break;
	default:		throw std::exception("Unrecognized opcode");
	}
}


auto is_number(const std::string& s) -> bool
{
	return !s.empty() && std::find_if(s.begin(),
		s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}


auto ishex(const string& s) {
	return "0X" == s.substr(0, 2) || "0x" == s.substr(0, 2) || 'x' == s[0] || 'X' == s[0];
}
//---------------------------------------------------------------------------------
//Compiler definition

map<string, opcode> vcomp::ISA = {	{"NOP",		ISA::NOP},
									{"MUL.V",	ISA::MULV},
									{"ADD.V",	ISA::ADDV},
									{"ADD.S",	ISA::ADDS},
									{"REDADD.V",ISA::REDADDV},
									{"ADDI.S",	ISA::ADDIS},
									{"MULI.S",	ISA::MULIS},
									{"NAND.S",	ISA::NANDS},
									{"LV",		ISA::LV},
									{"SV",		ISA::SV},
									{"LW",		ISA::LW},
									{"SW",		ISA::SW},
									{"BNE",		ISA::BNE},
									{"JMP",		ISA::JMP},
									{"SNEVS.D", ISA::SNEVSD},
									{"CVM",		ISA::CVM} };


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
			auto bin_inst = bitset<INSTRUCTION_BITLENGTH>(inst);
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
	auto lines = __break_program(text);
	
	//split the program to .data and .text sections
	auto text_line = -1;
	for (unsigned  i = 0; i < lines.size(); i++) {
		if (lines[i].find(".text") != string::npos) {
			text_line = i;
			break;
		}
	}
	
	if (text_line < 0)
		throw exception("No instruction found!");

	//compile the data section
	vector<string> data_section(begin(lines), begin(lines) + text_line);
	this->__compile_data(data_section);

	//compile the text section
	vector<string> text_section(begin(lines) + text_line, end(lines));
	lines.clear();
	this->__compile_text(text_section);
}

auto vcomp::__compile_data(vector<string>& lines) -> void
{
	unsigned memory_counter = 0;
	for (auto& line : lines){

		__remove_comments(line);
		
		if (line.find(".data") != string::npos) continue;
		
		//else
		auto tokens = __break_line(line);
		if (tokens.empty()) continue; //empty line

		if (tokens.size() != 4) throw exception("Data declaration isn't correct!");

		//memory argument already exists, cannot redeclare 
		if (this->memlocations.find(tokens.front()) != cend(this->memlocations)) 
			throw std::exception((string("Argument ") + tokens.front() + string(" already decalred!")).c_str());

		auto memory_data = make_pair((memword)stoul(tokens.back(), nullptr, ishex(tokens.back()) ? 16 : 10), memory_counter);
		this->memlocations[tokens.front()] = memory_data;

		memory_counter += sizeof(memword);
	}
}

auto vcomp::__compile_text(vector<string>& lines) -> void
{
	this->__discover_loops(lines);
	//compile the text lines
	for (size_t i = 0; i < lines.size(); i++) {

		//else
		try
		{
			this->__analyze_inst(lines[i], i);
		}
		catch (...)
		{
			if (lines[i].find(".text") != string::npos) continue;
			else throw;
		}
	}
}

auto vcomp::__analyze_inst(string line, unsigned line_no) -> void
{
	//one-timer helpers
	auto is_arithmetic_op	= [](enum ISA opcd) {return MULV == opcd || ADDV == opcd || ADDS == opcd || ADDIS == opcd || MULIS == opcd; };
	auto is_memory_op		= [](enum ISA opcd) {return LV == opcd || SV == opcd || LW == opcd || SW == opcd; };
	
	
	this->__remove_comments(line);
	auto tokens = __break_line(line);
	
	//analyze the operands
	unsigned short opcd;
	unsigned num_operands;
	vector<unsigned short> registers;
	pair<bool, short> immediate;
	
	//analyze the operands now
	tie(opcd, num_operands, registers, immediate) = this->__sum_metadata(tokens, line_no);
	auto isa_code = (enum ISA)opcd;

	//create the binary instruction
	bitset<INSTRUCTION_BITLENGTH> bininst(opcd);
	bininst <<= INSTRUCTION_BITLENGTH - OPCODE_BITLENGTH;

	//assemble the binary instruction 
	switch (__get_inst_type(isa_code))
	{
	case RTYPE: 
		if (NOP == isa_code) bininst = 0x0000;	//NOP
		else if (REDADDV == isa_code || SW == isa_code) {			//Reduce add
			bitset<INSTRUCTION_BITLENGTH> bitdst(registers.at(0)), bitsrc(registers.at(1));
			bitdst <<= REGISTER_BITLENGTH * 2;
			bitsrc <<= REGISTER_BITLENGTH;

			bininst |= (bitdst | bitsrc);
		}
		else {									//other r-type instructions
			bitset<INSTRUCTION_BITLENGTH> bitdst(registers.at(0)), bitreg1(registers.at(1)), bitreg2(registers.at(2));
			bitdst <<= REGISTER_BITLENGTH * 2;
			bitreg1 <<= REGISTER_BITLENGTH;

			bininst |= (bitdst | bitreg1 | bitreg2);
		}
		break;
	case ITYPE:	
		if (!immediate.first) throw exception("ITYPE operation must have an immediate field");
		if (is_arithmetic_op(isa_code)) {
			bitset<INSTRUCTION_BITLENGTH> bitdst(registers.at(0)), bitsrc(registers.at(1)), bitimm(immediate.second);
			bitimm &= IMMEDIATE_MASK;
			bitdst <<= REGISTER_BITLENGTH * 2;
			bitsrc <<= REGISTER_BITLENGTH;

			bininst |= (bitdst | bitsrc | bitimm);

		}
		else {	//memory operation
			bitset<INSTRUCTION_BITLENGTH> bitsrc(registers.at(0)), bitimm(immediate.second);
			bitsrc <<= REGISTER_BITLENGTH * 2;
			bitimm &= IMMEDIATE_MASK;

			bininst |= (bitsrc | bitimm);
		}
		break;
	case JTYPE:	
		if (!immediate.first) throw exception("JTYPE operation must have an immediate field");
		if (JMP == isa_code){
			bitset<INSTRUCTION_BITLENGTH> bitimm(immediate.second);
			bitimm &= 0x0FFF;

			bininst |= (bitimm);
		}
		else{
			bitset<INSTRUCTION_BITLENGTH> bitimm(immediate.second), bitsrc1(registers.at(0)), bitsrc2(registers.at(1));
			bitsrc1 <<= REGISTER_BITLENGTH * 2;
			bitsrc2 <<= REGISTER_BITLENGTH;
			bitimm &= IMMEDIATE_MASK;

			bininst |= (bitsrc1 | bitsrc2 | bitimm);
		}
		break;
	case CTYPE:	
		if (SNEVSD == isa_code){
			bitset<INSTRUCTION_BITLENGTH> bitsrc1(registers.at(0)), bitsrc2; (registers.at(1));
			bitsrc1 <<= REGISTER_BITLENGTH * 2;
			bitsrc2 <<= REGISTER_BITLENGTH;

			bininst |= (bitsrc1 | bitsrc2);
		}
		break;
	}

	//save the instruction
	this->binary.push_back(static_cast<unsigned short>(bininst.to_ulong()));
}

auto vcomp::__inst_type(string inst) -> unsigned
{
	if (inst == "NOP" || inst == "MUL.V" || inst == "ADD.V" ||
		inst == "ADD.S" || inst == "REDADD.V" || inst == "NAND.S" ||
		inst == "LV" || inst == "SV" || inst == "LW" || inst == "SW")	return RTYPE;
	else if (inst == "ADDI.S" || inst == "MULI.S")						return ITYPE;
	else if (inst == "BNE" || inst == "JMP")							return JTYPE;
	else if (inst == "SNEVS.D" || inst == "CVM")						return CTYPE;
	else throw exception("Unrecognizable instruction");
}

auto vcomp::__sum_metadata(vector<string>& line, unsigned line_number) -> tuple<opcode, unsigned int, vector<unsigned short>, pair<bool, short>>
{
	string inst = line[0];
	unsigned opcd = vcomp::ISA.at(inst);
	line.erase(begin(line), begin(line) + 1);

	unsigned int num_operands = line.size(); //size of the vector minus the instruction
	vector<unsigned short> registers;
	registers.reserve(num_operands);

	pair<bool, signed short> immediate = make_pair(false, -1);
	//interpret the remaining operands
	for (unsigned i = 0; i < line.size(); i++) {
		//register inspection
		if ('$' == line[i][0]) {
			if ('R' == line[i][1] || 'V' == line[i][1]) {
				//auto reg_str = line[i].substr(2, std::string::npos); //for debug perposes
				registers.push_back((unsigned short)stoi(line[i].substr(2, std::string::npos)));
			}
			else //unknown register
				throw exception((string("No register named ") + line[i].substr(1)).c_str());
			
		}
		
		else if (is_number(line[i]))
			immediate = make_pair(true, stoi(line.back()));
		

		//memory/loop label inspection (memory)
		else {
			//memory
			if (this->memlocations.find(line[i]) != cend(this->memlocations)) {
				auto _address = this->memlocations.at(line[i]).second;
				if (!immediate.first) immediate = make_pair(true, _address);
			}
			//loop
			else if (this->looplocations.find(line[i]) != cend(this->looplocations)) {
				auto loop_line_num = this->looplocations[line[i]];
				signed _offset = ((signed)(loop_line_num - line_number)) * sizeof(instruction);
				immediate = make_pair(true, _offset);
			}
			else throw exception("Immediate value and memory address value cannot be in the same instruction!");
		}
	}
	
	registers.shrink_to_fit();
	
	return make_tuple(opcd, num_operands, registers, immediate);
}

auto vcomp::__remove_comments(string & cmd_line) -> void
{
	auto _pos = cmd_line.find("#");
	if (_pos != std::string::npos) 
		cmd_line.erase(_pos);
	
}

auto vcomp::__discover_loops(vector<string>& lines) -> void
{
	for (size_t i = 0; i < lines.size(); i ++) {
		if (lines[i].find(".text") != string::npos) continue;
		
		auto tokens = __break_line(lines[i]);

		auto & potential_loop = tokens[0];
		

		//first token in each instruction line is either a loop label or an ISA instrucion
		//if the first token isn't an ISA instruction than it must be a loop
		if (vcomp::ISA.find(potential_loop) == cend(vcomp::ISA)) {
			auto & loop = potential_loop;
			loop.erase(loop.find(":"), string::npos);
			if (this->looplocations.find(loop) == cend(this->looplocations)) {
				this->looplocations.insert(make_pair(loop, i));
				
				//reconstruct the line without the loop label
				tokens.erase(begin(tokens), begin(tokens) + 1);
				ostringstream s;
				copy(begin(tokens), end(tokens), ostream_iterator<string>(s, " "));
				lines[i] = s.str();
			}
			else throw exception((string("Loop named: ") + loop + string(" appears more than once in the program!")).c_str());
		}
	}
}





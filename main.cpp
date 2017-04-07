#include "comp.h"
#include <iostream>

auto main() -> int {
	auto& compiler = vcomp::get_compiler();
	try{
		compiler.compile("./programs/MATMUL.txt", true);
	}
	catch (const std::exception& exp){
		std::cerr << exp.what() << std::endl;
		return -1;
	}
	compiler.generate_binary("./bin/MATMUL.v");
	return 0;
}
#include "comp.h"

auto main() -> int {
	auto& compiler = vcomp::get_compiler();
	compiler.compile("./programs/MATMUL.txt", true);
	compiler.generate_binary("./bin/MATMUL.v");
	return 0;
}
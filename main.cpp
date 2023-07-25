#include <fstream>
#include <iostream>
#include <string>

constexpr const char* REGISTER_MEMORY_FIELD_DECODE[16] =
{
	// REG can be R/M, in case MOD = 0b11 - it's the same mapping
	"al", // REG = 000 W = 0
	"ax", // REG = 000 W = 1
	"cl", // REG = 001 W = 0
	"cx", // REG = 001 W = 1
	"dl", // REG = 010 W = 0
	"dx", // REG = 010 W = 1
	"bl", // REG = 011 W = 0
	"bx", // REG = 011 W = 1
	"ah", // REG = 100 W = 0
	"sp", // REG = 100 W = 1
	"ch", // REG = 101 W = 0
	"bp", // REG = 101 W = 1
	"dh", // REG = 110 W = 0
	"si", // REG = 110 W = 1
	"bh", // REG = 111 W = 0
	"di"  // REG = 111 W = 1
};

std::string DecodeBinary8086(uint8_t* instruction)
{
	// Get flags and make sure that opcode is ok
	const bool wide_flag = instruction[0] & 1;
	instruction[0] >>= 1;
	const bool destination_flag = instruction[0] & 1;
	instruction[0] >>= 1;
	// Decode only mov instructions for the start
	constexpr uint8_t MOV_OPCODE = 0b100010;
	constexpr uint8_t OPCODE_MASK = ~(~1u << 5);
	if (instruction[0] ^ MOV_OPCODE)
	{
		// Wrong opcode, should be 0x90 for MOV
		return "";
	}

	std::string res;
	res.append("mov ");

	// Deduce registers
	constexpr uint8_t REG_MASK = ~(~1u << 2);
	const uint8_t source_reg_code = ((instruction[1] & REG_MASK) << 1) + wide_flag;
	const uint8_t destination_reg_code = (((instruction[1] >> 3) & REG_MASK) << 1) + wide_flag;
	res += REGISTER_MEMORY_FIELD_DECODE[source_reg_code];
	res += ", ";
	res += REGISTER_MEMORY_FIELD_DECODE[destination_reg_code];
	return res;
}

int main(int argc, char** argv)
{
	constexpr const char* input_filename = "listing_0038_many_register_mov";
	constexpr const char* output_filename = "code.asm";
	std::ifstream in_fstream{ input_filename, std::ios::binary};
	std::ofstream out_fstream{ output_filename };
	if (!in_fstream)
	{
		std::cerr << "Could not open file read: " << input_filename << std::endl;
		return 1;
	}

	if (!out_fstream)
	{
		std::cerr << "Could not open file for write: " << output_filename << std::endl;
		return 1;
	}

	// Header for assembly
	out_fstream << "bits 16\n";

	while(!in_fstream.eof())
	{
		uint8_t instruction[2];
		// TODO: There is a problem of binary file ending, how to detect properly?
		in_fstream.read(reinterpret_cast<char*>(&instruction), 2 * sizeof(uint8_t));
		out_fstream << DecodeBinary8086(instruction) << "\n";
	}
	in_fstream.close();
	out_fstream.close();

	return 0;

}

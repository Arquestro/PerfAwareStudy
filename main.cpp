#include <fstream>
#include <iostream>
#include <string>

constexpr uint8_t CountTrailingZeros(uint8_t x)
{
	// From here http://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightModLookupunsigned
	// A small hack is used here to widen uint8_t to int
	int v = static_cast<int>(x);           // find the number of trailing zeros in v
	constexpr uint8_t Mod37BitPosition[] = // map a bit value mod 37 to its position
	{
	  32, 0, 1, 26, 2, 23, 27, 0, 3, 16, 24, 30, 28, 11, 0, 13, 4,
	  7, 17, 0, 25, 22, 31, 15, 29, 10, 12, 6, 0, 21, 14, 9, 5,
	  20, 8, 19, 18
	};
	return Mod37BitPosition[(-v & v) % 37];
}


constexpr uint8_t MOD_FIELD_MASK = 0b11000000;
constexpr uint8_t REG_MASK = 0b00111000;
constexpr uint8_t REG_MEM_MASK = 0b00000111;
static_assert((MOD_FIELD_MASK | REG_MASK | REG_MEM_MASK) == 0b11111111, "Masks should fill all byte together");
static_assert((MOD_FIELD_MASK & REG_MASK & REG_MEM_MASK) == 0, "Masks can't intersect with each other");
constexpr uint8_t MOD_MEM_NO_DISP = 0b00000000;
constexpr uint8_t MOD_MEM_BYTE_DISP = 0b01000000;
constexpr uint8_t MOD_MEM_DWORD_DISP = 0b10000000;
constexpr uint8_t MOD_REG = 0b11000000;

#define BINARY_LEFT_BITS_MASK(X) (~1u << (CountTrailingZeros(X) - 1))
#define GET_DWORD_FROM_TWO_BYTES(LOW, HIGH) \
	static_cast<uint16_t>(HIGH) << 8 | static_cast<uint16_t>(LOW); \
	static_assert(sizeof(LOW) == 1, "Low should be one byte size!"); \
	static_assert(sizeof(HIGH) == 1, "High should be one byte size!")


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

constexpr const char* EFFECTIVE_ADDRESS_DECODE[8] =
{
	"bx + si", // R/M = 000
	"bx + di", // R/M = 001 
	"bp + si", // R/M = 010
	"bp + di", // R/M = 011
	"si",      // R/M = 100
	"di",      // R/M = 101 
	"bp",      // R/M = 110 
	"bx"       // R/M = 111
};

// Pair for deducing the opcode of the binary instruction
enum InstructionType : uint8_t
{
	REG_TO_REG_MOV = 0,
	IMMEDIATE_TO_REG_MEM_MOV,
	IMMEDIATE_TO_REG_MOV,
	NUMBER_OF_INSTRUCTION_TYPES
};

struct InstructionMetaInfo
{
	uint8_t opcode;
	uint8_t mask;
};

#define MAKE_OPCODE_WITH_MASK(OPCODE) { OPCODE, BINARY_LEFT_BITS_MASK(OPCODE)}

constexpr InstructionMetaInfo INSTRUCTION_META_INFO[] =
{
	MAKE_OPCODE_WITH_MASK(0b10001000), // REG_TO_REG_MOV
	MAKE_OPCODE_WITH_MASK(0b11000110), // IMMEDIATE_TO_REG_MEM_MOV
	MAKE_OPCODE_WITH_MASK(0b10110000)  // IMMEDIATE_TO_REG_MOV
};

inline bool InstructionIsOpcode(uint8_t first_instruction_byte, const InstructionMetaInfo& opcode_with_mask)
{
	return !((first_instruction_byte & opcode_with_mask.mask) ^ opcode_with_mask.opcode);
}

InstructionType DeduceOpcode(uint8_t first_instruction_byte)
{
	for (uint8_t i = 0; i < NUMBER_OF_INSTRUCTION_TYPES; ++i)
	{
		if(InstructionIsOpcode(first_instruction_byte, INSTRUCTION_META_INFO[i]))
		{
			return static_cast<InstructionType>(i);
		}
	}
	std::cerr << "ERROR: Wrong type of opcode for instruction first byte " << std::to_string(first_instruction_byte) << std::endl;
	return NUMBER_OF_INSTRUCTION_TYPES;
}

constexpr const char* NOT_IMPLEMENTED_YET = "NOT IMPLEMENTED INSTRUCTION";

std::string DecodeImmediateToRegMov(uint8_t* instruction_buffer, std::ifstream* in_fstream)
{
	std::string res = "mov ";
	const bool wide_flag = (instruction_buffer[0] & 1u << 3) >> 3;
	const uint8_t destination_reg_code = ((instruction_buffer[0] & REG_MEM_MASK) << 1) + wide_flag;
	res.append(REGISTER_MEMORY_FIELD_DECODE[destination_reg_code]);
	res.append(", ");
	// Load additional bytes of instruction, one if not wide, two otherwise
	if(wide_flag)
	{
		in_fstream->read(reinterpret_cast<char*>(&instruction_buffer[1]), 2 * sizeof(uint8_t));
		uint16_t data = GET_DWORD_FROM_TWO_BYTES(instruction_buffer[1], instruction_buffer[2]);
		res.append(std::to_string(data));
	}
	else
	{
		in_fstream->read(reinterpret_cast<char*>(&instruction_buffer[1]), sizeof(uint8_t));
		res.append(std::to_string(instruction_buffer[1]));
	}
	return res;
}

std::string DecodeImmediateToRegMemMov(uint8_t* instruction, std::ifstream* in_fstream)
{
	return NOT_IMPLEMENTED_YET;
}

std::string DecodeRegToRegMov(uint8_t* instruction_buffer, std::ifstream* in_fstream)
{
	// TODO: Displacement load and handling
	std::string res = "mov ";
	// Get flags
	const bool wide_flag = instruction_buffer[0] & 1;
	const bool destination_flag = instruction_buffer[0] >> 1 & 1;
	// Load additional bytes of instruction_buffer
	in_fstream->read(reinterpret_cast<char*>(&instruction_buffer[1]), sizeof(uint8_t));
	const uint8_t mod_field = instruction_buffer[1] & MOD_FIELD_MASK;
	// Deduce registers
	const uint8_t reg_mem_val = instruction_buffer[1] & REG_MEM_MASK;
	const uint8_t reg_mem_code = (reg_mem_val << 1) + wide_flag;
	const uint8_t reg_code = ((instruction_buffer[1] & REG_MASK) >> (CountTrailingZeros(REG_MASK) - 1)) + wide_flag;
	std::string first_op;
	std::string second_op = REGISTER_MEMORY_FIELD_DECODE[reg_code];
	switch (mod_field)
	{
	case (MOD_MEM_NO_DISP):
	{
		if(reg_mem_val != 0b110)
		{
			first_op = std::string("[") + EFFECTIVE_ADDRESS_DECODE[reg_mem_val] + std::string("]");
		}
		else
		{
			in_fstream->read(reinterpret_cast<char*>(&instruction_buffer[2]), 2 * sizeof(uint8_t));
			uint16_t displacement_val = GET_DWORD_FROM_TWO_BYTES(instruction_buffer[2], instruction_buffer[3]);
			std::string displacement = std::string(" + ") + std::to_string(displacement_val) + std::string("]");
			first_op = std::string("[") + EFFECTIVE_ADDRESS_DECODE[reg_mem_val] + displacement;
		}
		break;
	}
	case (MOD_MEM_BYTE_DISP):
	{
		in_fstream->read(reinterpret_cast<char*>(&instruction_buffer[2]), sizeof(uint8_t));
		// Case for 0 displacement
		std::string displacement = instruction_buffer[2] ? std::string(" + ") + std::to_string(instruction_buffer[2]) + std::string("]") : "]";
		first_op = std::string("[") + EFFECTIVE_ADDRESS_DECODE[reg_mem_val] + displacement;
		break;
	}
	case (MOD_MEM_DWORD_DISP):
	{
		in_fstream->read(reinterpret_cast<char*>(&instruction_buffer[2]), 2 * sizeof(uint8_t));
		uint16_t displacement_val = GET_DWORD_FROM_TWO_BYTES(instruction_buffer[2], instruction_buffer[3]);
		// Case for 0 displacement
		std::string displacement = displacement_val ? std::string(" + ") + std::to_string(displacement_val) + std::string("]") : "]";
		first_op = std::string("[") + EFFECTIVE_ADDRESS_DECODE[reg_mem_val] + displacement;
		break;
	}
	case (MOD_REG):
		first_op = REGISTER_MEMORY_FIELD_DECODE[reg_mem_code];
		break;
	default:
		std::cerr << "Error: something went wrong calculating the MOD field!" << std::endl;
	}

	if (destination_flag)
	{
		first_op.swap(second_op);
	}
	res.append(first_op);
	res.append(", ");
	res.append(second_op);
	return res;
}

std::string DecodeBinary8086(uint8_t* instruction, std::ifstream* in_fstream, InstructionType type)
{
	typedef std::string(*InstructionDecodeFunc) (uint8_t*, std::ifstream*);
	constexpr InstructionDecodeFunc decode_callbacks[] = {
		DecodeRegToRegMov,
		DecodeImmediateToRegMemMov,
		DecodeImmediateToRegMov
	};
	static_assert(std::size(decode_callbacks) == NUMBER_OF_INSTRUCTION_TYPES, "Every type should have corresponding decode callback!");
	return decode_callbacks[type](instruction, in_fstream);
}

int main(int argc, char** argv)
{
	constexpr const char* input_filename = "listing_0039_more_movs";
	constexpr const char* output_filename = "code.asm";
	std::ifstream in_fstream{ input_filename, std::ios::binary};
	std::ofstream out_fstream{ output_filename };
	if (!in_fstream)
	{
		std::cerr << "Error: Could not open file read: " << input_filename << std::endl;
		return 1;
	}

	if (!out_fstream)
	{
		std::cerr << "Error: Could not open file for write: " << output_filename << std::endl;
		return 1;
	}

	// Header for assembly
	out_fstream << "bits 16\n";

	// Maximum instruction byte size is 6
	uint8_t instruction_buffer[6];
	while(true)
	{
		// TODO: There is a problem of binary file ending, how to detect properly?
	    // Read first byte and deduce the type, then load needed bytes
		in_fstream.read(reinterpret_cast<char*>(&instruction_buffer), sizeof(uint8_t));
		if(in_fstream.eof())
		{
			break;
		}
		InstructionType type = DeduceOpcode(instruction_buffer[0]);
		// TODO: Debug ifdef this check
		if (type == NUMBER_OF_INSTRUCTION_TYPES)
		{
			std::cerr << "Error: Could not deduce instruction type, shutting down!" << std::endl;
			return 1;
		}
		out_fstream << DecodeBinary8086(&instruction_buffer[0], &in_fstream, type) << "\n";
	}
	in_fstream.close();
	out_fstream.close();

	return 0;

}

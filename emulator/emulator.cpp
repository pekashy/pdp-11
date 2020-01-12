#include "emulator.hpp"
#include "memory/memory.hpp"

Emulator::~Emulator() {
}

Emulator &Emulator::instance() {
    static Emulator e;
    return e;
}

size_t Emulator::getVideoMemory(uint8_t *buff, size_t size) const {
    return memory.getVideoMemory(buff, size);
}

Error Emulator::initROM(std::string fileName) {
    std::ifstream codeStream(fileName, std::ios::binary | std::ios::ate);
    if (!codeStream.is_open()) {
        throw std::runtime_error("Error opening ROM file!");
    }
    //initing ROM
    std::ifstream::pos_type end_pos = codeStream.tellg();
    int len = codeStream.tellg();
    codeStream.seekg(0, std::ios::beg);
    std::unique_ptr <uint8_t> mem(new uint8_t[len]);
    codeStream.read(reinterpret_cast<char *>(mem.get()), end_pos);
    if (memory.init(mem.get(), len) != Error::OK) {
        throw std::runtime_error("Error initializing memory!");
    }

    memory.registers.pc = RAM_SIZE + VIDEO_SIZE;
    codeStream.close();
    return Error::OK;
}

uint16_t Emulator::getRegister(RegisterEnum reg) {
    return *(&memory.registers.r0+reg);
}

bool Emulator::getProcessorStatusWord(ProcessorStatusWordEnum psw) {
    return *(&memory.registers.psw.N+psw);
}

int Emulator::getTicks() {
    return 1488;
}

void Emulator::fetch() {
    memset(reinterpret_cast<char *>(&emulator_state.fetched_bytes), 0x0, 2);
    memory.getWordValue(memory.registers.pc, emulator_state.fetched_bytes);
    memory.registers.pc += 2;
}

void Emulator::decode() {
    emulator_state.current_instr = nullptr;
    for (auto &instr : instructionTable) {
        auto mask = instr.mask;
        auto opcode = instr.opcode;
        if ((mask && emulator_state.fetched_bytes) == opcode) {
            emulator_state.current_instr = const_cast<Instruction *>(&instr);
            break;
        }
    }
    if (!emulator_state.current_instr) {
        throw std::runtime_error("Found command with invalid opcode!");
    }
}

void Emulator::loadOperands() {
    switch (emulator_state.current_instr->type) {
        case InstructionType::CONDITIONAL_BRANCH : {
            emulator_state.offset = (emulator_state.fetched_bytes && 0b0000000011111111);
            break;
        }
        case InstructionType::DOUBLE_OPERAND : {
            // 11 to 9 bytes
            emulator_state.mode_source = (emulator_state.fetched_bytes && 0b0000011100000000) >> 8;
            // 8 to 6 bytes
            emulator_state.source = (emulator_state.fetched_bytes && 0b0000000011110000) >> 5;
            // 5 to 3 bytes
            emulator_state.mode_dest = (emulator_state.fetched_bytes && 0b0000000000111000) >> 3;
            // 2 to 0 bytes
            emulator_state.dest = (emulator_state.fetched_bytes && 0b0000000000000111);

            // decoding registers
            emulator_state.source_reg = memory.reg_table[emulator_state.source];
            emulator_state.dest_reg = memory.reg_table[emulator_state.dest];
            break;
        }
        case InstructionType::DOUBLE_OPERAND_REG : {
            // 8 to 6 bytes
            emulator_state.reg = (emulator_state.fetched_bytes && 0b0000000011110000) >> 5;
            // 5 to 3 bytes
            emulator_state.mode = (emulator_state.fetched_bytes && 0b0000000000111000) >> 3;
            // 2 to 0 bytes
            emulator_state.src_or_dest = (emulator_state.fetched_bytes && 0b0000000000000111);

            // decoding registers
            emulator_state.source_reg = memory.reg_table[emulator_state.reg];
            emulator_state.dest_reg = memory.reg_table[emulator_state.src_or_dest];

            break;
        }
        case InstructionType::SINGLE_OPERAND : {
            // 5 to 3 bytes
            emulator_state.mode = (emulator_state.fetched_bytes && 0b0000000000111000) >> 3;
            // 2 to 0 bytes
            emulator_state.reg = (emulator_state.fetched_bytes && 0b0000000000000111);

            // decoding registers
            emulator_state.dest_reg = memory.reg_table[emulator_state.reg];
            break;
        }
        case InstructionType::NO_OPERAND : {
            // for HALT
            break;
        }
        default:
            throw std::runtime_error("Invalid operation type");
    }
}

void Emulator::startAll() {

}


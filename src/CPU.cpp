#include "CPU.h"
#include <iostream>
#include <CPUOpcodes.h>

namespace mysn
{
    CPU::CPU() : program_counter(0),
                 register_a(0),
                 register_x(0),
                 register_y(0),
                 status(0),
                 memory(0xFFFF, 0){};

    void CPU::run()
    {
        while (true)
        {
            // 操作码
            auto code = mem_read(program_counter);
            ++program_counter;
            auto program_counter_state = program_counter;
            auto opcode = CPUOpcodes::CPU_OPS_CODES_MAP.find(code);

            // 判断操作码是否存在
            if (opcode == CPUOpcodes::CPU_OPS_CODES_MAP.end())
            {
                return;
            }

            auto mnemonic = (opcode->second).mnemonic;
            auto mode = (opcode->second).mode;
            auto len = (opcode->second).len;

            switch (mnemonic)
            {
            case CPUOpcodeMnemonics::LDA:
            {
                lda(mode);
                break;
            }

            case CPUOpcodeMnemonics::STA:
            {
                sta(mode);
                break;
            }

            case CPUOpcodeMnemonics::TAX:
            {
                tax();
                break;
            }

            case CPUOpcodeMnemonics::INX:
            {
                inx();
                break;
            }

            case CPUOpcodeMnemonics::BRK:
            {
                return;
            }
            }

            if (program_counter_state == program_counter)
            {
                program_counter += (len - 1);
            }
        }
    }

    void CPU::lda(AddressingMode mode)
    {
        auto addr = get_operand_address(mode);
        auto value = mem_read(addr);

        register_a = value;
        update_zero_and_negative_flags(register_a);
    }

    void CPU::sta(AddressingMode mode)
    {
        auto addr = get_operand_address(mode);

        mem_write(addr, register_a);
    }

    void CPU::tax()
    {
        register_x = register_a;
        update_zero_and_negative_flags(register_x);
    }

    void CPU::inx()
    {
        ++register_x;
        update_zero_and_negative_flags(register_x);
    }

    void CPU::update_zero_and_negative_flags(Byte result)
    {
        if (result == 0)
        {
            status = status | 0b00000010;
        }
        else
        {
            status = status & 0b11111101;
        }

        if ((result & 0b10000000) != 0)
        {
            status = status | 0b10000000;
        }
        else
        {
            status = status & 0b01111111;
        }
    };

    Byte CPU::get_register_a()
    {
        return register_a;
    };

    Byte CPU::get_register_x()
    {
        return register_x;
    };

    Byte CPU::get_status()
    {
        return status;
    };

    Byte CPU::mem_read(Address addr)
    {
        return memory[addr];
    }

    void CPU::mem_write(Address addr, Byte data)
    {
        memory[addr] = data;
    }

    DobuleByte CPU::mem_read_u16(Address addr)
    {
        return mem_read(addr) | mem_read(addr + 1) << 8;
    }

    void CPU::mem_write_u16(Address addr, DobuleByte data)
    {
        Byte low = Byte(data & 0xff);
        Byte high = Byte(data >> 8);

        mem_write(addr, low);
        mem_write(addr + 1, high);
    }

    void CPU::load(std::vector<Byte> &program)
    {
        Address start = 0x8000;

        // 程序数据（Program ROM/PRG ROM），存储在插入的墨盒中（Cartridges），存储的是游戏的代码
        // 从内存地址的 0x8000 开始装载
        // TODO: 暂时不考虑溢出情况
        for (auto i : program)
        {
            memory[start] = i;
            ++start;
        }

        mem_write_u16(0xFFFC, 0x8000);
    }

    void CPU::reset()
    {
        register_a = 0;
        register_x = 0;
        register_y = 0;
        status = 0;

        program_counter = mem_read_u16(0xFFFC);
    }

    void CPU::load_and_run(std::vector<Byte> &program)
    {
        load(program);
        reset();
        run();
    }

    Address CPU::get_operand_address(AddressingMode mode)
    {
        switch (mode)
        {
        case AddressingMode::Immediate:
        {
            return program_counter;
        }

        case AddressingMode::ZeroPage:
        {

            return Address(mem_read(program_counter));
        }

        case AddressingMode::Absolute:
        {

            return mem_read_u16(program_counter);
        }

        case AddressingMode::ZeroPage_X:
        {
            auto pos = mem_read(program_counter);
            Address addr = Address(pos) + Address(register_x);

            return addr;
        }

        case AddressingMode::ZeroPage_Y:
        {
            auto pos = mem_read(program_counter);
            Address addr = Address(pos) + Address(register_y);

            return addr;

            break;
        }

        case AddressingMode::Absolute_X:
        {
            auto base = mem_read_u16(program_counter);
            Address addr = base + register_x;

            return addr;
        }

        case AddressingMode::Absolute_Y:
        {
            auto base = mem_read_u16(program_counter);
            Address addr = base + register_y;

            return addr;
        }

        case AddressingMode::Indirect_X:
        {
            auto base = mem_read(program_counter);
            auto ptr = Byte(base + register_x);
            DobuleByte lo = DobuleByte(mem_read(ptr));
            DobuleByte hi = DobuleByte(Byte(ptr + 1));

            return lo | hi << 8;
        }

        case AddressingMode::Indirect_Y:
        {
            auto base = mem_read(program_counter);
            auto lo = mem_read(base);
            auto hi = mem_read(base + 1);
            auto deref_base = lo | hi << 8;
            auto deref = register_y + deref_base;

            return deref;
        }

        case AddressingMode::NoneAddressing:
        {
            std::cout << mode << "is not supported" << std::endl;
            abort();
        }
        }
    };

}
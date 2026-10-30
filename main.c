#include <stdio.h>
#include <stdint.h>

#define FL_POS 1
#define FL_ZRO 2
#define FL_NEG 4

int16_t sign_extend(uint16_t value, uint16_t bit_count)
{
    uint16_t sign_bit = (uint16_t)(1u << (bit_count - 1));
    uint16_t mask = (uint16_t)((1u << bit_count) - 1u);
    int32_t result;

    value &= mask;

    if (value & sign_bit)
    {
        result = (int32_t)value - (int32_t)(1u << bit_count);
    }
    else
    {
        result = (int32_t)value;
    }

    return (int16_t)result;
}

void update_flags(uint16_t registers[],
                  uint16_t r,
                  uint16_t *condition_flag)
{
    uint16_t value = registers[r];

    if (value == 0)
    {
        *condition_flag = FL_ZRO;
    }
    else if (value & 0x8000)
    {
        *condition_flag = FL_NEG;
    }
    else
    {
        *condition_flag = FL_POS;
    }
}

void execute_ADD(uint16_t instruction,
                 uint16_t registers[],
                 uint16_t *condition_flag)
{
    uint16_t dr = (instruction >> 9) & 0x7;
    uint16_t sr1 = (instruction >> 6) & 0x7;
    uint16_t bit5 = (instruction >> 5) & 0x1;

    if (bit5 == 1)
    {
        int16_t imm5 = sign_extend(instruction & 0x1F, 5);
        registers[dr] = (uint16_t)(registers[sr1] + imm5);
    }
    else
    {
        uint16_t sr2 = instruction & 0x7;
        registers[dr] = (uint16_t)(registers[sr1] + registers[sr2]);
    }

    update_flags(registers, dr, condition_flag);
}

void execute_AND(uint16_t instruction,
                 uint16_t registers[],
                 uint16_t *condition_flag)
{
    uint16_t dr = (instruction >> 9) & 0x7;
    uint16_t sr1 = (instruction >> 6) & 0x7;
    uint16_t bit5 = (instruction >> 5) & 0x1;

    if (bit5 == 1)
    {
        int16_t imm5 = sign_extend(instruction & 0x1F, 5);
        registers[dr] = (uint16_t)(registers[sr1] & (uint16_t)imm5);
    }
    else
    {
        uint16_t sr2 = instruction & 0x7;
        registers[dr] = registers[sr1] & registers[sr2];
    }

    update_flags(registers, dr, condition_flag);
}

void execute_NOT(uint16_t instruction,
                 uint16_t registers[],
                 uint16_t *condition_flag)
{
    uint16_t dr = (instruction >> 9) & 0x7;
    uint16_t sr = (instruction >> 6) & 0x7;

    registers[dr] = (uint16_t)~registers[sr];

    update_flags(registers, dr, condition_flag);
}

void execute_LD(uint16_t instruction,
                uint16_t memory[],
                uint16_t pc,
                uint16_t registers[],
                uint16_t *condition_flag)
{
    uint16_t dr = (instruction >> 9) & 0x7;
    int16_t pc_offset9 = sign_extend(instruction & 0x1FF, 9);
    uint16_t address = (uint16_t)(pc + pc_offset9);

    registers[dr] = memory[address];
    update_flags(registers, dr, condition_flag);
}

void execute_ST(uint16_t instruction,
                uint16_t memory[],
                uint16_t pc,
                uint16_t registers[])
{
    uint16_t sr = (instruction >> 9) & 0x7;
    int16_t pc_offset9 = sign_extend(instruction & 0x1FF, 9);
    uint16_t address = (uint16_t)(pc + pc_offset9);

    memory[address] = registers[sr];
}

void execute_LDR(uint16_t instruction,
                 uint16_t memory[],
                 uint16_t registers[],
                 uint16_t *condition_flag)
{
    uint16_t dr = (instruction >> 9) & 0x7;
    uint16_t base_r = (instruction >> 6) & 0x7;
    int16_t offset6 = sign_extend(instruction & 0x3F, 6);
    uint16_t address = (uint16_t)(registers[base_r] + offset6);

    registers[dr] = memory[address];
    update_flags(registers, dr, condition_flag);
}

void execute_STR(uint16_t instruction,
                 uint16_t memory[],
                 uint16_t registers[])
{
    uint16_t sr = (instruction >> 9) & 0x7;
    uint16_t base_r = (instruction >> 6) & 0x7;
    int16_t offset6 = sign_extend(instruction & 0x3F, 6);
    uint16_t address = (uint16_t)(registers[base_r] + offset6);

    memory[address] = registers[sr];
}

void execute_LDI(uint16_t instruction,
                 uint16_t memory[],
                 uint16_t pc,
                 uint16_t registers[],
                 uint16_t *condition_flag)
{
    uint16_t dr = (instruction >> 9) & 0x7;
    int16_t pc_offset9 = sign_extend(instruction & 0x1FF, 9);
    uint16_t address = (uint16_t)(pc + pc_offset9);

    address = memory[address];
    registers[dr] = memory[address];

    update_flags(registers, dr, condition_flag);
}

void execute_STI(uint16_t instruction,
                 uint16_t memory[],
                 uint16_t pc,
                 uint16_t registers[])
{
    uint16_t sr = (instruction >> 9) & 0x7;
    int16_t pc_offset9 = sign_extend(instruction & 0x1FF, 9);
    uint16_t address = (uint16_t)(pc + pc_offset9);

    address = memory[address];
    memory[address] = registers[sr];
}

void execute_LEA(uint16_t instruction,
                 uint16_t pc,
                 uint16_t registers[],
                 uint16_t *condition_flag)
{
    uint16_t dr = (instruction >> 9) & 0x7;
    int16_t pc_offset9 = sign_extend(instruction & 0x1FF, 9);

    registers[dr] = (uint16_t)(pc + pc_offset9);

    update_flags(registers, dr, condition_flag);
}

void execute_BR(uint16_t instruction,
                uint16_t *pc,
                uint16_t condition_flag)
{
    uint16_t n = (instruction >> 11) & 0x1;
    uint16_t z = (instruction >> 10) & 0x1;
    uint16_t p = (instruction >> 9) & 0x1;
    int16_t pc_offset9 = sign_extend(instruction & 0x1FF, 9);

    if ((n && condition_flag == FL_NEG) ||
        (z && condition_flag == FL_ZRO) ||
        (p && condition_flag == FL_POS))
    {
        *pc = (uint16_t)(*pc + pc_offset9);
    }
}

void execute_TRAP(uint16_t instruction,
                  uint16_t memory[],
                  uint16_t registers[],
                  uint16_t *pc,
                  int *running)
{
    uint16_t trap_vector = instruction & 0xFF;

    registers[7] = *pc;

    switch (trap_vector)
    {
        case 0x20:
        {
            int c = getchar();
            registers[0] = (c == EOF) ? 0 : (uint16_t)(unsigned char)c;
            break;
        }

        case 0x21:
            putchar((char)(registers[0] & 0xFF));
            break;

        case 0x22:
        {
            uint16_t address = registers[0];
            uint32_t count;

            for (count = 0; count < 65536 && memory[address] != 0; count++)
            {
                putchar((char)(memory[address] & 0xFF));
                address++;
            }

            break;
        }

        case 0x23:
        {
            printf("Enter a character: ");

            int c = getchar();

            if (c != '\n' && c != EOF)
            {
                int next;
                while ((next = getchar()) != '\n' && next != EOF)
                {
                }
            }

            registers[0] = (c == EOF) ? 0 : (uint16_t)(unsigned char)c;

            if (c != EOF)
            {
                putchar(c);
            }

            break;
        }

        case 0x24:
        {
            uint16_t address = registers[0];
            uint32_t count;

            for (count = 0; count < 65536 && memory[address] != 0; count++)
            {
                uint16_t value = memory[address];
                char char1 = (char)(value & 0xFF);
                char char2 = (char)((value >> 8) & 0xFF);

                if (char1 != '\0')
                {
                    putchar(char1);
                }

                if (char2 != '\0')
                {
                    putchar(char2);
                }

                address++;
            }

            break;
        }

        case 0x25:
            printf("HALT\n");
            *running = 0;
            break;

        default:
            printf("Unknown TRAP vector: x%02X\n", trap_vector);
            *running = 0;
            break;
    }
}

void execute_JSR(uint16_t instruction,
                 uint16_t registers[],
                 uint16_t *pc)
{
    uint16_t bit11 = (instruction >> 11) & 0x1;

    registers[7] = *pc;

    if (bit11 == 1)
    {
        int16_t pc_offset11 = sign_extend(instruction & 0x7FF, 11);
        *pc = (uint16_t)(*pc + pc_offset11);
    }
    else
    {
        uint16_t base_r = (instruction >> 6) & 0x7;
        *pc = registers[base_r];
    }
}

void execute_JMP(uint16_t instruction,
                 uint16_t registers[],
                 uint16_t *pc)
{
    uint16_t base_r = (instruction >> 6) & 0x7;

    *pc = registers[base_r];
}

int main()
{
    uint16_t memory[65536] = {0};
    uint16_t registers[8] = {0};
    uint16_t pc = 0x3000;
    uint16_t condition_flag = FL_POS;

    registers[1] = 1;

    memory[0x3000] = 0x4801;
    memory[0x3001] = 0xF025;
    memory[0x3002] = 0x127F;
    memory[0x3003] = 0xC1C0;

    int running = 1;

    while (running)
    {
        uint16_t instruction = memory[pc];

        pc++;

        uint16_t opcode = instruction >> 12;

        switch (opcode)
        {
            case 0:
                execute_BR(
                    instruction,
                    &pc,
                    condition_flag
                );
                break;

            case 1:
                execute_ADD(
                    instruction,
                    registers,
                    &condition_flag
                );
                break;

            case 2:
                execute_LD(
                    instruction,
                    memory,
                    pc,
                    registers,
                    &condition_flag
                );
                break;

            case 3:
                execute_ST(
                    instruction,
                    memory,
                    pc,
                    registers
                );
                break;

            case 4:
                execute_JSR(
                    instruction,
                    registers,
                    &pc
                );
                break;

            case 5:
                execute_AND(
                    instruction,
                    registers,
                    &condition_flag
                );
                break;

            case 6:
                execute_LDR(
                    instruction,
                    memory,
                    registers,
                    &condition_flag
                );
                break;

            case 7:
                execute_STR(
                    instruction,
                    memory,
                    registers
                );
                break;

            case 8:
                printf("RTI not supported\n");
                running = 0;
                break;

            case 9:
                execute_NOT(
                    instruction,
                    registers,
                    &condition_flag
                );
                break;

            case 10:
                execute_LDI(
                    instruction,
                    memory,
                    pc,
                    registers,
                    &condition_flag
                );
                break;

            case 11:
                execute_STI(
                    instruction,
                    memory,
                    pc,
                    registers
                );
                break;

            case 12:
                execute_JMP(
                    instruction,
                    registers,
                    &pc
                );
                break;

            case 13:
                printf("Reserved opcode\n");
                running = 0;
                break;

            case 14:
                execute_LEA(
                    instruction,
                    pc,
                    registers,
                    &condition_flag
                );
                break;

            case 15:
                execute_TRAP(
                    instruction,
                    memory,
                    registers,
                    &pc,
                    &running
                );
                break;

            default:
                printf(
                    "Unknown opcode: %X\n",
                    opcode
                );

                running = 0;
                break;
        }
    }

    printf("R1 = %d\n", registers[1]);
    printf("R5 = %d\n", registers[5]);

    return 0;
}

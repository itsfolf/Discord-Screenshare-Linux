#include <unistd.h>
#include <algorithm>
#include <sys/mman.h>

#include "rtc_base/logging.h"
#include "Hook.h"

namespace LinuxFix
{

    namespace
    {
        using namespace std;
        const uint64_t PAGE_SIZE = getpagesize();

        void *AllocatePageNearAddress(void *targetAddr)
        {
            uint64_t startAddr = (uint64_t(targetAddr) & ~(PAGE_SIZE - 1)); // round down to nearest page boundary
            uint64_t minAddr = min(startAddr - 0x7FFFFF00, (uint64_t)PAGE_SIZE);
            uint64_t maxAddr = max(startAddr + 0x7FFFFF00, (uint64_t)(128ull * 1024 * 1024 * 1024 * 1024));

            uint64_t startPage = (startAddr - (startAddr % PAGE_SIZE));

            uint64_t pageOffset = 1;
            while (1)
            {
                uint64_t byteOffset = pageOffset * PAGE_SIZE;
                uint64_t highAddr = startPage + byteOffset;
                uint64_t lowAddr = (startPage > byteOffset) ? startPage - byteOffset : 0;

                bool needsExit = highAddr > maxAddr && lowAddr < minAddr;
                if (highAddr < maxAddr)
                {
                    void *outAddr = mmap((void *)highAddr, PAGE_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                    if (outAddr != nullptr && outAddr != (void *)-1)
                        return outAddr;
                    else
                        RTC_LOG(LS_ERROR) << "mmap failed: " << strerror(errno);
                }

                if (lowAddr > minAddr)
                {
                    void *outAddr = mmap((void *)lowAddr, PAGE_SIZE, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                    if (outAddr != nullptr && outAddr != (void *)-1)
                        return outAddr;
                    else
                        RTC_LOG(LS_ERROR) << "mmap failed: " << strerror(errno);
                }

                pageOffset++;

                if (needsExit)
                {
                    break;
                }
            }

            return nullptr;
        }

        void WriteAbsoluteJump64(void *absJumpMemory, void *addrToJumpTo)
        {
            uint8_t absJumpInstructions[] = {0x49, 0xBA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, // mov 64 bit value into r10
                                             0x41, 0xFF, 0xE2};                                          // jmp r10

            uint64_t addrToJumpTo64 = (uint64_t)addrToJumpTo;
            memcpy(&absJumpInstructions[2], &addrToJumpTo64, sizeof(addrToJumpTo64));
            memcpy(absJumpMemory, absJumpInstructions, sizeof(absJumpInstructions));
        }
    }

    int CreateHook(void *targetFn, void *hookFn)
    {
        void *relayFuncMemory = AllocatePageNearAddress(targetFn);
        WriteAbsoluteJump64(relayFuncMemory, hookFn);

        const uint64_t PAGE_SIZE = getpagesize();
        uint64_t addr = (uint64_t(targetFn) & ~(PAGE_SIZE - 1));
        if (mprotect((void *)addr, 1024, PROT_READ | PROT_WRITE | PROT_EXEC) == -1)
        {
            RTC_LOG(LS_ERROR) << (void *)addr << " mprotect failed: " << strerror(errno);
            return -1;
        }

        // 32 bit relative jump opcode is E9, takes 1 32 bit operand for jump offset
        uint8_t jmpInstruction[5] = {0xE9, 0x0, 0x0, 0x0, 0x0};

        // to fill out the last 4 bytes of jmpInstruction, we need the offset between
        // the relay function and the instruction immediately AFTER the jmp instruction
        const uint64_t relAddr = (uint64_t)relayFuncMemory - ((uint64_t)targetFn + sizeof(jmpInstruction));
        memcpy(jmpInstruction + 1, &relAddr, 4);

        // install the hook
        memcpy(targetFn, jmpInstruction, sizeof(jmpInstruction));
        return 0;
    }
}
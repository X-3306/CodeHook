#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>

// The original function to be hooked
void originalFunction()
{
    std::cout << "Original function" << std::endl;
}

// Our hooked function
void hookedFunction()
{
    std::cout << "Hooked function" << std::endl;
    // Call the original function
    originalFunction();
}

// Function to change memory page protection
bool changePageProtection(void* addr, size_t size, int prot)
{
    size_t pageSize = sysconf(_SC_PAGESIZE);
    uintptr_t start = reinterpret_cast<uintptr_t>(addr) & ~(pageSize - 1);
    int result = mprotect(reinterpret_cast<void*>(start), size, prot);
    return result == 0;
}

int main()
{
    // Memory page size
    size_t pageSize = sysconf(_SC_PAGESIZE);

    // Determine the memory size we need to allocate
    size_t memorySize = 2 * pageSize;

    // Allocate memory
    void* memory = mmap(nullptr, memorySize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        std::cerr << "Error allocating memory" << std::endl;
        return 1;
    }

    // Copy the original function's code to memory
    std::memcpy(memory, reinterpret_cast<void*>(originalFunction), memorySize);

    // Change memory page protection to write mode and perform modification
    if (!changePageProtection(memory, memorySize, PROT_WRITE)) {
        std::cerr << "Error changing memory page protection" << std::endl;
        return 1;
    }

    // Replace the call to the hook function with a call to the original function
    uintptr_t hookedFunctionAddress = reinterpret_cast<uintptr_t>(hookedFunction);
    uintptr_t originalFunctionAddress = reinterpret_cast<uintptr_t>(memory);
    uintptr_t callInstruction = 0xe8; // CALL x instruction code
    intptr_t offset = hookedFunctionAddress - (originalFunctionAddress + 5); // +5 to skip the CALL instruction code

    *reinterpret_cast<uint8_t*>(memory) = callInstruction;
    *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(memory) + 1) = static_cast<int32_t>(offset);

    // Restore memory page protection to read-only and execute the hooked function
    if (!changePageProtection(memory, memorySize, PROT_READ)) {
        std::cerr << "Error changing memory page protection" << std::endl;
        return 1;
    }

    // Call the hooked function
    reinterpret_cast<void (*)()>(memory)();

    // Free memory
    if (munmap(memory, memorySize) == -1) {
        std::cerr << "Error freeing memory" << std::endl;
        return 1;
    }

    return 0;
}

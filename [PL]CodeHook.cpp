#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>

// Właściwa funkcja, którą zamierzamy zhakować
void originalFunction()
{
    std::cout << "Oryginalna funkcja" << std::endl;
}

// Nasza funkcja hakująca
void hookedFunction()
{
    std::cout << "Funkcja hakująca" << std::endl;
    // Wywołanie oryginalnej funkcji
    originalFunction();
}

// Funkcja do zmiany ochrony strony pamięci
bool changePageProtection(void* addr, size_t size, int prot)
{
    size_t pageSize = sysconf(_SC_PAGESIZE);
    uintptr_t start = reinterpret_cast<uintptr_t>(addr) & ~(pageSize - 1);
    int result = mprotect(reinterpret_cast<void*>(start), size, prot);
    return result == 0;
}

int main()
{
    // Rozmiar strony pamięci
    size_t pageSize = sysconf(_SC_PAGESIZE);

    // Określ rozmiar pamięci, który musimy zarezerwować
    size_t memorySize = 2 * pageSize;

    // Alokuj pamięć
    void* memory = mmap(nullptr, memorySize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        std::cerr << "Błąd przy alokacji pamięci" << std::endl;
        return 1;
    }

    // Skopiuj kod funkcji oryginalnej do pamięci
    std::memcpy(memory, reinterpret_cast<void*>(originalFunction), memorySize);

    // Zmień ochronę strony pamięci na tryb zapisu i wykonaj modyfikację
    if (!changePageProtection(memory, memorySize, PROT_WRITE)) {
        std::cerr << "Błąd przy zmianie ochrony strony pamięci" << std::endl;
        return 1;
    }

    // Zastąp wywołanie funkcji haka wywołaniem funkcji oryginalnej
    uintptr_t hookedFunctionAddress = reinterpret_cast<uintptr_t>(hookedFunction);
    uintptr_t originalFunctionAddress = reinterpret_cast<uintptr_t>(memory);
    uintptr_t callInstruction = 0xe8; // Kod instrukcji CALL x
    intptr_t offset = hookedFunctionAddress - (originalFunctionAddress + 5); // +5, aby ominąć kod instrukcji CALL

    *reinterpret_cast<uint8_t*>(memory) = callInstruction;
    *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(memory) + 1) = static_cast<int32_t>(offset);

    // Przywróć ochronę strony pamięci na tryb tylko do odczytu i wykonaj funkcję hakującą
    if (!changePageProtection(memory, memorySize, PROT_READ)) {
        std::cerr << "Błąd przy zmianie ochrony strony pamięci" << std::endl;
        return 1;
    }

    // Wywołaj funkcję hakującą
    reinterpret_cast<void (*)()>(memory)();

    // Zwolnij pamięć
    if (munmap(memory, memorySize) == -1) {
        std::cerr << "Błąd przy zwalnianiu pamięci" << std::endl;
        return 1;
    }

    return 0;
}

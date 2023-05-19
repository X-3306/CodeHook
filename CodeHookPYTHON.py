import ctypes
import mmap
import sys

# The original function to be hooked
def original_function():
    print("Original function")

# Our hooked function
def hooked_function():
    print("Hooked function")
    # Call the original function
    original_function()

# Function to change memory protection
def change_page_protection(addr, size, prot):
    pagesize = mmap.PAGESIZE
    start = addr & ~(pagesize - 1)
    return mmap.mprotect(start, size, prot) == 0

def main():
    # Determine the memory size we need to allocate
    pagesize = mmap.PAGESIZE
    memory_size = 2 * pagesize

    # Allocate memory
    memory = mmap.mmap(-1, memory_size, mmap.MAP_PRIVATE | mmap.MAP_ANONYMOUS)
    if memory.fileno() == -1:
        print("Error allocating memory")
        return 1

    # Copy the original function's code to memory
    original_function_addr = ctypes.cast(original_function, ctypes.c_void_p).value
    memory.write(ctypes.string_at(original_function_addr, memory_size))

    # Change memory protection to write mode and perform modification
    if not change_page_protection(memory_addr, memory_size, mmap.PROT_WRITE):
        print("Error changing memory protection")
        return 1

    # Replace the call to the hook function with a call to the original function
    hooked_function_addr = ctypes.cast(hooked_function, ctypes.c_void_p).value
    call_instruction = b'\xe8'  # CALL x instruction code
    offset = hooked_function_addr - (original_function_addr + 5)  # +5 to skip the CALL instruction code

    memory[0] = call_instruction
    memory[1:5] = offset.to_bytes(4, sys.byteorder, signed=True)

    # Restore memory protection to read-only and execute the hooked function
    if not change_page_protection(memory_addr, memory_size, mmap.PROT_READ):
        print("Error changing memory protection")
        return 1

    # Call the hooked function
    ctypes.CFUNCTYPE(None)(memory_addr)()

    # Free memory
    memory.close()

    return 0

if __name__ == "__main__":
    main()

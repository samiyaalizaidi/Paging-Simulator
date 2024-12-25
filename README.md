# Paging Simulator 

This repository contains a C-based paging simulator designed to implement and demonstrate the paging memory management technique. The simulator dynamically adapts to various configurations of logical and physical memory, page sizes, and degrees of multiprogramming.

## Features

- Simulates paging in memory management for configurable physical and logical memory sizes.
- Supports dynamic page sizes and degrees of multiprogramming.
- Parses binary files to load process code and data segments.
- Implements page table management for each process.
- Handles memory allocation, fragmentation, and free frame management.
- Provides detailed output, including memory dumps, free frame lists, and internal fragmentation.

## Usage

### Prerequisites
Before running the simulator, ensure the following dependencies are installed:

- **GCC compiler**: To compile the simulator and utilities.
- **Make**: For building the project using the Makefile.

### Cloning the Repository
Start by cloning the repository to your local machine:

```bash
git clone https://github.com/samiyaalizaidi/Paging-Simulator.git
cd Paging-Simulator
```

### Compilation

To compile the simulator, use the provided `Makefile`:

```bash
make
```

### Running the Simulator
The simulator accepts the following command-line arguments:
```bash
./paging <physical memory size in B> <logical address size in bits> <page size in B> <path to process 1 file> <path to process 2 file> ... <path to process n file>
```

**Example**:
```bash
./paging 1048576 12 1024 "p1.proc" "./processes/p2.proc" "p3.proc"
```

This example runs the simulator with:

- 1 MB of physical memory.
- 12-bit logical address size.
- 1 KB page size.
- Processes loaded from ``p1.proc``, ``./processes/p2.proc``, and ``p3.proc``.

## Repository Structure
```
.
├── README.md
├── LICENSE
├── Makefile              
├── paging.c             
└── process_generator.c 
```

## Input Process
### File Format
Process files should follow this binary structure:
1. Process ID (1 byte)
2. Code Segment Size (2 bytes)
3. Code Segment (variable size based on the code segment size)
4. Data Segment Size (2 bytes following the code segment)
5. Data Segment (variable size based on the data segment size)
6. End of Process (1 byte, ``0xFF``)

### Generating Process Files
You can use the ``process_generator.c`` file in this repository to generate valid process files conforming to the specified format.

Compile the process generator:
```bash
gcc -o process_generator process_generator.c
```

Run it to create a process file:
```bash
./process_generator
```

## Outputs
- **Memory Dump**: Displays the mapping of logical pages to physical frames for each process.
- **Free Frame List**: Shows available frames in memory.
- **Internal Fragmentation**: Reports total unused space in memory allocations.
  
## Error Handling
- Ensures binary files conform to the specified format.
- Gracefully handles malformed inputs or missing end-of-process markers (``0xFF``).

## Contributions
All code in this repository was written by [Samiya Ali Zaidi](https://github.com/samiyaalizaidi).

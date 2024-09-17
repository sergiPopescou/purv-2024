#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <elf.h>

/* 'magic number' - identifying file as an ELF object file */
const char EIMAG[4] = { 0x7f, 'E', 'L', 'F' };

const char EICLASS[3][50] = {"Invalid class", "32bit objects", "64bit objects"};

const char EIDATA[3][50] = {"Invalid data encoding","Little Endian","Big Endian"};

const char EIVERSION[2][50] = {"Invalid version", "Current version"};

const char EIOSABI[14][50] = {
    "No extensions or unspecified", 
    "Hewlett-Packard HP-UX", 
    "NetBSD", 
    "Object uses GNU ELF extensions", 
    "Sun Solaris", 
    "IBM AIX",
    "SGI Irix", 
    "FreeBSD", 
    "Compaq TRU64 UNIX", 
    "Novell Modesto", 
    "OpenBSD", 
    "ARM EABI", 
    "ARM", 
    "Standalone (embedded) application "
};

/* This member identifies the object file type */					
const char ETYPE[5][50] = {"No file type", "Relocatable file", "Executable file", "Shared object file", "Core file"};

/* This member's value specifies the required architecture for an individual file */
const char EMACHINE[101][100] = {
    "No machine",                          // 0 - EM_NONE
    "AT&T WE 32100",                       // 1 - EM_M32
    "SPARC",                               // 2 - EM_SPARC
    "Intel 80386",                         // 3 - EM_386
    "Motorola 68000",                      // 4 - EM_68K
    "Motorola 88000",                      // 5 - EM_88K
    "Reserved for future use (was EM_486)",// 6 - reserved
    "Intel 80860",                         // 7 - EM_860
    "MIPS I Architecture",                 // 8 - EM_MIPS
    "IBM System/370 Processor",            // 9 - EM_S370
    "MIPS RS3000 Little-endian",           // 10 - EM_MIPS_RS3_LE
    "Reserved for future use",             // 11-14 - reserved
    "Hewlett-Packard PA-RISC",             // 15 - EM_PARISC
    "Reserved for future use",             // 16 - reserved
    "Fujitsu VPP500",                      // 17 - EM_VPP500
    "Enhanced instruction set SPARC",      // 18 - EM_SPARC32PLUS
    "Intel 80960",                         // 19 - EM_960
    "PowerPC",                             // 20 - EM_PPC
    "64-bit PowerPC",                      // 21 - EM_PPC64
    "IBM System/390 Processor",            // 22 - EM_S390
    "Reserved for future use",             // 23-35 - reserved
    "NEC V800",                            // 36 - EM_V800
    "Fujitsu FR20",                        // 37 - EM_FR20
    "TRW RH-32",                           // 38 - EM_RH32
    "Motorola RCE",                        // 39 - EM_RCE
    "Advanced RISC Machines ARM",          // 40 - EM_ARM
    "Digital Alpha",                       // 41 - EM_ALPHA
    "Hitachi SH",                          // 42 - EM_SH
    "SPARC Version 9",                     // 43 - EM_SPARCV9
    "Siemens TriCore embedded processor",  // 44 - EM_TRICORE
    "Argonaut RISC Core",                  // 45 - EM_ARC
    "Hitachi H8/300",                      // 46 - EM_H8_300
    "Hitachi H8/300H",                     // 47 - EM_H8_300H
    "Hitachi H8S",                         // 48 - EM_H8S
    "Hitachi H8/500",                      // 49 - EM_H8_500
    "Intel IA-64 processor architecture",  // 50 - EM_IA_64
    "Stanford MIPS-X",                     // 51 - EM_MIPS_X
    "Motorola ColdFire",                   // 52 - EM_COLDFIRE
    "Motorola M68HC12",                    // 53 - EM_68HC12
    "Fujitsu MMA Multimedia Accelerator",  // 54 - EM_MMA
    "Siemens PCP",                         // 55 - EM_PCP
    "Sony nCPU embedded RISC processor",   // 56 - EM_NCPU
    "Denso NDR1 microprocessor",           // 57 - EM_NDR1
    "Motorola Star*Core processor",        // 58 - EM_STARCORE
    "Toyota ME16 processor",               // 59 - EM_ME16
    "STMicroelectronics ST100 processor",  // 60 - EM_ST100
    "Advanced Logic Corp. TinyJ",          // 61 - EM_TINYJ
    "AMD x86-64 architecture",             // 62 - EM_X86_64
    "Sony DSP Processor",                  // 63 - EM_PDSP
    "Digital Equipment Corp. PDP-10",      // 64 - EM_PDP10
    "Digital Equipment Corp. PDP-11",      // 65 - EM_PDP11
    "Siemens FX66 microcontroller",        // 66 - EM_FX66
    "STMicroelectronics ST9+",             // 67 - EM_ST9PLUS
    "STMicroelectronics ST7",              // 68 - EM_ST7
    "Motorola MC68HC16 Microcontroller",   // 69 - EM_68HC16
    "Motorola MC68HC11 Microcontroller",   // 70 - EM_68HC11
    "Motorola MC68HC08 Microcontroller",   // 71 - EM_68HC08
    "Motorola MC68HC05 Microcontroller",   // 72 - EM_68HC05
    "Silicon Graphics SVx",                // 73 - EM_SVX
    "STMicroelectronics ST19",             // 74 - EM_ST19
    "Digital VAX",                         // 75 - EM_VAX
    "Axis Communications 32-bit processor",// 76 - EM_CRIS
    "Infineon Technologies 32-bit processor",// 77 - EM_JAVELIN
    "Element 14 64-bit DSP Processor",     // 78 - EM_FIREPATH
    "LSI Logic 16-bit DSP Processor",      // 79 - EM_ZSP
    "Donald Knuth's 64-bit processor",     // 80 - EM_MMIX
    "Harvard University machine-independent",// 81 - EM_HUANY
    "SiTera Prism",                        // 82 - EM_PRISM
    "Atmel AVR 8-bit microcontroller",     // 83 - EM_AVR
    "Fujitsu FR30",                        // 84 - EM_FR30
    "Mitsubishi D10V",                     // 85 - EM_D10V
    "Mitsubishi D30V",                     // 86 - EM_D30V
    "NEC v850",                            // 87 - EM_V850
    "Mitsubishi M32R",                     // 88 - EM_M32R
    "Matsushita MN10300",                  // 89 - EM_MN10300
    "Matsushita MN10200",                  // 90 - EM_MN10200
    "picoJava",                            // 91 - EM_PJ
    "OpenRISC 32-bit embedded processor",  // 92 - EM_OPENRISC
    "ARC Cores Tangent-A5",                // 93 - EM_ARC_A5
    "Tensilica Xtensa Architecture",       // 94 - EM_XTENSA
    "Alphamosaic VideoCore processor",     // 95 - EM_VIDEOCORE
    "Thompson Multimedia GPP",             // 96 - EM_TMM_GPP
    "National Semiconductor 32000",        // 97 - EM_NS32K
    "Tenor Network TPC processor",         // 98 - EM_TPC
    "Trebia SNP 1000 processor",           // 99 - EM_SNP1K
    "STMicroelectronics ST200",            // 100 - EM_ST200
};


bool is_file_an_ELF(Elf64_Ehdr* elf_header);

int main(int argc, char *argv[]) {

    Elf64_Ehdr elf_header;
    
    /*Otvaranje fajla*/
    FILE* fp = fopen(argv[1], "r");
    if (fp == NULL) { 
        printf("Greska pri otvaranju fajla !\n");
        return -1;
    }

    /* Ucitavanje zaglavlja ELF fajla u promjenjivu tipa Elf64_Ehdr */
    fread(&elf_header, sizeof(Elf64_Ehdr), 1, fp);

    printf("Magic number: %d :", is_file_an_ELF(&elf_header));
    for(int i=0;i<4;i++)
        printf("%x-", elf_header.e_ident[i]);
    printf("\n");

    printf("Class: %s \n", EICLASS[elf_header.e_ident[EI_CLASS]]);

    printf("Data: %s \n", EIDATA[elf_header.e_ident[EI_DATA]]);
    
    printf("Version: %s \n", EIVERSION[elf_header.e_ident[EI_VERSION]]);
    
    printf("OSABI: %s \n", EIOSABI[elf_header.e_ident[EI_OSABI]]);
    
    printf("ABI Version: %d \n", elf_header.e_ident[EI_ABIVERSION]);
    
    printf("E_MACHINE: %s \n", EMACHINE[elf_header.e_machine]);
    
    printf("E_TYPE: %s \n", ETYPE[elf_header.e_type]);
    
    printf("E_ENTRY: 0x%lx \n",elf_header.e_entry);

    printf("Flags: 0x%x\n", elf_header.e_flags);

    printf("ELF header's size in bytes: %d \n", elf_header.e_ehsize);

    printf("Size in bytes of one entry in the file's program header table: %d \n", elf_header.e_phentsize);

    printf("number of entries in the program header table: %d\n", elf_header.e_phnum);

    printf("section header's size in bytes: %d \n", elf_header.e_shentsize);

    printf("number of entries in the section header table.: %d\n", elf_header.e_shnum);

    printf("section header table index of the entry associated with the section name string table: %d\n", elf_header.e_shstrndx);
    
    fclose(fp); 
    return 0;
}

bool is_file_an_ELF(Elf64_Ehdr *elf_header){
    int count = 0;
    for(int i=0;i<4;i++){
        if ((*elf_header).e_ident[i] == EIMAG[i])
            count++;
   }
   if (count == 4) 
        return true;
   else 
        return false;
}




























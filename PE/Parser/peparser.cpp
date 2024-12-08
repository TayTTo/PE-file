#include <iostream>
#include <windows.h>

using namespace std;

int main(int argc, char *argv[]) {
	HANDLE hFile = CreateFileA(argv[1], GENERIC_ALL, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	DWORD fileSize = GetFileSize(hFile, NULL);
	LPVOID fileData = HeapAlloc(GetProcessHeap(), 0, fileSize);

	ReadFile(hFile, fileData, fileSize, NULL, NULL);
	PIMAGE_DOS_HEADER dosHeader = (PIMAGE_DOS_HEADER)fileData;

	cout << "DOS HEADER" << endl;
	cout << "Magic number: 0x" << dosHeader->e_magic << endl;
	cout << "End of dos header: 0x" << dosHeader->e_lfanew << endl;

	cout << "-------------------" << endl;
	PIMAGE_NT_HEADERS ntHeader =
		(PIMAGE_NT_HEADERS)((DWORD)fileData + dosHeader->e_lfanew);
	cout << "NT HEADER" << endl;
	cout << "Signature: " << ntHeader->Signature << endl;
	cout << "-------------------" << endl;

	cout << "File header:" << endl;
	cout << "Machine: 0x" << ntHeader->FileHeader.Machine << endl;
	cout << "Number of sections: " << ntHeader->FileHeader.NumberOfSections
		<< endl;
	cout << "Time stamp: " << ntHeader->FileHeader.TimeDateStamp << endl;
	cout << "Pointer to Symbol TABLE: "
		<< ntHeader->FileHeader.PointerToSymbolTable << endl;
	cout << "Number of symbols: " << ntHeader->FileHeader.NumberOfSymbols << endl;
	cout << "Size of Optional Header: "
		<< ntHeader->FileHeader.SizeOfOptionalHeader << endl;
	cout << "Characteristics: " << ntHeader->FileHeader.Characteristics << endl;

	cout << "--------------------" << endl;
	cout << "Optional head: " << endl;

	IMAGE_OPTIONAL_HEADER optionalHeader = ntHeader->OptionalHeader;
	cout << "Magic: " << optionalHeader.Magic << endl;
	cout << "Size of code: " << optionalHeader.SizeOfCode << endl;
	cout << "Size of Initialized Data: " << optionalHeader.SizeOfInitializedData
		<< endl;
	cout << "Size of Uninitalized data: "
		<< optionalHeader.SizeOfUninitializedData << endl;
	cout << "Address of entry point: " << optionalHeader.AddressOfEntryPoint
		<< endl;
	cout << "Base of code: " << optionalHeader.BaseOfCode << endl;
	cout << "Image base: " << optionalHeader.ImageBase << endl;
	cout << "Section alignment: " << optionalHeader.SectionAlignment << endl;
	cout << "File alignment: " << optionalHeader.FileAlignment << endl;
	cout << "Major image version: " << optionalHeader.MajorImageVersion << endl;
	cout << "Minor image version: " << optionalHeader.MinorLinkerVersion << endl;
	cout << "Size of Image: " << optionalHeader.SizeOfImage << endl;
	cout << "Size of header: " << optionalHeader.SizeOfHeaders << endl;
	cout << "Checksum: " << optionalHeader.CheckSum << endl;

	cout << "-----------------" << endl;

	cout << "Section header" << endl;
	DWORD sectionLocation = (DWORD)ntHeader + sizeof(DWORD) +
		(DWORD)(sizeof(IMAGE_FILE_HEADER)) +
		(DWORD)ntHeader->FileHeader.SizeOfOptionalHeader;
	DWORD sectionSize = (DWORD)sizeof(IMAGE_SECTION_HEADER);
	DWORD importDirectoryRVA =
		optionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	PIMAGE_SECTION_HEADER sectionHeader;
	PIMAGE_SECTION_HEADER importSection;

	for (int i = 0; i < ntHeader->FileHeader.NumberOfSections; i++) {
		sectionHeader = (PIMAGE_SECTION_HEADER)sectionLocation;
		cout << "section header : " << sectionHeader->Name << endl;
		cout << "Virtual size: " << sectionHeader->Misc.VirtualSize << endl;
		cout << "Size of raw data: " << sectionHeader->VirtualAddress << endl;
		cout << "Pointer to rawdata: " << sectionHeader->SizeOfRawData << endl;
		cout << "Pointer to relocations: " << sectionHeader->PointerToRelocations
			<< endl;
		cout << "Pointer to line numbers: " << sectionHeader->PointerToLinenumbers
			<< endl;
		cout << "characteristics: " << sectionHeader->Characteristics << endl;
		cout << "-----------------" << endl;

		if (importDirectoryRVA >= sectionHeader->VirtualAddress &&
				importDirectoryRVA <
				sectionHeader->VirtualAddress + sectionHeader->Misc.VirtualSize) {
			importSection = sectionHeader;
		}
		sectionLocation += sectionSize;
	}
	PIMAGE_THUNK_DATA32 thunkData = nullptr;
	uintptr_t rawOffset =
		reinterpret_cast<uintptr_t>(fileData) + importSection->PointerToRawData;

	PIMAGE_IMPORT_DESCRIPTOR importDescriptor =
		reinterpret_cast<PIMAGE_IMPORT_DESCRIPTOR>(
				rawOffset +
				(ntHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT]
				 .VirtualAddress -
				 importSection->VirtualAddress));

	cout << "Import DLLs:" << endl;

	for (; importDescriptor->Name != 0; ++importDescriptor) {
		const char *dllName = reinterpret_cast<const char *>(
				rawOffset + (importDescriptor->Name - importSection->VirtualAddress));
		cout << "DLL: " << dllName << endl;

		DWORD thunk = importDescriptor->OriginalFirstThunk == 0
			? importDescriptor->FirstThunk
			: importDescriptor->OriginalFirstThunk;

		thunkData = reinterpret_cast<PIMAGE_THUNK_DATA32>(
				rawOffset + (thunk - importSection->VirtualAddress));

		for (; thunkData->u1.AddressOfData != 0; ++thunkData) {
			if (thunkData->u1.AddressOfData & IMAGE_ORDINAL_FLAG32) {
				DWORD ordinal = thunkData->u1.Ordinal & 0xFFFF; // 
				cout << "  Ordinal: " << ordinal << endl;
			} else {
				// Nhập khẩu theo tên
				PIMAGE_IMPORT_BY_NAME importByName =
					reinterpret_cast<PIMAGE_IMPORT_BY_NAME>(
							rawOffset +
							(thunkData->u1.AddressOfData - importSection->VirtualAddress));
				cout << "  Function: " << importByName->Name << endl;
			}
		}
		cout << "-------------" << endl;
	}

	return 0;
}

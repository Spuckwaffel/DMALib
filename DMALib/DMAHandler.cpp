#include "DMAHandler.h"

#include <chrono>
#include <iostream>
#include <leechcore.h>
#include <unordered_map>
#include <vmmdll.h>

void DMAHandler::log(const char* fmt, ...)
{
	//small check before processing
	if (strlen(fmt) > 2000) return;
	char logBuffer[2001] = { 0 };

	//time related
	const auto now_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm time_info;
	localtime_s(&time_info, &now_time_t);
	// Format time as a string
	std::ostringstream oss;
	oss << std::put_time(&time_info, "%H:%M:%S");
	va_list args;
	va_start(args, fmt);
	vsprintf_s(logBuffer, 2000, fmt, args);
	printf("[DMA @ %s]: %s\n", oss.str().c_str(), logBuffer);
}

void DMAHandler::assertNoInit() const
{
	if (!DMA_INITIALIZED || PROCESS_INITIALIZED)
	{
		log("DMA or process not inizialized!");
		throw new std::string("DMA not inizialized!");
	}

}

DMAHandler::DMAHandler(const wchar_t* wname)
{
	if (!DMA_INITIALIZED)
	{
		log("loading libraries...");
		modules.VMM = LoadLibraryA("vmm.dll");
		modules.FTD3XX = LoadLibraryA("FTD3XX.dll");
		modules.LEECHCORE = LoadLibraryA("leechcore.dll");

		if (!modules.VMM || !modules.FTD3XX || !modules.LEECHCORE)
		{
			log("ERROR: could not load a library:");
			log("vmm: %p\n", modules.VMM);
			log("ftd: %p\n", modules.FTD3XX);
			log("leech: %p\n", modules.LEECHCORE);
		}


		log("inizializing...");

		char arg1[] = "-v";
		char arg2[] = "-device";
		char arg3[] = "fpga";

		LPSTR argv[] = { arg1, arg2, arg3 };
		DWORD argc = sizeof(argv) / sizeof(LPSTR);

		if (!VMMDLL_Initialize(3, argv))
		{
			log("ERROR: Initialization failed! Is the DMA in use or disconnected?");
		}

		ULONG64 FPGA_ID, DEVICE_ID;

		VMMDLL_ConfigGet(LEECHCORE_OPT_FPGA_FPGA_ID, &FPGA_ID);
		VMMDLL_ConfigGet(LEECHCORE_OPT_FPGA_DEVICE_ID, &DEVICE_ID);

		log("FPGA ID: %llu", FPGA_ID);
		log("DEVICE ID: %llu", DEVICE_ID);
		log("success!");

		DMA_INITIALIZED = TRUE;
	}

	// Convert the wide string to a standard string because VMMDLL_PidGetFromName expects LPSTR.
	std::wstring ws(wname);
	const std::string str(ws.begin(), ws.end());

	processInfo.name = str;
	processInfo.wname = wname;
	if (!VMMDLL_PidGetFromName(const_cast<char*>(processInfo.name.c_str()), &processInfo.pid))
	{
		log("WARN: Process with name %s not found!", processInfo.name.c_str());
	}
	else
		PROCESS_INITIALIZED = TRUE;

}

bool DMAHandler::isInitialized() const
{
	return DMA_INITIALIZED && PROCESS_INITIALIZED;
}

DWORD DMAHandler::getPID() const
{
	assertNoInit();
	return processInfo.pid;
}

ULONG64 DMAHandler::getBaseAddress()
{
	if (!processInfo.base)
		processInfo.base = VMMDLL_ProcessGetModuleBase(processInfo.pid, const_cast<LPWSTR>(processInfo.wname));

	return processInfo.base;
}

void DMAHandler::read(const ULONG64 address, const ULONG64 buffer, const SIZE_T size) const
{
	assertNoInit();
	DWORD dwBytesRead = 0;

#if COUNT_TOTAL_READSIZE
	readSize += size;
#endif

	VMMDLL_MemReadEx(processInfo.pid, address, reinterpret_cast<PBYTE>(buffer), size, &dwBytesRead, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_ZEROPAD_ON_FAIL | VMMDLL_FLAG_NOPAGING_IO);

	if (dwBytesRead != size)
		log("Didnt read all bytes requested! Only read %llu/%llu bytes!", dwBytesRead, size);
}

bool DMAHandler::write(const ULONG64 address, const ULONG64 buffer, const SIZE_T size) const
{
	assertNoInit();
	return VMMDLL_MemWrite(processInfo.pid, address, reinterpret_cast<PBYTE>(buffer), size);
}


ULONG64 DMAHandler::patternScan(const char* pattern, const std::string& mask, bool returnCSOffset)
{
	assertNoInit();
	//technically not write if you use the same pattern but once with RVA flag and once without
	//but i dont see any case where both results are needed so i cba
	static std::unordered_map<const char*, uint64_t> patternMap{};

	static std::vector<IMAGE_SECTION_HEADER> sectionHeaders;
	static char* textBuff = nullptr;
	static bool init = false;
	static DWORD virtualSize = 0;
	static uint64_t vaStart = 0;

	auto CheckMask = [](const char* Base, const char* Pattern, const char* Mask) {
		for (; *Mask; ++Base, ++Pattern, ++Mask) {
			if (*Mask == 'x' && *Base != *Pattern) {
				return false;
			}
		}
		return true;
	};

	if (patternMap.contains(pattern))
		return patternMap[pattern];

	if (!init)
	{
		init = true;

		static IMAGE_DOS_HEADER dosHeader;
		static IMAGE_NT_HEADERS ntHeaders;

		dosHeader = read<IMAGE_DOS_HEADER>(getBaseAddress());


		if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
			throw std::runtime_error("dosHeader.e_magic invalid!");

		ntHeaders = read<IMAGE_NT_HEADERS>(getBaseAddress() + dosHeader.e_lfanew);

		if (ntHeaders.Signature != IMAGE_NT_SIGNATURE)
			throw std::runtime_error("ntHeaders.Signature invalid!");

		const DWORD sectionHeadersSize = ntHeaders.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
		sectionHeaders.resize(ntHeaders.FileHeader.NumberOfSections);

		read(getBaseAddress() + dosHeader.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), reinterpret_cast<DWORD64>(sectionHeaders.data()), sectionHeadersSize);


		for (const auto& section : sectionHeaders) {
			std::string sectionName(reinterpret_cast<const char*>(section.Name));
			if (sectionName == ".text") {
				textBuff = static_cast<char*>(calloc(section.Misc.VirtualSize, 1));
				read(getBaseAddress() + section.VirtualAddress, reinterpret_cast<DWORD64>(textBuff), section.Misc.VirtualSize);
				virtualSize = section.Misc.VirtualSize;
				vaStart = getBaseAddress() + section.VirtualAddress;
			}
		}
	}


	const int length = virtualSize - mask.length();

	for (int i = 0; i <= length; ++i)
	{
		char* addr = &textBuff[i];

		if (!CheckMask(addr, pattern, mask.c_str()))
			continue;

		const uint64_t uAddr = reinterpret_cast<uint64_t>(addr);

		if (returnCSOffset)
		{
			const auto res = vaStart + i + *reinterpret_cast<int*>(uAddr + 3) + 7;
			patternMap.insert(std::pair(pattern, res));
			return res;
		}

		const auto res = vaStart + i;
		patternMap.insert(std::pair(pattern, res));
		return res;
	}
	return 0;
}

void DMAHandler::closeDMA()
{
	log("DMA closed!");
	DMA_INITIALIZED = FALSE;
	VMMDLL_Close();
}

#if COUNT_TOTAL_READSIZE

DWORD64 DMAHandler::getTotalReadSize()
{
	return readSize;
}

void DMAHandler::resetReadSize()
{
	log("Bytes read since last reset: %llu B, %llu KB, %llu MB", readSize, readSize / 1024, readSize / 1024 / 1024);
	readSize = 0;
}

#endif
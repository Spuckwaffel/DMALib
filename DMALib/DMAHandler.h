#pragma once
#include <string>
#include <Windows.h>

// set to FALSE if you dont want to track the total read size of the DMA
#define COUNT_TOTAL_READSIZE TRUE

class DMAHandler
{
	// Static variables, shared over all instances

	struct LibModules
	{
		HMODULE VMM = nullptr;
		HMODULE FTD3XX = nullptr;
		HMODULE LEECHCORE = nullptr;
	};

	static inline LibModules modules{};

	static inline BOOLEAN DMA_INITIALIZED = FALSE;

	// Counts the size of the reads in total. Reset every frame preferrably for memory tracking
	static inline DWORD64 readSize = 0;

	// Nonstatic variables, different for each class object on purpose, in case the user tries to access
	// multiple processes
	struct BaseProcessInfo
	{
		DWORD pid = 0;
		std::string name = nullptr;
		const wchar_t* wname = nullptr;
		ULONG64 base = 0;
	};

	BaseProcessInfo processInfo{};

	BOOLEAN PROCESS_INITIALIZED = FALSE;

	// Private log function used by the DMAHandler class
	static void log(const char* fmt, ...);

	// Will always throw a runtime error if PROCESS_INITIALIZED or DMA_INITIALIZED is false
	void assertNoInit() const;


public:
	/**
	 * \brief Constructor takes a wide string of the process.
	 * Expects that all the libraries are in the root dir
	 * \param wname process name
	 */
	DMAHandler(const wchar_t* wname);

	// Whether the DMA and Process are initialized
	bool isInitialized() const;

	// Gets the PID of the process
	DWORD getPID() const;

	// Gets the Base address of the process
	ULONG64 getBaseAddress();

	void read(ULONG64 address, ULONG64 buffer, SIZE_T size) const;

	template <typename T>
	T read(void* address)
	{
		T buffer{};
		memset(&buffer, 0, sizeof(T));
		read(reinterpret_cast<ULONG64>(address), reinterpret_cast<ULONG64>(&buffer), sizeof(T));

		return buffer;
	}

	template <typename T>
	T read(ULONG64 address)
	{
		return read<T>(reinterpret_cast<void*>(address));
	}

	bool write(ULONG64 address, ULONG64 buffer, SIZE_T size) const;

	template <typename T>
	bool write(ULONG64 address, T* buffer)
	{
		return write(address, reinterpret_cast<ULONG64>(buffer), sizeof(T));
	}

	template <typename T>
	bool write(void* address, T* buffer)
	{
		return write(reinterpret_cast<ULONG64>(address), reinterpret_cast<ULONG64>(buffer), sizeof(T));
	}

	template <typename T>
	bool write(ULONG64 address, T value)
	{
		return write(address, reinterpret_cast<ULONG64>(&value), sizeof(T));
	}

	template <typename T>
	bool write(void* address, T value)
	{
		return write(reinterpret_cast<ULONG64>(address), reinterpret_cast<ULONG64>(&value), sizeof(T));
	}

	/**
	 * \brief pattern scans the text section and returns 0 if unsuccessful
	 * \param pattern the pattern
	 * \param mask the mask
	 * \param returnCSOffset in case your pattern leads to a xxx, cs:offset, it will return the address of the global variable instead
	 * \return the address
	 */
	ULONG64 patternScan(const char* pattern, const std::string& mask, bool returnCSOffset = true);

	/**
	 * \brief closes the DMA and sets DMA_INITIALIZED to FALSE
	 */
	static void closeDMA();

#if COUNT_TOTAL_READSIZE

	static DWORD64 getTotalReadSize();

	static void resetReadSize();

#endif
};

#include <Windows.h>
#include <iostream>

#include "DMAHandler.h"


int main()
{

	auto target = DMAHandler(L"MallocTest.exe");

	if(!target.isInitialized())
		DebugBreak();

	if (!target.getPID())
		DebugBreak();

	printf("PID: 0x%X\n", target.getPID());


	printf("base: 0x%llX\n", target.getBaseAddress());

	const auto res = target.read<uint64_t>(target.getBaseAddress() + 0x3038);

	printf("result: %llu\n", res);

	target.write(target.getBaseAddress() + 0x3038, 1337ull);

	getchar();
	DMAHandler::closeDMA();
	return 0;
}
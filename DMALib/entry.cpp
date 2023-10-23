#include <Windows.h>
#include <iostream>

#include "DMAHandler.h"


int main()
{
	auto target = DMAHandler(L"target.exe");

	//not initialized?
	if(!target.isInitialized())
		DebugBreak();

	//is the PID valid?
	if (!target.getPID())
		DebugBreak();

	//get the process ID, should not be 0
	printf("PID: 0x%X\n", target.getPID());

	//get the base address
	printf("base: 0x%llX\n", target.getBaseAddress());

	//read some random address
	auto res = target.read<uint64_t>(target.getBaseAddress() + 0x3038);

	//print the result
	printf("result: %llu\n", res);

	//write to the same address
	target.write(target.getBaseAddress() + 0x3038, 1337ull);

	//read again
	res = target.read<uint64_t>(target.getBaseAddress() + 0x3038);

	//print the result
	printf("result: %llu\n", res);

	getchar();
	DMAHandler::closeDMA();
	return 0;
}
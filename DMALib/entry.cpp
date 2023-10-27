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

	//Create a handle for scatter
	auto handle = target.createScatterHandle();

	//When doing scatter reads you can NOT read a pointer and have the next request use that pointer.
	//The pointer that you add to a scatter request has to be first executed before you can use any of the "Scattered" data.
	uint64_t res2;
	target.addScatterReadRequest(handle, target.getBaseAddress() + 0x3038, &res, sizeof(res));
	target.addScatterReadRequest(handle, target.getBaseAddress() + 0x3040, &res2, sizeof(res2));
	target.executeReadScatter(handle);

	printf("Read Scatter result: %llu\n", res);
	printf("Read Scatter result2: %llu\n", res2);

	
	//Always make sure you close your handle
	target.closeScatterHandle(handle);

	//print the result
	printf("result: %llu\n", res);

	getchar();
	target.closeDMA();
	return 0;
}
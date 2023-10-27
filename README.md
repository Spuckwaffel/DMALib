# DMALib

This is just a small library which contains the basic support for your DMA including
- memory reading
- memory writing
- getting PID and Base Address
- pattern scanning
- scatter reading
- logging
- good documentation and clean code

Feel free to modify the code or make it better. Replace the example dlls with your own dlls.

## Please read!

The program expects you to have the dlls FTD3XX.dll, leechcore.dll and vmm.dll (download them from your DMA supplier) at the root directory when shipping the program.

The project requires the leechcore.lib and vmm.lib libraries in the libs/ folder. I did not add the precompiled libraries for security purposes. 
You can get the files from 
https://github.com/ufrisk/LeechCore
and
https://github.com/ufrisk/MemProcFS/tree/master/vmm

and compiled from if you are lazy.
https://github.com/ufrisk/MemProcFS/tree/master/includes/lib32

Also special thanks to ufrisk for the libraries i used in this project.
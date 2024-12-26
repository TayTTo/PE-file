# Pefile Injection.
**P/s: The notepad++ sample is notepad++ 32 bit, program in this repo will not manipulate 64bit app**
## Pefile parser (In the folder Parser).
This file will extract all the fields in the PE header of a PE file.
Run tool using : `peparser.exe filepath`.
Here are some results:
> Resutl:
> ![](./images/pic1.png)
> ![](./images/pic2.png)
> ![](./images/pic3.png)

## Message box injection (In the MessageBoxInject).
The program will inject a message box to a PE file.
Run the program using the following cmd: `inject.exe filepath` .
> Result:
> ![](./images/pic4.png)
> ![](./images/pic5.png)

## Inject using a masm shell (In the InjectAllDirectory).
- The program will inject a shell code to another pe file, this shell code can load windows api for it self and it will infect the whole folder.
- Run the following command: `injectDir.exe .`
**Note that "." mean the whole folder**
> Result:
> ![](./images/pic6.png)
When try to run notepad++.exe, it will display message box
> ![](./images/pic7.png)
> ![](./images/pic8.png)
Message box when open peparser.exe
> ![](./images/pic9.png)

## Inject and run normally (In the folder InjectAndRunNormally)
- Run command `infected.exe fileName`
This program will inject a messageBox to a PEfile and after quit the messageBox, the infected pe file will run normally.

## ProcessHollowing.
Navigate to processhol dir, run file run.bat to compile the file.
the msgbox is undered msgbox dir, also run.bat to compile.
- Don't know for some reason, when I let the file process hollowing run, it will let the msgbox.exe file sleep right away, it will start run when I restart the process using taskmanager of ProcessHacker.
![](./images/pic10.png)

## C2
Navigate to dir C2, run 2 bat file to compile server and client program, the put the client program into a Vm and try run the server first, then the client. The client for Linux is in the repo C2/clientOnLinux.
> Result:
> ![](./images/pic11.png)

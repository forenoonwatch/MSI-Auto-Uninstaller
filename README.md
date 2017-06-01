# MSI Auto Uninstaller

### What is it?
The MSI Auto Uninstaller is a program with the ability to uninstall a program in a similar way to clicking on the program via "Add or Remove Programs" on windows, except entirely via command line.

### Why would I use this?
You would use this at any time you want to automatically or quietly uninstall a program that you don't have integrated command line support for uninstalling. The use-case it was originally designed for was around LogMeIn's remote access client. I needed to uninstall LogMeIn through its own "One2Many" service, which is able to run programs remotely on other computers, but other methods of uninstalling the LogMeIn program did not work, so I made this.

### How do I use it?
Simply call the program from a command line with Administrator permissions with the first and only parameter being the desired program's full name:
`msi-uninstall "Program Name"`

### How does it work?
This program searches several key parts of the Windows registry where the GUIDs for program installations are stored, and matches them to the given program name. It then uses Windows' built in msiexec.exe to uninstall the program.

### Future plans
* Add the ability to search with partial or non-exact names of programs, with a more powerful and complete search algorithm
* Create a "lite" version of the program using assembly for the sake of a smaller executable
* Add support for a wider range of installations

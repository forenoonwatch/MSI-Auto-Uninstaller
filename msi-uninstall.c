#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_BUFFER_SIZE		255

#define ROOT_WOW_NODE		"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
#define ROOT_SOFTWARE		"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"

typedef int bool;

enum {
	false = 0,
	true
};

/**
 * Finds the GUID for a MSI install among the given root key's subkeys
 * The GUID is put into the [result] buffer and its string length is put in
 * the [resultSize] value
 */
bool findGUIDEntry(HKEY root, const char* programName, char* result, size_t* resultSize);

bool isCorrectKey(HKEY, HKEY*, const char*, const char*, size_t);
void uninstallByGUID(const char*);

int main(int argc, char** argv) {
	if (argc != 2) {
		printf("Invalid number of arguments: %d\n", argc - 1);
		puts("Expected: msi-uninstall <Program Name>");
		return 1;
	}

	int result;
	HKEY uninstallRootKey;

	char* installerName = (char*)malloc(MAX_BUFFER_SIZE * sizeof(char));
	size_t installerNameLen = MAX_BUFFER_SIZE;
	
	bool foundInstaller = false;

	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ROOT_SOFTWARE,
		0, KEY_READ, &uninstallRootKey);

	if (result != ERROR_SUCCESS) {
		printf("Error: failed to open root key: %s\n", ROOT_SOFTWARE);
		RegCloseKey(uninstallRootKey);
		free(installerName);

		return 1;
	}

	if (findGUIDEntry(uninstallRootKey, argv[1], installerName, &installerNameLen)) {
		printf("Found %s in software root\n", argv[1]);
		foundInstaller = true;
		puts(installerName);
	}
	else {
		printf("Did not find %s in software root\n", argv[1]);
	}

	RegCloseKey(uninstallRootKey);

	if (!foundInstaller) {
		result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, ROOT_WOW_NODE,
			0, KEY_READ, &uninstallRootKey);

		if (result != ERROR_SUCCESS) {
			printf("Error: failed to open root key: %s\n", ROOT_WOW_NODE);
			RegCloseKey(uninstallRootKey);
			free(installerName);

			return 1;
		}

		if (findGUIDEntry(uninstallRootKey, argv[1], installerName, &installerNameLen)) {
			printf("Found %s in WOW Node root\n", argv[1]);
			foundInstaller = true;
			puts(installerName);
		}
		else {
			printf("Did not find %s in WOW Node root\n", argv[1]);
		}

		RegCloseKey(uninstallRootKey);
	}

	if (foundInstaller) {
		printf("Uninstalling %s\n", argv[1]);
		uninstallByGUID(installerName);
	}

	free(installerName);

	return 0;
}

bool findGUIDEntry(HKEY root, const char* programName, char* result, size_t* resultSize) {
	DWORD numSubKeys = 0;
	DWORD maxSubKeyLen = MAX_BUFFER_SIZE;

	RegQueryInfoKey(root, NULL, NULL, NULL, &numSubKeys, &maxSubKeyLen,
		NULL, NULL, NULL, NULL, NULL, NULL);

	char* keyNameBuffer = (char*)malloc(maxSubKeyLen * sizeof(char));
	DWORD keyNameSize;

	HKEY currentKey;
	int errCode;

	int programNameLen = strlen(programName);

	for (int i = 0; i < numSubKeys; ++i) {
		keyNameSize = MAX_BUFFER_SIZE;

		errCode = RegEnumKeyEx(root, i, keyNameBuffer, &keyNameSize, NULL, NULL, NULL, NULL);

		// TODO: possibly log errors
		if (errCode == ERROR_SUCCESS) {
			currentKey = NULL;

			if (isCorrectKey(root, &currentKey, keyNameBuffer, programName, programNameLen) && currentKey != NULL) {
				strcpy(result, keyNameBuffer);
				*resultSize = keyNameSize;

				free(keyNameBuffer);
				return true;
			}

			RegCloseKey(currentKey);
		}
	}

	free(keyNameBuffer);
	return false;
}

bool isCorrectKey(HKEY parent, HKEY* keyOut, const char* subKeyName, const char* queryName, size_t queryLen) {
	int res = RegOpenKeyEx(parent, subKeyName, 0, KEY_READ, keyOut);

	// TODO: possibly log errors
	if (res != ERROR_SUCCESS) {
		RegCloseKey(*keyOut);
		return false;
	}

	DWORD numValues = 0;
	DWORD maxValueNameLen = MAX_BUFFER_SIZE;
	DWORD maxValueLen = MAX_BUFFER_SIZE;

	RegQueryInfoKey(*keyOut, NULL, NULL, NULL, NULL, NULL, NULL, &numValues,
		&maxValueNameLen, &maxValueLen, NULL, NULL);

	char* nameBuffer = (char*)malloc(maxValueNameLen * sizeof(char));
	BYTE* dataBuffer = (BYTE*)malloc(maxValueLen * sizeof(BYTE));

	DWORD nameLength, dataLength;

	for (int i = 0; i < numValues; ++i) {
		nameLength = maxValueNameLen;
		dataLength = maxValueLen;

		RegEnumValue(*keyOut, i, nameBuffer, &nameLength, NULL, NULL, dataBuffer, &dataLength);

		if (strcmp(nameBuffer, "DisplayName") == 0) {
			if (memcmp(dataBuffer, queryName, dataLength < queryLen ? queryLen : dataLength) == 0) {
				free(nameBuffer);
				free(dataBuffer);

				return true;
			}
		}
	}

	free(nameBuffer);
	free(dataBuffer);

	return false;
}

void runAsAdmin(const char* program, const char* args) {
	ShellExecute(NULL, "runas", program, args, NULL, SW_SHOWNORMAL);
}

void uninstallByGUID(const char* guid) {
	char commandString[MAX_BUFFER_SIZE];
	sprintf(commandString, "/uninstall %s /q", guid);

	runAsAdmin("msiexec", commandString);
}

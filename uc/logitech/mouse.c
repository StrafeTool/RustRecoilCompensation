#include "mouse.h"
#include <windows.h>
#include <winternl.h>
#pragma comment(lib, "ntdll.lib")

typedef struct {
	char Button;
	char X;
	char Y;
	char Wheel;
	char Unk1;
} MOUSE_IO;

#define MOUSE_PRESS 1
#define MOUSE_RELEASE 2
#define MOUSE_MOVE 3
#define MOUSE_CLICK 4

static HANDLE GInput;
static IO_STATUS_BLOCK GIO;

BOOL FoundMouse;

static BOOL CallMouse(MOUSE_IO* Buffer)
{
	IO_STATUS_BLOCK Block;
	return NtDeviceIoControlFile(GInput, 0, 0, 0, &Block, 0x2a2010, Buffer, sizeof(MOUSE_IO), 0, 0) == 0L;
}

static NTSTATUS DeviceInitialize(PCWSTR DeviceName)
{
	UNICODE_STRING Name;
	OBJECT_ATTRIBUTES Attributes;

	RtlInitUnicodeString(&Name, DeviceName);
	InitializeObjectAttributes(&Attributes, &Name, 0, NULL, NULL);

	NTSTATUS Status = NtCreateFile(&GInput, GENERIC_WRITE | SYNCHRONIZE, &Attributes, &GIO, 0,
		FILE_ATTRIBUTE_NORMAL, 0, 3, FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, 0, 0);

	return Status;
}

BOOL MouseOpen(void)
{
	NTSTATUS Status = 0;

	if (GInput == 0) {

		wchar_t Buffer[] = L"\\??\\ROOT#SYSTEM#0001#{1abc05c0-c378-41b9-9cef-df1aba82b015}";

		Status = DeviceInitialize(Buffer);
		if (NT_SUCCESS(Status))
			FoundMouse = 1;
	}
	return Status == 0;
}

void MouseClose(void)
{
	if (GInput != 0) {
		NtClose(GInput);	
		GInput = 0;
	}
}

void MouseMove(char Button, char X, char Y, char Wheel)
{
	MOUSE_IO IO;
	IO.Unk1 = 0;
	IO.Button = Button;
	IO.X = X;
	IO.Y = Y;
	IO.Wheel = Wheel;

	if (!CallMouse(&IO)) {
		MouseClose();
		MouseOpen();
	}
}

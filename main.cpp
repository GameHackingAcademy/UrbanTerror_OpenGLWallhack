/*
A wallhack for Urban Terror 4.3.4 that reveals entities through walls by hooking the game's OpenGL function "glDrawElements" and disabling
depth-testing for OpenGL.

This is done by location the glDrawElements function inside the OpenGL library and create two codecaves, one at the start of the function and 
one at the end. In the first codecave, we call glDepthRange to clear the depth clipping plane and glDepthFunc to disable depth testing. In the
second codecave, we call these same functions to re-enable the depth clipping plane and re-enable depth testing.

This DLL must be injected into the Urban Terror process to work. One way to do this is to use a DLL injector. 
Another way is to enable AppInit_DLLs in the registry.

The offsets and method to discover them are discussed in the article at: https://gamehacking.academy/lesson/23
*/
#include <Windows.h>

HMODULE openGLHandle = NULL;

// Function pointers for two OpenGL functions that we will dynamically populate
// after injecting our DLL
void (__stdcall *glDepthFunc)(unsigned int) = NULL;
void (__stdcall *glDepthRange)(double, double) = NULL;

unsigned char* hook_location;

DWORD ret_address = 0;
DWORD count = 0;
DWORD old_protect;

// Codecave that runs before glDrawElements is called
__declspec(naked) void pre_codecave() {
	// First, we retrieve the count parameter from the original call.
	// Then, we retrieve the value of the count parameter, which specifies the amount
	// of indicies to be rendered
	__asm {
		pushad
		mov eax, dword ptr ds:[esp+0x10]
		mov count, eax 
		popad
		pushad
	}

	// If the count is over 500, we clear the depth clipping plane and then 
	// set the depth function to GL_ALWAYS
	if (count > 500) {
		(*glDepthRange)(0.0, 0.0);
		(*glDepthFunc)(0x207);
	}

	// Finally, restore the original instruction and jump back
	__asm {
		popad
		mov esi, dword ptr ds:[esi+0xA18]
		jmp ret_address
	}
}

// Codecave that runs after glDrawEntities is called
__declspec(naked) void post_codecave() {
	__asm {
		pushad
	}

	// Restore the depth clipping plane to the game's default value and then
	// set the depth function to GL_LEQUAL
	(*glDepthRange)(0.0, 1.0);
	(*glDepthFunc)(0x203);

	// Restore the original instructions
	__asm {
		popad
		pop esi
		pop ebp
		ret 0x10
	}
}

// The injected thread responsible for creating our hooks
void injected_thread() {
	while (true) {
		// Since OpenGL will be loaded dynamically into the process, our thread needs to wait
		// until it sees that the OpenGL module has been loaded.
		if (openGLHandle == NULL) {
			openGLHandle = GetModuleHandle(L"opengl32.dll");
		}

		// Once loaded, we first find the location of the two depth functions we are using in our
		// codecaves above
		if (openGLHandle != NULL && glDepthFunc == NULL) {
			*(FARPROC*)&glDepthFunc = GetProcAddress(openGLHandle, "glDepthFunc");
			*(FARPROC*)&glDepthRange = GetProcAddress(openGLHandle, "glDepthRange");

			// Then we find the location of glDrawElements and offset to an instruction that is easy to hook
			hook_location = (unsigned char*)GetProcAddress(openGLHandle, "glDrawElements");
			hook_location += 0x16;

			// For each hook, we unprotect the memory at the code we wish to write at
			// Then set the first opcode to E9, or jump
			// Caculate the location using the formula: new_location - original_location+5
			// And finally, since the first original instructions totalled 6 bytes, NOP out the last remaining byte
			VirtualProtect((void*)hook_location, 5, PAGE_EXECUTE_READWRITE, &old_protect);
			*hook_location = 0xE9;
			*(DWORD*)(hook_location + 1) = (DWORD)&pre_codecave - ((DWORD)hook_location + 5);
			*(hook_location + 5) = 0x90;

			// Since OpenGL is loaded dynamically, we need to dynamically calculate the return address
			ret_address = (DWORD)(hook_location + 0x6);

			// The offset for the post call hook
			hook_location = (unsigned char*)GetProcAddress(openGLHandle, "glDrawElements");
			hook_location += 0x29;

			VirtualProtect((void*)hook_location, 5, PAGE_EXECUTE_READWRITE, &old_protect);
			*hook_location = 0xE9;
			*(DWORD*)(hook_location + 1) = (DWORD)&post_codecave - ((DWORD)hook_location + 5);
		}

		// So our thread doesn't constantly run, we have it pause execution for a millisecond.
        	// This allows the processor to schedule other tasks.
		Sleep(1);
	}
}

// When our DLL is loaded, create a thread in the process to create the hook
// We need to do this as our DLL might be loaded before OpenGL is loaded by the process
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	if (fdwReason == DLL_PROCESS_ATTACH) {
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)injected_thread, NULL, 0, NULL);
	}

	return true;
}

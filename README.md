# UrbanTerror_OpenGLWallhack

A wallhack for Urban Terror 4.3.4 that reveals entities through walls by hooking the game's OpenGL function "glDrawElements" and disabling depth-testing for OpenGL.

This is done by location the glDrawElements function inside the OpenGL library and create two codecaves, one at the start of the function and one at the end. In the first codecave, we call glDepthRange to clear the depth clipping plane and glDepthFunc to disable depth testing. In the second codecave, we call these same functions to re-enable the depth clipping plane and re-enable depth testing.

This DLL must be injected into the Urban Terror process to work. One way to do this is to use a DLL injector. Another way is to enable AppInit_DLLs in the registry.

The offsets and method to discover them are discussed in the article at: https://gamehacking.academy/lesson/23

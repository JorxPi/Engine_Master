#include "Globals.h"
#include "Application.h"
#include "ModuleEditor.h"

void log(const char file[], int line, const char* format, ...)
{
	static char tmp_string[4096];
	static char tmp_string2[4096];
	static va_list  ap;

	// Construct the string from variable arguments
	va_start(ap, format);
	vsprintf_s(tmp_string, 4095, format, ap);
	va_end(ap);

	const char* filename = strrchr(file, '\\');
	if (!filename) filename = strrchr(file, '/');
	filename = filename ? filename + 1 : file;

	sprintf_s(tmp_string2, 4095, "%s(%d) : %s", filename, line, tmp_string);
	OutputDebugStringA(tmp_string2);

	app->getModule<ModuleEditor>()->logg(tmp_string2);
}

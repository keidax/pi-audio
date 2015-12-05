#include <portaudio.h>
#include <stdio.h>

int main() {
	const PaVersionInfo * version_info = Pa_GetVersionInfo();
	printf("%s\n", version_info->versionText);
	return 0;
}

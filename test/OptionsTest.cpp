
#include "Options.h"

int main(int argc, char ** argv)
{
	Options * options = Options::Instance();
	if (options->ParseCommandLine(argc, argv))
	{
		options->Show();
	}
	return 0;
}

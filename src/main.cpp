#include <cstdlib>
#include <exception>
#include <iostream>

#include "renderer.h"

int main()
{
	try
	{
		OspreyApp app;
		app.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
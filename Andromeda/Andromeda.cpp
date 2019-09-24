#include <cstdio>

#include "color/color.hpp"
#include "utils.hpp"

#include "APK.hpp"

#include "linenoise/linenoise.hpp"

void usage()
{
	printf("Usage:\n\tAndromeda apk_file_path\n");
}

void print_todo()
{
	color::color_printf(color::FG_LIGHT_RED, "TODO\n");
}

void help_commands()
{
	printf("Commands:\n");
	color::color_printf(color::FG_LIGHT_GREEN, "entry_points [ep]");
	printf(" - print list of entry points [LIMITED]\n");
	color::color_printf(color::FG_LIGHT_GREEN, "entry_points_extended [epe]");
	printf(" - print all possible entry points\n");
	color::color_printf(color::FG_LIGHT_GREEN, "classes");
	printf(" - print all classes from APK file\n");
	color::color_printf(color::FG_LIGHT_GREEN, "class_info [class] class_path");
	printf(" - print list of methods from a class\n");
	color::color_printf(color::FG_LIGHT_GREEN, "disassemble [dis] method_path");
	printf(" - disassemble a method\n");
	color::color_printf(color::FG_LIGHT_GREEN, "manifest");
	printf(" - print content of AndroidManifest.xml file\n");
	color::color_printf(color::FG_LIGHT_GREEN, "is_debuggable");
	printf(" - Checks android::debuggable field of AndroidManifest.xml file\n");
	color::color_printf(color::FG_LIGHT_GREEN, "certificate");
	printf(" - print content of root certificate\n");
	color::color_printf(color::FG_LIGHT_GREEN, "creation_date");
	printf(" - print creation date of the application based on a certificate\n");

	color::color_printf(color::FG_LIGHT_GREEN, "clr");
	printf(": Clear screen\n");
	color::color_printf(color::FG_LIGHT_GREEN, "\nexit/quit\n");
	printf("\n");
}

int main(const int argc, char* argv[])
{
	utils::clrscr();
	color_printf(color::FG_LIGHT_RED, "A n d r o m e d a ");
	color_printf(color::FG_LIGHT_CYAN, " - Interactive Reverse Engineering Tool for Android Applications\n\n");
	if (argc < 2)
	{
		usage();
		return -1;
	}
	// disable buffering
	setbuf(stdout, nullptr);

	const auto full_path = fs::absolute(argv[1]);
	if (!exists(full_path))
	{
		printf("Invalid file path: %ls\n", full_path.wstring().c_str());
		return -1;
	}
	//printf("File: %ls\n", full_path.wstring().c_str());

	// Setup completion words every time when a user types
	linenoise::SetCompletionCallback([](const char* editBuffer, std::vector<std::string>& completions)
	{
		if (editBuffer[0] == 'h')
		{
			completions.push_back("hello");
			completions.push_back("hello there");
		}
	});

	// Setup completion words every time when a user types
	linenoise::SetCompletionCallback([](const char* editBuffer, std::vector<std::string>& completions)
	{
		if (editBuffer[0] == 'e')
		{
			if (strlen(editBuffer) > 1 && editBuffer[1] == 'x')
			{
				completions.emplace_back("exit");
			}

			completions.emplace_back("ep");
			completions.emplace_back("entry_points");
			completions.emplace_back("epe");
			completions.emplace_back("entry_points_extended");
		}
		else if (editBuffer[0] == 'd')
		{
			completions.emplace_back("dis ");
			completions.emplace_back("disassemble ");
		}
		else if (editBuffer[0] == 'c')
		{
			completions.emplace_back("class ");
			completions.emplace_back("class_info ");
			completions.emplace_back("classes");

			completions.emplace_back("certificate");
			completions.emplace_back("creation_date");

			if (strlen(editBuffer) > 1 && editBuffer[1] == 'l')
			{
				completions.emplace_back("clr");
			}
		}
		else if (editBuffer[0] == 'm')
		{
			completions.emplace_back("manifest");
		}
		else if (editBuffer[0] == 'r')
		{
			completions.emplace_back("revoke_date");
		}
		else if (editBuffer[0] == 'i')
		{
			completions.emplace_back("is_debuggable");
		}

		else if (editBuffer[0] == 'h')
		{
			completions.emplace_back("help");
		}
	});

	// PROCESS APK FILE
	andromeda::apk apk(full_path);
	if (!apk.is_valid)
	{
		printf("Failed to parse APK file\n");
		return -1;
	}

	while (true)
	{
		std::string line;
		const auto quit = linenoise::Readline("Andromeda> ", line);
		if (quit || line == "quit" || line == "exit")
		{
			//if (fs::exists(apk.))
			//{
			//    fs::remove_all(unpacked_dir);
			//}
			break;
		}
		linenoise::AddHistory(line.c_str());

		if (line == "?" || line == "help")
		{
			help_commands();
		}

		else if (line == "manifest")
		{
			apk.dump_manifest_file();
		}
		else if (line == "is_debuggable")
		{
			apk.is_debuggable();
		}

		else if (line == "ep" || line == "entry_points")
		{
			apk.app_manifest->dump_entry_points();
		}
		else if (line == "epe" || line == "entry_points_extended")
		{
			apk.app_manifest->dump_entry_points(true);
		}

		else if (utils::starts_with(line, "class ") || utils::starts_with(line, "class_info "))
		{
			auto [_, class_path] = utils::split(line, ' ');
			if (!class_path.empty())
			{
				apk.dump_class_methods(class_path);
			}
			else
			{
				color::color_printf(color::FG_LIGHT_RED, "Invalid class path\n");
			}
		}

		else if (line == "classes")
		{
			apk.dump_classes();
		}

		else if (utils::starts_with(line, "dis ") || utils::starts_with(line, "disassemble "))
		{
			auto [_, method_path] = utils::split(line, ' ');
			if (!method_path.empty())
			{
				apk.disasm_method(method_path);
			}
			else
			{
				color::color_printf(color::FG_LIGHT_RED, "Invalid method path\n");
			}
		}
		else if (line == "certificate")
		{
			apk.dump_certificate();
		}
		else if (line == "creation_date")
		{
			apk.dump_creation_date();
		}
		else if (line == "revoke_date")
		{
			apk.dump_revoke_date();
		}

			// clear screen
		else if (line == "clr")
		{
			utils::clrscr();
		}
			// invalid command
		else
		{
			color::color_printf(color::FG_RED, "Invalid command: %s\n", line.c_str());
			help_commands();
		}

		// loop end
	}

	/* ----------- EOF ------------ */

	color_printf(color::FG_LIGHT_GREEN, "----------- EOF -----------\n");
	return 0;
}

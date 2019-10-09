#include <cstdio>

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
	color::color_printf(color::FG_YELLOW, "Commands:\n");

	printf("\n");
	color::color_printf(color::FG_LIGHT_GREEN, "entry_points [ep]");
	printf(" - print list of entry points [LIMITED]\n");
	color::color_printf(color::FG_LIGHT_GREEN, "entry_points_extended [epe]");
	printf(" - print all possible entry points\n");

	// permissions
	printf("\n");
	color::color_printf(color::FG_LIGHT_GREEN, "permissions [perms]");
	printf(" - permissions requested by the APK file\n");	
	// activities
	color::color_printf(color::FG_LIGHT_GREEN, "activities");
	printf(" - Names of activities contained in the APK file\n");
	// Services
	color::color_printf(color::FG_LIGHT_GREEN, "services");
	printf(" - Names of services contained in the APK file\n");
	// Receivers
	color::color_printf(color::FG_LIGHT_GREEN, "receivers");
	printf(" - Names of handlers declared in the APK file for receiving broadcasts\n");

	printf("\n");
	color::color_printf(color::FG_LIGHT_GREEN, "classes");
	printf(" - print all classes from APK file\n");
	color::color_printf(color::FG_LIGHT_GREEN, "class_info [class] class_path");
	printf(" - print list of methods from a class\n");
	color::color_printf(color::FG_LIGHT_GREEN, "find_class _str_");
	printf(" - find a class which contains _str_ string\n");

	printf("\n");
	color::color_printf(color::FG_LIGHT_GREEN, "methods [funcs]");
	printf(" - print all methods from APK file\n");
	color::color_printf(color::FG_LIGHT_GREEN, "disassemble [dis] method_path");
	printf(" - disassemble a method\n");
	color::color_printf(color::FG_LIGHT_GREEN, "find_method [find_func] _str_");
	printf(" - find a method which contains _str_ string\n");

	printf("\n");
	color::color_printf(color::FG_LIGHT_GREEN, "manifest");
	printf(" - print content of AndroidManifest.xml file\n");
	color::color_printf(color::FG_LIGHT_GREEN, "is_debuggable");
	printf(" - Checks android::debuggable field of AndroidManifest.xml file\n");
	color::color_printf(color::FG_LIGHT_GREEN, "certificate");
	printf(" - print content of root certificate\n");
	color::color_printf(color::FG_LIGHT_GREEN, "creation_date");
	printf(" - print creation date of the application based on a certificate\n");
	
	// libs
	printf("\n");
	color::color_printf(color::FG_LIGHT_GREEN, "libs");
	printf(" - print list of native library files\n");
	color::color_printf(color::FG_LIGHT_GREEN, "dump_libs");
	printf(" - write all lib files to disk\n");
	color::color_printf(color::FG_LIGHT_GREEN, "dump_lib lib_path");
	printf(" - write 'lib_path' file to disk\n");
	color::color_printf(color::FG_LIGHT_GREEN, "libs_hash [libh]");
	printf(" - SHA-1 hashes of lib files\n");
	
	// strings
	printf("\n");
	color::color_printf(color::FG_LIGHT_GREEN, "strings [strs]");
	printf(" - print the strings of APK (thanks to Strings Constant Pool)\n");
	color::color_printf(color::FG_LIGHT_GREEN, "string [str] search_string");
	printf(" - find \"search_string\" in the strings of APK\n");
	color::color_printf(color::FG_LIGHT_GREEN, "interesting_strings [???]"); // TODO(lasha): short form
	printf(" - Interesting/Suspicious strings from the APK file\n");

	// misc
	printf("\n");
	color::color_printf(color::FG_LIGHT_GREEN, "language [lang]");
	printf(" - print a language used to write the application\n");

	printf("\n");
	color::color_printf(color::FG_LIGHT_GREEN, "cls [clr]");
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
		else if (editBuffer[0] == 'a')
		{
			completions.emplace_back("activities");
		}
		else if (editBuffer[0] == 'd')
		{
			completions.emplace_back("dis ");
			completions.emplace_back("disassemble ");

			completions.emplace_back("dump_lib ");
			completions.emplace_back("dump_libs");
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
				completions.emplace_back("cls");
				completions.emplace_back("clr");
			}
		}
		else if (editBuffer[0] == 'f')
		{
			completions.emplace_back("find_class ");
			completions.emplace_back("find_method ");
			completions.emplace_back("find_func ");

			completions.emplace_back("funcs ");
		}
		else if (editBuffer[0] == 'm')
		{
			completions.emplace_back("manifest");
			completions.emplace_back("methods");
		}
		else if (editBuffer[0] == 'r')
		{
			completions.emplace_back("revoke_date");
			completions.emplace_back("receivers");
		}
		else if (editBuffer[0] == 's')
		{
			completions.emplace_back("strs");
			completions.emplace_back("strings");

			completions.emplace_back("str ");
			completions.emplace_back("string ");

			completions.emplace_back("services");
		}
		else if (editBuffer[0] == 'i')
		{
			completions.emplace_back("is_debuggable");
			completions.emplace_back("interesting_strings");
		}
		else if (editBuffer[0] == 'l')
		{
			completions.emplace_back("libs");
			completions.emplace_back("libs_hash");
			completions.emplace_back("libh");
			
			completions.emplace_back("language");
			completions.emplace_back("lang");
			
		}
		else if (editBuffer[0] == 'p')
		{
			completions.emplace_back("permissions");
			completions.emplace_back("perms");
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
		else if (line == "activities")
		{
			apk.dump_activities();
		}
		else if (line == "services")
		{
			apk.dump_services();
		}
		else if (line == "receivers")
		{
			apk.dump_receivers();
		}
		else if (line == "manifest")
		{
			apk.dump_manifest_file();
		}
		else if (line == "permissions" || line == "perms")
		{
			apk.dump_permissions();
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
		else if (utils::starts_with(line, "find_class "))
		{
			auto [_, class_part] = utils::split(line, ' ');
			if (!class_part.empty())
			{
				apk.find_dump_class(class_part);
			}
		}

		else if (line == "methods" || line == "funcs")
		{
			apk.dump_methods();
		}
		else if (utils::starts_with(line, "find_method ") || utils::starts_with(line, "find_func "))
		{
			auto [_, method_name] = utils::split(line, ' ');
			if (!method_name.empty())
			{
				apk.fin_dump_method(method_name);
			}
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

		// libs
		else if (line == "libs")
		{
			const auto libs = apk.get_libs(full_path);
			if (!libs.empty())
			{
				color::color_printf(color::FG_DARK_GRAY, "Libs:\n");
				for (const auto& lib : libs)
				{
					color::color_printf(color::FG_GREEN, "\t%s\n", lib.c_str());
				}
			}
		}
		else if (line == "dump_libs")
		{
			apk.get_libs(full_path, true);
		}
		else if (utils::starts_with(line, "dump_lib "))
		{
			auto [_, lib_path] = utils::split(line, ' ');
			if (!lib_path.empty())
			{
				apk.get_libs(full_path, true, lib_path);
			}
		}
		else if (line == "libs_hash" || line == "libh")
		{
			apk.get_libs(full_path, true, "", true);
		}
		

		// strings
		else if (line == "strings" || line == "strs")
		{
			apk.dump_strings();
		}
		else if (line == "interesting_strings")
		{
			apk.dump_interesting_strings();
		}
		else if (utils::starts_with(line, "str ") || utils::starts_with(line, "string "))
		{
			auto [_, target_string] = utils::split(line, ' ');
			if (!target_string.empty())
			{
				apk.search_string(target_string);
			}
		}

		// misc
		else if (line == "language" || line == "lang")
		{
			apk.dump_language();
		}

		// clear screen
		else if (line == "clr" || line == "cls" || line == "clear")
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

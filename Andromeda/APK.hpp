#pragma once

#include "utils.hpp"

#include "dex.hpp"
#include "manifest.hpp"
#include "cert.hpp"
#include "patterns.hpp"

#include "digestpp/digestpp.hpp"

namespace andromeda
{
	class apk
	{
	public:
		bool is_valid = false;
		std::shared_ptr<manifest> app_manifest;
		std::shared_ptr<andromeda::certificate> cert;
		std::vector<parsed_dex> parsed_dexes{};
		std::string unzip_path{};
		std::vector<std::string> file_pathes{};

		explicit apk(const std::string& full_path)
		{
			is_valid = true;
			// Unzip APK file

			if (utils::ends_with(full_path, ".apk"))
			{
				const auto unzip_result = utils::unzip_file(full_path, false);
				this->unzip_path = std::get<0>(unzip_result);
				this->file_pathes = std::get<1>(unzip_result);

				if (unzip_path.empty())
				{
					color_printf(color::FG_RED, "Failed to unpack the file: %s\n",
					             full_path.c_str());
					is_valid = false;
					return;
				}
			}
			else
			{
				printf("invalid valid format\npath: %s\n", full_path.c_str());
				is_valid = false;
				return;
			}

			// certificate
			const auto certificate_dir = unzip_path + '/' + "META-INF";
			cert = std::shared_ptr<certificate>{new certificate(certificate_dir)};
			

			// manifest
			const auto manifest_path = unzip_path + '/' + "AndroidManifest.xml";
			if (fs::exists(manifest_path))
			{
				app_manifest = std::shared_ptr<manifest>{new manifest(manifest_path)};
			}
			else
			{
				printf("Failed to locate AndroidManifest.xml file\nPath: %s\n",
				       manifest_path.c_str());
				is_valid = false;
				return;
			}

			// dex
			for (auto& p : fs::directory_iterator(unzip_path))
			{
				const auto& dex_path = p.path();
				if (dex_path.extension() == ".dex")
				{
					parsed_dex current_dex(dex_path);
					parsed_dexes.emplace_back(current_dex);
				}
			}
			if (parsed_dexes.empty())
			{
				printf("Failed to parse DEX files\n");
				is_valid = false;
				return;
			}

			// ctor end
		}

		apk(const apk&) = default;
		apk& operator=(const apk&) = default;

		std::vector<std::string> get_libs(const fs::path& file_path, bool extract = false, const std::string& target_lib_path = "", const bool get_hash = false)
		{
			if (get_hash == true)
			{
				extract = true;
			}

			std::vector<std::string> libs{};

			mz_zip_archive zip_archive;
			memset(&zip_archive, 0, sizeof(zip_archive));

			const auto status = mz_zip_reader_init_file(&zip_archive, file_path.string().c_str(), 0);
			if (!status)
				return libs;

			const auto file_count = mz_zip_reader_get_num_files(&zip_archive);
			if (file_count == 0)
			{
				mz_zip_reader_end(&zip_archive);
				return libs;
			}
			// printf("number_of_files: %zu\n", file_count);

			const auto dest_dir = fs::current_path().string() + '/' + "libs";

			const auto current_path = fs::current_path().string();
			for (auto i = 0; i < file_count; i++)
			{
				mz_zip_archive_file_stat file_stat;
				if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
				{
					printf("failed to get file stat. index: %d\n", i);
					continue;
				}
				if (mz_zip_reader_is_file_a_directory(&zip_archive, i))
				{
					continue;
				}

				const std::string file_name{file_stat.m_filename};
				const auto dest_file = dest_dir + '/' + file_name;

				if (utils::starts_with(file_name, "lib/"))
				{
					const auto [_, lib_path] = utils::split(file_name, '/');
					if (!extract)
					{
						if (!lib_path.empty())
						{
							libs.emplace_back(lib_path);
						}
						continue;
					}
					else if (extract && !target_lib_path.empty())
					{
						if (target_lib_path != lib_path)
						{
							continue;
						}
					}

					const fs::path under_dir_path{dest_file};
					const std::string under_dir_full = under_dir_path.parent_path();
					if (!fs::exists(under_dir_full))
					{
						fs::create_directories(under_dir_full);
					}

					const auto is_okay = mz_zip_reader_extract_to_file(&zip_archive, i, dest_file.c_str(), 0);
					if (!is_okay)
					{
						color::color_printf(color::FG_LIGHT_RED, "[APK.hpp] Failed to unpack file: %s\n", file_name.c_str());
					}
					else
					{
						if (get_hash == true)
						{
							const auto file_content = utils::read_file_content(dest_file);
							const auto file_sha1_ascii = digestpp::sha1().absorb(file_content).hexdigest();
							color::color_printf(color::FG_GREEN, "%s: ", file_name.c_str());
							color::color_printf(color::FG_DARK_GRAY, "%s\n", file_sha1_ascii.c_str());
						}
						else
						{
							color::color_printf(color::FG_GREEN, "unpacked lib: %s\n", dest_file.c_str());
						}
					}
				}

			}

			mz_zip_reader_end(&zip_archive);

			return libs;
		}

		void dump_classes()
		{
			for (auto& dex : parsed_dexes)
			{
				const auto dex_classes = dex.get_classes();
				if (!dex_classes.empty())
				{
					color::color_printf(color::FG_DARK_GRAY, "DEX file: %s\n", dex.get_dex_name().c_str());
					for (const auto& i_class : dex_classes)
					{
						color::color_printf(color::FG_GREEN, "\t%s\n", i_class.c_str());
					}
				}
			}

		}

		void find_dump_class(const std::string& class_part)
		{
			for (auto& dex : parsed_dexes)
			{
				const auto dex_classes = dex.get_classes();
				if (!dex_classes.empty())
				{
					for (const auto& i_class : dex_classes)
					{
						if (utils::find_case_insensitive(i_class, class_part) != std::string::npos)
						{
							color::color_printf(color::FG_DARK_GRAY, "DEX file: %s\n", dex.get_dex_name().c_str());
							color::color_printf(color::FG_GREEN, "\t%s\n", i_class.c_str());
						}
					}
				}
			}
		}

		void dump_methods()
		{
			for (auto& parsed_dex : parsed_dexes)
			{
				const auto dex_methods = parsed_dex.get_methods();
				if (!dex_methods.empty())
				{
					color::color_printf(color::FG_DARK_GRAY, "DEX file: %s\n", parsed_dex.get_dex_name().c_str());
					for (const auto& [class_path, method_name] : dex_methods)
					{
						color::color_printf(color::FG_DARK_GRAY, "%s.", class_path.c_str());
						color::color_printf(color::FG_GREEN, "%s\n", method_name.c_str());
					}
				}
			}
		}

		void fin_dump_method(const std::string& target_method_name)
		{
			for (auto& parsed_dex : parsed_dexes)
			{
				const auto dex_methods = parsed_dex.get_methods();
				if (!dex_methods.empty())
				{
					for (const auto& [class_path, method_name] : dex_methods)
					{
						if (utils::find_case_insensitive(method_name, target_method_name) != std::string::npos)
						{
							color::color_printf(color::FG_DARK_GRAY, "DEX file: %s\n", parsed_dex.get_dex_name().c_str());
							color::color_printf(color::FG_DARK_GRAY, "%s.", class_path.c_str());
							color::color_printf(color::FG_GREEN, "%s\n", method_name.c_str());
						}
					}
				}
			}
		}

		void dump_class_methods(const std::string& class_path)
		{
			auto found = false;
			color::color_printf(color::FG_LIGHT_GRAY, "Class: %s\n",
			                    class_path.c_str());
			for (auto& parsed_dex : parsed_dexes)
			{
				const auto methods = parsed_dex.get_class_methods(class_path);
				if (methods.empty())
				{
					continue;
				}
				const auto dex_name = parsed_dex.get_dex_name();
				color::color_printf(color::FG_DARK_GRAY, "DEX file: %s\n",
				                    dex_name.c_str());
				for (const auto& i_method : methods)
				{
					color::color_printf(color::FG_GREEN, "\t%s\n", i_method.c_str());
					found = true;
				}
			}

			if (!found)
			{
				color::color_printf(color::FG_LIGHT_RED, "Failed to locate a class\n");
			}
		}

		void disasm_method(const std::string& method_path)
		{
			auto found = false;
			for (auto& parsed_dex : parsed_dexes)
			{
				const auto current_found = parsed_dex.dump_method(method_path);
				found = found ? found : current_found;
			}

			if (!found)
			{
				color::color_printf(color::FG_LIGHT_RED, "Failed to locale method: %s\n",
				                    method_path.c_str());
			}
		}

		void dump_permissions() const 
		{
			if (!app_manifest->permissions.empty())
			{
				color::color_printf(color::FG_DARK_GRAY, "Permissions:\n");
				for (const auto& perm : app_manifest->permissions)
				{
					color::color_printf(color::FG_GREEN, "\t%s\n", perm.c_str());
				}
			}
		}

		
		void dump_activities() const 
		{
			if (!app_manifest->activities.empty())
			{
				color::color_printf(color::FG_DARK_GRAY, "Activities:\n");
				for (const auto& [name, intents]  : app_manifest->activities)
				{
					auto full_class_name = name;
					if (!full_class_name.empty() && full_class_name[0] == '.')
					{
						if (!app_manifest->manifest_package.empty())
						{
							full_class_name = app_manifest->manifest_package + full_class_name;
						}
					}

					color_printf(color::FG_GREEN, "\t%s\n", full_class_name.c_str());
				}
			}
		}

		void dump_services() const
		{
			if (!app_manifest->services.empty())
			{
				color::color_printf(color::FG_DARK_GRAY, "Services:\n");
				for (const auto& [name, intents]  : app_manifest->services)
				{
					color_printf(color::FG_GREEN, "\t%s\n", name.c_str());
				}
			}
		}

		void dump_receivers() const
		{
			if (!app_manifest->receivers.empty())
			{
				color::color_printf(color::FG_DARK_GRAY, "Receivers:\n");
				for (const auto& [name, intents]  : app_manifest->receivers)
				{
					color_printf(color::FG_GREEN, "\t%s\n", name.c_str());
				}
			}
		}

		void is_debuggable() const
		{
			const auto is_debug = app_manifest->is_debuggable();
			if (is_debug)
			{
				color::color_printf(color::FG_LIGHT_GREEN, "Yes\n");
			}
			else
			{
				color::color_printf(color::FG_LIGHT_RED, "No\n");
			}
		}

		void dump_manifest_file() const
		{
			color::color_printf(color::FG_LIGHT_GREEN, "----------- BEGIN -----------\n");
			printf("%s\n", app_manifest->manifest_content.c_str());
			color::color_printf(color::FG_LIGHT_GREEN, "----------- EOF -----------\n");
		}

		// certificate
		void dump_certificate() const
		{
			color::color_printf(color::FG_LIGHT_GREEN, "----------- BEGIN -----------\n");
			printf("%s\n", cert->get_certificate().get());
			color::color_printf(color::FG_LIGHT_GREEN, "----------- EOF -----------\n");
		}

		void dump_creation_date() const 
		{
			printf("%s\n", cert->get_creation_date().get());
		}

		void dump_revoke_date() const 
		{
			printf("%s\n", cert->get_revoke_date().get());
		}

		// strings
		void dump_strings()
		{
			for (auto parsed_dex : parsed_dexes)
			{
				const auto dex_strings = parsed_dex.get_strings();
				if (!dex_strings.empty())
				{
					color::color_printf(color::FG_DARK_GRAY, "DEX file: %s\n", parsed_dex.get_dex_name().c_str());
					for (const auto& str : dex_strings)
					{
						color::color_printf(color::FG_GREEN, "\t%s\n", str.c_str());
					}
				}
			}
		}

		void dump_interesting_strings()
		{
			std::vector<std::string> urls{};
			std::vector<std::string> emails{};

			for (auto parsed_dex : parsed_dexes)
			{
				const auto dex_strings = parsed_dex.get_strings();
				if (!dex_strings.empty())
				{
					for (const auto& str : dex_strings)
					{
						if (is_url(str))
						{
							urls.emplace_back(str);
						}
						
						if(is_email(str))
						{
							emails.emplace_back(str);
						}
					}
				}
			}

			// URLs:
			if (!urls.empty())
			{
				color::color_printf(color::FG_DARK_GRAY, "URLs:\n");
				for (const auto& url : urls)
				{
					color::color_printf(color::FG_GREEN, "\t%s\n", url.c_str());
				}
			}

			// emails
			if (!emails.empty())
			{
				color::color_printf(color::FG_DARK_GRAY, "e-Mails:\n");
				for (const auto& email : emails)
				{
					color::color_printf(color::FG_GREEN, "\t%s\n", email.c_str());
				}
			}

		}

		void search_string(std::string& target_string)
		{
			for (auto parsed_dex : parsed_dexes)
			{
				auto dex_strings = parsed_dex.get_strings();
				if (!dex_strings.empty())
				{
					for (auto& str : dex_strings)
					{
						if (!str.empty() &&
							utils::find_case_insensitive(str, target_string) != std::string::npos )
						{
							color::color_printf(color::FG_DARK_GRAY, "%s: ", parsed_dex.get_dex_name().c_str());
							color::color_printf(color::FG_GREEN, "%s\n", str.c_str());
						}
					}
				}
			}
		}

		void dump_language()
		{
			std::string lang = "Java";
			auto print_color = color::FG_LIGHT_RED;

			for (const auto& file_path : this->file_pathes)
			{
				if (file_path.find("kotlin/") == 0) // starts_with
				{
					lang = "Kotlin";
					print_color = color::FG_CYAN;
					break;
				}
				else if (file_path.find("assemblies/Xamarin.") == 0)
				{
					lang = ".NET (Xamarin)";
					print_color = color::FG_BLUE;
					break;
				}
			}

			color::color_printf(print_color, "%s\n", lang.c_str());
		}

		// class apk
	};
} // namespace andromeda

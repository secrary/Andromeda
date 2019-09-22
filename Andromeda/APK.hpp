#pragma once

#include "color/color.hpp"
#include "dex.hpp"
#include "manifest.hpp"

#include "utils.hpp"

namespace andromeda
{
	class apk
	{
	public:
		bool is_valid = false;
		std::shared_ptr<manifest> app_manifest;
		std::vector<parsed_dex> parsed_dexes{};
		std::string unzip_path{};

		explicit apk(const std::string& full_path)
		{
			is_valid = true;
			// Unzip APK file

			if (utils::ends_with(full_path, ".apk"))
			{
				unzip_path = utils::unzip_file(full_path, false);
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

			// manifest
			const auto manifest_path = unzip_path + "/" + "AndroidManifest.xml";
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

		// class apk
	};
} // namespace andromeda

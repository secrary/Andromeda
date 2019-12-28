#pragma once

#include <vector>
#include <string>
#include <algorithm>

#include "color/color.hpp"
#include "pugixml/pugixml.hpp"

namespace andromeda
{
	class manifest
	{
		std::string application_class_name_{};

		enum class intent_target
		{
			activity,
			service,
			receiver
		};

		void dump_intents(const intent_target type)
		{
			auto ptr_target = &activities;
			switch (type)
			{
			case intent_target::service:
				ptr_target = &services;
				break;

			case intent_target::receiver:
				ptr_target = &receivers;

			default:
				break;
			}

			for (const auto& [name, intents] : *ptr_target)
			{
				color_printf(color::FG_GREEN, "\t%s\n", name.c_str());
				if (!intents.empty())
				{
					color_printf(color::FG_DARK_GRAY, "\t\tIntents:\n");
					for (const auto& intent : intents)
					{
						color_printf(color::FG_DEFAULT, "\t\t%s\n", intent.c_str());
					}
				}
			}
		}

		void emplace_intents(const pugi::xml_node& node, const intent_target type)
		{
			auto name = std::string{node.attribute("android:name").as_string()};
			auto alias_target = std::string{node.attribute("android:targetActivity").as_string()};
			if (!alias_target.empty())
			{
				name = alias_target;
			}

			if (!name.empty() && name[0] == '.' && !manifest_package.empty())
			{
				name = manifest_package + name;
			}

			std::vector<std::string> intent_filters{};
			for (const auto& intent_child : node.child("intent-filter").children("action"))
			{
				const auto intent_name = intent_child.attribute("android:name").as_string();
				intent_filters.emplace_back(intent_name);
			}

			if (type == intent_target::service)
			{
				services.emplace_back(name, intent_filters);
			}
			else if (type == intent_target::activity)
			{
				activities.emplace_back(name, intent_filters);
			}
			else if (type == intent_target::receiver)
			{
				receivers.emplace_back(name, intent_filters);
			}
		}

		bool decode_manifest(const std::string& manifest_path)
		{
			size_t file_size = 0;
			const auto file_content = utils::read_file(manifest_path, file_size);
			char* xml_content = nullptr;
			size_t xml_size = 0;
			AxmlToXml(&xml_content, &xml_size, file_content.get(), file_size);
			fs::remove(manifest_path);

			const auto status = utils::write_file(manifest_path, xml_content, xml_size);
			manifest_content = std::string(xml_content, xml_size);
			free(xml_content);
			if (status == false)
			{
				printf("Failed to write AndroidManifest.xml\n");
				return false;
			}

			return true;
		}

	public:
		std::vector<std::string> permissions{};
		std::string manifest_package;
		std::vector<std::pair<std::string, std::vector<std::string>>> activities{};
		std::vector<std::pair<std::string, std::vector<std::string>>> services{};
		std::vector<std::pair<std::string, std::vector<std::string>>> receivers{};
		std::string manifest_content;
		bool debuggable = false;

		explicit manifest(const std::string& xml_path)
		{
			const auto status = decode_manifest(xml_path);
			if (status == false)
			{
				printf("Failed to decode manifest file: %s\n", xml_path.c_str());
				return;
			}

			pugi::xml_document xml_doc;
			xml_doc.load_file(xml_path.c_str());

			auto is_debug_string = std::string{
				xml_doc.child("manifest").child("application").attribute("android:debuggable").as_string()
			};
			if (!is_debug_string.empty() && is_debug_string == "true")
			{
				debuggable = true;
			}

			// application class
			application_class_name_ = xml_doc.child("manifest").child("application").attribute("android:name").
			                                  as_string();
			manifest_package = std::string{xml_doc.child("manifest").attribute("package").as_string()};
			if (!application_class_name_.empty() && application_class_name_[0] == '.' && !manifest_package.empty())
			{
				application_class_name_ = manifest_package + application_class_name_;
			}

			// permissions
			for (const auto& child : xml_doc.child("manifest").children("uses-permission"))
			{
				const auto permission_name = child.attribute("android:name").as_string();
				permissions.emplace_back(permission_name);
			}
			// activities
			for (const auto& child : xml_doc.child("manifest").child("application").children("activity"))
			{
				emplace_intents(child, intent_target::activity);
			}
			for (const auto& child : xml_doc.child("manifest").child("application").children("activity-alias"))
			{
				emplace_intents(child, intent_target::activity);
			}

			// services
			for (const auto& child : xml_doc.child("manifest").child("application").children("service"))
			{
				emplace_intents(child, intent_target::service);
			}

			// Broadcast receivers
			for (const auto& child : xml_doc.child("manifest").child("application").children("receiver"))
			{
				emplace_intents(child, intent_target::receiver);
			}

			// ctor
		}

		// No copy/move semantics
		manifest(const manifest&) = delete;
		manifest& operator=(const manifest&) = delete;

		/* 
			dump entry points from manifest file
	
			Three of the four component types—activities, services, and broadcast receivers — are activated by an asynchronous message called an intent. 
			
			details: https://developer.android.com/guide/components/fundamentals
		*/
		void dump_entry_points(bool extended = false)
		{
			// application class
			if (!application_class_name_.empty())
			{
				color_printf(color::FG_LIGHT_GRAY, "Application class name:\n\t");
				color_printf(color::FG_GREEN, "%s\n", application_class_name_.c_str());
			}

			// main activity ("Activity Action: Start as a main entry point, does not expect to receive data.")
			for (const auto& [name, intents] : activities)
			{
				if (std::find(intents.begin(), intents.end(), R"(android.intent.action.MAIN)") != intents.end())
				{
					color_printf(color::FG_LIGHT_GRAY, "Main activity:\n\t");
					auto full_class_name = name;
					if (!full_class_name.empty() && full_class_name[0] == '.')
					{
						if (!manifest_package.empty())
						{
							full_class_name = manifest_package + full_class_name;
						}
					}

					color_printf(color::FG_GREEN, "%s\n", full_class_name.c_str());
					break;
				}
			}

			if (!extended)
			{
				return;
			}

			// activities
			if (!activities.empty())
			{
				color_printf(color::FG_LIGHT_GRAY, "Activities:\n");
				dump_intents(intent_target::activity);
			}

			// services
			if (!services.empty())
			{
				color_printf(color::FG_LIGHT_GRAY, "Services:\n");
				dump_intents(intent_target::service);
			}

			// receivers
			if (!receivers.empty())
			{
				color_printf(color::FG_LIGHT_GRAY, "Services:\n");
				dump_intents(intent_target::receiver);
			}

			// dump_entry_points()
		}

		bool is_debuggable() const
		{
			return debuggable;
		}

		// class: manifest
	};
} // namespace andromeda

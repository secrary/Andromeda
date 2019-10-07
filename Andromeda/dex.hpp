#pragma once

#include "utils.hpp"

// slicer
#include "slicer/dex_format.h"
#include "slicer/reader.h"
#include "slicer/common.h"
#include "slicer/code_ir.h"
#include "slicer/dex_ir.h"


#include "disassambler/dissassembler.h"

namespace andromeda
{
	class parsed_dex
	{
		std::shared_ptr<char> dex_content_ = nullptr;
		std::shared_ptr<dex::Reader> dex_reader_ = nullptr;
		std::vector<std::string> dex_classes_;
		std::vector<std::pair<std::string, std::string>> dex_methods_; // class_path, function_name
		std::vector<std::string> strings_pool; // thanks to Strings Constant Pool
		std::string dex_name_;

		static std::string name_to_descriptor(const std::string& name)
		{
			auto descriptor = name;
			std::replace(descriptor.begin(), descriptor.end(), '.', '/');
			descriptor = "L" + descriptor + ";";

			return descriptor;
		}

		static std::pair<std::string, std::string> split_method_path(const std::string& method_path)
		{
			std::string class_path, function_name;
			const auto found = method_path.find_last_of(".");
			if (found == std::string::npos)
			{
				return std::make_pair(class_path, function_name);
			}

			class_path = method_path.substr(0, found);
			function_name = method_path.substr(found + 1);

			return std::make_pair(class_path, function_name);
		}

	public:

		explicit parsed_dex(const fs::path& dex_path)
		{
			size_t file_size{};
			dex_name_ = dex_path.filename().string();
			dex_content_ = utils::read_file(dex_path.string(), file_size);
			dex_reader_ = std::shared_ptr<dex::Reader>{
				new dex::Reader((dex::u1*)(dex_content_.get()), file_size)
			};

			// ctor
		}

		parsed_dex(const parsed_dex&) = default;
		parsed_dex& operator=(const parsed_dex&) = default;

		std::string get_dex_name() const
		{
			return dex_name_;
		}

		std::vector<std::string> get_strings()
		{
			if (strings_pool.empty())
			{
				dex_reader_->CreateFullIr();
				auto ir = dex_reader_->GetIr();
				for (const auto& s : ir->strings)
				{
					auto current_string = std::string { s->c_str() };
					if (!current_string.empty())
					{
						current_string = utils::strip(current_string);
						strings_pool.emplace_back(current_string);
					}
				}
			}

			return strings_pool;
		}

		std::vector<std::string> get_classes()
		{
			if (dex_classes_.empty())
			{
				const auto classes = dex_reader_->ClassDefs();
				const auto types = dex_reader_->TypeIds();
				for (const auto& current_class : classes)
				{
					const auto type_id = types[current_class.class_idx];
					const auto descriptor = dex_reader_->GetStringMUTF8(type_id.descriptor_idx);
					dex_classes_.emplace_back(dex::DescriptorToDecl(descriptor));
				}
			}

			return dex_classes_;
		}

		std::vector<std::pair<std::string, std::string>> get_methods()
		{
			if (dex_methods_.empty())
			{
				dex_reader_->CreateFullIr();
				auto dex_ir = dex_reader_->GetIr();

				for (auto& current_method : dex_ir->methods)
				{
					//printf("%s!%s!%s\n", current_method->parent->Decl().c_str(), current_method->name->c_str(), current_method->prototype->Signature().c_str());
					dex_methods_.emplace_back(current_method->parent->Decl().c_str(), current_method->name->c_str());
				}

			}

			return dex_methods_;
		}

		std::vector<std::string> get_class_methods(const std::string& class_path) const
		{
			std::vector<std::string> class_methods;

			const auto class_descriptor = name_to_descriptor(class_path);
			const auto class_index = dex_reader_->FindClassIndex(class_descriptor.c_str());
			if (class_index == dex::kNoIndex)
			{
				// printf("Can not find a class: %s\n", class_path.c_str());
				return class_methods;
			}
			// printf("class found: %s\n\t%s\n", class_descriptor.c_str(), dex_name.c_str());

			dex_reader_->CreateClassIr(class_index);

			auto dex_ir = dex_reader_->GetIr();

			if (dex_ir->encoded_methods.empty())
			{
				return class_methods;
			}
			//color_printf(color::FG_DARK_GRAY, "Class: %s\n", class_descriptor.c_str());
			for (auto& ir_method : dex_ir->encoded_methods)
			{
				//color_printf(color::FG_GREEN, "\t%s\n", ir_method->decl->name->c_str());
				class_methods.emplace_back(ir_method->decl->name->c_str());
			}

			return class_methods;
			// get_class_methods
		}

		bool dump_method(const std::string& method_path) const
		{
			const auto [class_path, function_name] = split_method_path(method_path);

			auto found = false;
			const auto class_descriptor = name_to_descriptor(class_path);
			const auto class_index = dex_reader_->FindClassIndex(class_descriptor.c_str());
			if (class_index == dex::kNoIndex)
			{
				// printf("Can not find a class: %s\n\t%s\n", class_descriptor.c_str(), dex_name.c_str());
				return found;
			}

			dex_reader_->CreateClassIr(class_index);
			auto dex_ir = dex_reader_->GetIr();

			//printf("n_methods: %zu\n", dex_ir->encoded_methods.size());
			for (auto& ir_method : dex_ir->encoded_methods)
			{
				const std::string method_class_path = ir_method->decl->parent->Decl().c_str();
				if (method_class_path != class_path)
				{
					continue;
				}
				const std::string current_function_name = ir_method->decl->name->c_str();
				//printf("%s - %s\n", function_name.c_str(), current_function_name.c_str());
				if (current_function_name != function_name)
					continue;

				found = true;
				const auto type = DexDissasembler::CfgType::None;
				DexDissasembler disasm(dex_ir, type);
				disasm.DumpMethod(ir_method.get());
			}

			return found;
		}
	};
} // namespace andromeda

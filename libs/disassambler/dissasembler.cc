/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
    Lasha Khasaia (@_qaz_qaz) 2019
    Small modifications related to printing
*/

#include "dissassembler.h"

#include <stdio.h>
#include <cinttypes>
#include <cmath>
#include <sstream>

#include "color/color.hpp"

// Builds a human readable method declaration, not including the name, ex:
// "(android.content.Context, android.content.pm.ActivityInfo) : java.lang.String"
static std::string MethodDeclaration(const ir::Proto *proto)
{
    std::stringstream ss;
    ss << "(";
    if (proto->param_types != nullptr)
    {
        bool first = true;
        for (auto type : proto->param_types->types)
        {
            ss << (first ? "" : ", ") << type->Decl();
            first = false;
        }
    }
    ss << "):";
    ss << proto->return_type->Decl();
    return ss.str();
}

void PrintCodeIrVisitor::StartInstruction(const lir::Instruction *instr)
{
    if (cfg_ == nullptr || current_block_index_ >= cfg_->basic_blocks.size())
    {
        return;
    }
    const lir::BasicBlock &current_block = cfg_->basic_blocks[current_block_index_];
    if (instr == current_block.region.first)
    {
        printf("............................. begin block %d .............................\n", current_block.id);
    }
}

void PrintCodeIrVisitor::EndInstruction(const lir::Instruction *instr)
{
    if (cfg_ == nullptr || current_block_index_ >= cfg_->basic_blocks.size())
    {
        return;
    }
    const lir::BasicBlock &current_block = cfg_->basic_blocks[current_block_index_];
    if (instr == current_block.region.last)
    {
        printf(".............................. end block %d ..............................\n", current_block.id);
        ++current_block_index_;
    }
}

bool PrintCodeIrVisitor::Visit(lir::Bytecode *bytecode)
{
    StartInstruction(bytecode);
    printf("\t%5u| ", bytecode->offset);
    color::color_printf(color::FG_LIGHT_CYAN, "%s", dex::GetOpcodeName(bytecode->opcode));
    bool first = true;
    for (auto op : bytecode->operands)
    {
        printf(first ? " " : ", ");
        op->Accept(this);
        first = false;
    }
    printf("\n");
    EndInstruction(bytecode);
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::PackedSwitchPayload *packed_switch)
{
    StartInstruction(packed_switch);
    printf("\t%5u| packed-switch-payload\n", packed_switch->offset);
    int key = packed_switch->first_key;
    for (auto target : packed_switch->targets)
    {
        printf("\t\t%5d: ", key++);
        printf("Label_%d", target->id);
        printf("\n");
    }
    EndInstruction(packed_switch);
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::SparseSwitchPayload *sparse_switch)
{
    StartInstruction(sparse_switch);
    printf("\t%5u| sparse-switch-payload\n", sparse_switch->offset);
    for (auto &switchCase : sparse_switch->switch_cases)
    {
        printf("\t\t%5d: ", switchCase.key);
        printf("Label_%d", switchCase.target->id);
        printf("\n");
    }
    EndInstruction(sparse_switch);
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::ArrayData *array_data)
{
    StartInstruction(array_data);
    printf("\t%5u| fill-array-data-payload\n", array_data->offset);
    EndInstruction(array_data);
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::CodeLocation *target)
{
    printf("Label_%d", target->label->id);
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::Const32 *const32)
{
    printf("#%+d (0x%08x | ", const32->u.s4_value, const32->u.u4_value);
    if (std::isnan(const32->u.float_value))
    {
        printf("NaN)");
    }
    else
    {
        printf("%#.6g)", const32->u.float_value);
    }
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::Const64 *const64)
{
    printf("#%+" PRId64 " (0x%016" PRIx64 " | ", const64->u.s8_value, const64->u.u8_value);
    if (std::isnan(const64->u.double_value))
    {
        printf("NaN)");
    }
    else
    {
        printf("%#.6g)", const64->u.double_value);
    }
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::VReg *vreg)
{
    color::color_printf(color::FG_LIGHT_BLUE, "v%d", vreg->reg);
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::VRegPair *vreg_pair)
{
    color::color_printf(color::FG_LIGHT_BLUE, "v%d:v%d", vreg_pair->base_reg, vreg_pair->base_reg + 1);
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::VRegList *vreg_list)
{
    bool first = true;
    printf("{");
    for (auto reg : vreg_list->registers)
    {
        color::color_printf(color::FG_LIGHT_BLUE, "%sv%d", (first ? "" : ","), reg);
        first = false;
    }
    printf("}");
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::VRegRange *vreg_range)
{
    if (vreg_range->count == 0)
    {
        printf("{}");
    }
    else
    {
        printf("{v%d..v%d}", vreg_range->base_reg,
               vreg_range->base_reg + vreg_range->count - 1);
    }
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::String *string)
{
    if (string->ir_string == nullptr)
    {
        printf("<null>");
        return true;
    }
    auto ir_string = string->ir_string;
    printf("\"");
    for (const char *p = ir_string->c_str(); *p != '\0'; ++p)
    {
        if (::isprint(*p))
        {
            printf("%c", *p);
        }
        else
        {
            switch (*p)
            {
            case '\'':
                printf("\\'");
                break;
            case '\"':
                printf("\\\"");
                break;
            case '\?':
                printf("\\?");
                break;
            case '\\':
                printf("\\\\");
                break;
            case '\a':
                printf("\\a");
                break;
            case '\b':
                printf("\\b");
                break;
            case '\f':
                printf("\\f");
                break;
            case '\n':
                printf("\\n");
                break;
            case '\r':
                printf("\\r");
                break;
            case '\t':
                printf("\\t");
                break;
            case '\v':
                printf("\\v");
                break;
            default:
                printf("\\x%02x", *p);
                break;
            }
        }
    }
    printf("\"");
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::Type *type)
{
    SLICER_CHECK(type->index != dex::kNoIndex);
    auto ir_type = type->ir_type;
    printf("%s", ir_type->Decl().c_str());
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::Field *field)
{
    SLICER_CHECK(field->index != dex::kNoIndex);
    auto ir_field = field->ir_field;
    color::color_printf(color::FG_LIGHT_GRAY, "%s.%s", ir_field->parent->Decl().c_str(), ir_field->name->c_str());
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::Method *method)
{
    SLICER_CHECK(method->index != dex::kNoIndex);
    auto ir_method = method->ir_method;
    color::color_printf(color::FG_GREEN, "%s", ir_method->parent->Decl().c_str());
    printf(".");
    color::color_printf(color::FG_LIGHT_YELLOW, "%s%s",
            ir_method->name->c_str(),
            MethodDeclaration(ir_method->prototype).c_str());
    // printf("%s.%s%s",
    //        ir_method->parent->Decl().c_str(),
    //        ir_method->name->c_str(),
    //        MethodDeclaration(ir_method->prototype).c_str());
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::LineNumber *line_number)
{
    printf("%d", line_number->line);
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::Label *label)
{
    StartInstruction(label);
    printf("Label_%d:%s\n", label->id, (label->aligned ? " <aligned>" : ""));
    EndInstruction(label);
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::TryBlockBegin *try_begin)
{
    StartInstruction(try_begin);
    printf("\t.try_begin_%d\n", try_begin->id);
    EndInstruction(try_begin);
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::TryBlockEnd *try_end)
{
    StartInstruction(try_end);
    printf("\t.try_end_%d\n", try_end->try_begin->id);
    for (const auto &handler : try_end->handlers)
    {
        printf("\t  catch(%s) : Label_%d\n", handler.ir_type->Decl().c_str(),
               handler.label->id);
    }
    if (try_end->catch_all != nullptr)
    {
        printf("\t  catch(...) : Label_%d\n", try_end->catch_all->id);
    }
    EndInstruction(try_end);
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::DbgInfoHeader *dbg_header)
{
    StartInstruction(dbg_header);
    printf("\t.params");
    bool first = true;
    for (auto paramName : dbg_header->param_names)
    {
        printf(first ? " " : ", ");
        printf("\"%s\"", paramName ? paramName->c_str() : "?");
        first = false;
    }
    printf("\n");
    EndInstruction(dbg_header);
    return true;
}

bool PrintCodeIrVisitor::Visit(lir::DbgInfoAnnotation *annotation)
{
    StartInstruction(annotation);
    const char *name = ".dbg_???";
    bool skip = false;
    switch (annotation->dbg_opcode)
    {
    case dex::DBG_START_LOCAL:
        name = ".local";
        break;
    case dex::DBG_START_LOCAL_EXTENDED:
        name = ".local_ex";
        break;
    case dex::DBG_END_LOCAL:
        name = ".end_local";
        break;
    case dex::DBG_RESTART_LOCAL:
        name = ".restart_local";
        break;
    case dex::DBG_SET_PROLOGUE_END:
        name = ".prologue_end";
        break;
    case dex::DBG_SET_EPILOGUE_BEGIN:
        name = ".epilogue_begin";
        break;
    case dex::DBG_ADVANCE_LINE:
        name = ".line";
        skip = true;
        break;
    case dex::DBG_SET_FILE:
        name = ".src";
        skip = true;
        break;
    }
    if (!skip)
    {
        printf("\t%s", name);

        bool first = true;
        for (auto op : annotation->operands)
        {
            printf(first ? " " : ", ");
            op->Accept(this);
            first = false;
        }

        printf("\n");
    }
    EndInstruction(annotation);
    return true;
}

void DexDissasembler::DumpAllMethods() const
{
    for (auto &ir_method : dex_ir_->encoded_methods)
    {
        DumpMethod(ir_method.get());
    }
}

void DexDissasembler::DumpMethod(ir::EncodedMethod *ir_method) const
{
    printf("\nmethod %s.%s%s\n{\n",
           ir_method->decl->parent->Decl().c_str(),
           ir_method->decl->name->c_str(),
           MethodDeclaration(ir_method->decl->prototype).c_str());
    Dissasemble(ir_method);
    printf("}\n");
}

void DexDissasembler::Dissasemble(ir::EncodedMethod *ir_method) const
{
    lir::CodeIr code_ir(ir_method, dex_ir_);
    std::unique_ptr<lir::ControlFlowGraph> cfg;
    switch (cfg_type_)
    {
    case CfgType::Compact:
        cfg.reset(new lir::ControlFlowGraph(&code_ir, false));
        break;
    case CfgType::Verbose:
        cfg.reset(new lir::ControlFlowGraph(&code_ir, true));
        break;
    default:
        break;
    }
    PrintCodeIrVisitor visitor(dex_ir_, cfg.get());
    code_ir.Accept(&visitor);
}

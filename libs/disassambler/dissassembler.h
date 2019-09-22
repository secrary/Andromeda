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

#pragma once

#include "slicer/common.h"
#include "slicer/code_ir.h"
#include "slicer/dex_ir.h"
#include "slicer/control_flow_graph.h"

#include <memory>

// Code IR formatting visitor
class PrintCodeIrVisitor : public lir::Visitor
{
public:
    PrintCodeIrVisitor(std::shared_ptr<ir::DexFile> dex_ir, lir::ControlFlowGraph *cfg)
        : dex_ir_(dex_ir), cfg_(cfg) {}

private:
    virtual bool Visit(lir::Bytecode *bytecode) override;
    virtual bool Visit(lir::PackedSwitchPayload *packed_switch) override;
    virtual bool Visit(lir::SparseSwitchPayload *sparse_switch) override;
    virtual bool Visit(lir::ArrayData *array_data) override;
    virtual bool Visit(lir::Label *label) override;
    virtual bool Visit(lir::CodeLocation *location) override;
    virtual bool Visit(lir::Const32 *const32) override;
    virtual bool Visit(lir::Const64 *const64) override;
    virtual bool Visit(lir::VReg *vreg) override;
    virtual bool Visit(lir::VRegPair *vreg_pair) override;
    virtual bool Visit(lir::VRegList *vreg_list) override;
    virtual bool Visit(lir::VRegRange *vreg_range) override;
    virtual bool Visit(lir::String *string) override;
    virtual bool Visit(lir::Type *type) override;
    virtual bool Visit(lir::Field *field) override;
    virtual bool Visit(lir::Method *method) override;
    virtual bool Visit(lir::LineNumber *line) override;
    virtual bool Visit(lir::DbgInfoHeader *dbg_header) override;
    virtual bool Visit(lir::DbgInfoAnnotation *dbg_annotation) override;
    virtual bool Visit(lir::TryBlockBegin *try_begin) override;
    virtual bool Visit(lir::TryBlockEnd *try_end) override;

    void StartInstruction(const lir::Instruction *instr);
    void EndInstruction(const lir::Instruction *instr);

private:
    std::shared_ptr<ir::DexFile> dex_ir_;
    lir::ControlFlowGraph *cfg_ = nullptr;
    size_t current_block_index_ = 0;
};

// A .dex bytecode dissasembler using lir::CodeIr
class DexDissasembler
{
public:
    // The type of CFG (Control Flow Graph) used by the dissasembler:
    //    None    - no CFG, plain listing
    //    Compact - CFG with non-exceptional flow only
    //    Verbose - CFG modeling the EH control flow too
    enum class CfgType
    {
        None,
        Compact,
        Verbose
    };

public:
    explicit DexDissasembler(std::shared_ptr<ir::DexFile> dex_ir, CfgType cfg_type = CfgType::None)
        : dex_ir_(dex_ir), cfg_type_(cfg_type) {}

    DexDissasembler(const DexDissasembler &) = delete;
    DexDissasembler &operator=(const DexDissasembler &) = delete;

    void DumpAllMethods() const;
    void DumpMethod(ir::EncodedMethod *ir_method) const;

private:
    void Dissasemble(ir::EncodedMethod *ir_method) const;

private:
    std::shared_ptr<ir::DexFile> dex_ir_;
    CfgType cfg_type_ = CfgType::None;
};

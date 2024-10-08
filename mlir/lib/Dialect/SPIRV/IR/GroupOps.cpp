//===- GroupOps.cpp - MLIR SPIR-V Group Ops  ------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Defines the group operations in the SPIR-V dialect.
//
//===----------------------------------------------------------------------===//

#include "mlir/Dialect/SPIRV/IR/SPIRVOps.h"
#include "mlir/Dialect/SPIRV/IR/TargetAndABI.h"

#include "SPIRVOpUtils.h"
#include "SPIRVParsingUtils.h"

using namespace mlir::spirv::AttrNames;

namespace mlir::spirv {

template <typename OpTy>
static ParseResult parseGroupNonUniformArithmeticOp(OpAsmParser &parser,
                                                    OperationState &state) {
  spirv::Scope executionScope;
  GroupOperation groupOperation;
  OpAsmParser::UnresolvedOperand valueInfo;
  if (spirv::parseEnumStrAttr<spirv::ScopeAttr>(
          executionScope, parser, state,
          OpTy::getExecutionScopeAttrName(state.name)) ||
      spirv::parseEnumStrAttr<GroupOperationAttr>(
          groupOperation, parser, state,
          OpTy::getGroupOperationAttrName(state.name)) ||
      parser.parseOperand(valueInfo))
    return failure();

  std::optional<OpAsmParser::UnresolvedOperand> clusterSizeInfo;
  if (succeeded(parser.parseOptionalKeyword(kClusterSize))) {
    clusterSizeInfo = OpAsmParser::UnresolvedOperand();
    if (parser.parseLParen() || parser.parseOperand(*clusterSizeInfo) ||
        parser.parseRParen())
      return failure();
  }

  Type resultType;
  if (parser.parseColonType(resultType))
    return failure();

  if (parser.resolveOperand(valueInfo, resultType, state.operands))
    return failure();

  if (clusterSizeInfo) {
    Type i32Type = parser.getBuilder().getIntegerType(32);
    if (parser.resolveOperand(*clusterSizeInfo, i32Type, state.operands))
      return failure();
  }

  return parser.addTypeToList(resultType, state.types);
}

template <typename GroupNonUniformArithmeticOpTy>
static void printGroupNonUniformArithmeticOp(Operation *groupOp,
                                             OpAsmPrinter &printer) {
  printer
      << " \""
      << stringifyScope(
             groupOp
                 ->getAttrOfType<spirv::ScopeAttr>(
                     GroupNonUniformArithmeticOpTy::getExecutionScopeAttrName(
                         groupOp->getName()))
                 .getValue())
      << "\" \""
      << stringifyGroupOperation(
             groupOp
                 ->getAttrOfType<GroupOperationAttr>(
                     GroupNonUniformArithmeticOpTy::getGroupOperationAttrName(
                         groupOp->getName()))
                 .getValue())
      << "\" " << groupOp->getOperand(0);

  if (groupOp->getNumOperands() > 1)
    printer << " " << kClusterSize << '(' << groupOp->getOperand(1) << ')';
  printer << " : " << groupOp->getResult(0).getType();
}

template <typename OpTy>
static LogicalResult verifyGroupNonUniformArithmeticOp(Operation *groupOp) {
  spirv::Scope scope =
      groupOp
          ->getAttrOfType<spirv::ScopeAttr>(
              OpTy::getExecutionScopeAttrName(groupOp->getName()))
          .getValue();
  if (scope != spirv::Scope::Workgroup && scope != spirv::Scope::Subgroup)
    return groupOp->emitOpError(
        "execution scope must be 'Workgroup' or 'Subgroup'");

  GroupOperation operation =
      groupOp
          ->getAttrOfType<GroupOperationAttr>(
              OpTy::getGroupOperationAttrName(groupOp->getName()))
          .getValue();
  if (operation == GroupOperation::ClusteredReduce &&
      groupOp->getNumOperands() == 1)
    return groupOp->emitOpError("cluster size operand must be provided for "
                                "'ClusteredReduce' group operation");
  if (groupOp->getNumOperands() > 1) {
    Operation *sizeOp = groupOp->getOperand(1).getDefiningOp();
    int32_t clusterSize = 0;

    // TODO: support specialization constant here.
    if (failed(extractValueFromConstOp(sizeOp, clusterSize)))
      return groupOp->emitOpError(
          "cluster size operand must come from a constant op");

    if (!llvm::isPowerOf2_32(clusterSize))
      return groupOp->emitOpError(
          "cluster size operand must be a power of two");
  }
  return success();
}

//===----------------------------------------------------------------------===//
// spirv.GroupBroadcast
//===----------------------------------------------------------------------===//

LogicalResult GroupBroadcastOp::verify() {
  spirv::Scope scope = getExecutionScope();
  if (scope != spirv::Scope::Workgroup && scope != spirv::Scope::Subgroup)
    return emitOpError("execution scope must be 'Workgroup' or 'Subgroup'");

  if (auto localIdTy = llvm::dyn_cast<VectorType>(getLocalid().getType()))
    if (localIdTy.getNumElements() != 2 && localIdTy.getNumElements() != 3)
      return emitOpError("localid is a vector and can be with only "
                         " 2 or 3 components, actual number is ")
             << localIdTy.getNumElements();

  return success();
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformBallotOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformBallotOp::verify() {
  spirv::Scope scope = getExecutionScope();
  if (scope != spirv::Scope::Workgroup && scope != spirv::Scope::Subgroup)
    return emitOpError("execution scope must be 'Workgroup' or 'Subgroup'");

  return success();
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformBallotFindLSBOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformBallotFindLSBOp::verify() {
  spirv::Scope scope = getExecutionScope();
  if (scope != spirv::Scope::Workgroup && scope != spirv::Scope::Subgroup)
    return emitOpError("execution scope must be 'Workgroup' or 'Subgroup'");

  return success();
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformBallotFindLSBOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformBallotFindMSBOp::verify() {
  spirv::Scope scope = getExecutionScope();
  if (scope != spirv::Scope::Workgroup && scope != spirv::Scope::Subgroup)
    return emitOpError("execution scope must be 'Workgroup' or 'Subgroup'");

  return success();
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformBroadcast
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformBroadcastOp::verify() {
  spirv::Scope scope = getExecutionScope();
  if (scope != spirv::Scope::Workgroup && scope != spirv::Scope::Subgroup)
    return emitOpError("execution scope must be 'Workgroup' or 'Subgroup'");

  // SPIR-V spec: "Before version 1.5, Id must come from a
  // constant instruction.
  auto targetEnv = spirv::getDefaultTargetEnv(getContext());
  if (auto spirvModule = (*this)->getParentOfType<spirv::ModuleOp>())
    targetEnv = spirv::lookupTargetEnvOrDefault(spirvModule);

  if (targetEnv.getVersion() < spirv::Version::V_1_5) {
    auto *idOp = getId().getDefiningOp();
    if (!idOp || !isa<spirv::ConstantOp,           // for normal constant
                      spirv::ReferenceOfOp>(idOp)) // for spec constant
      return emitOpError("id must be the result of a constant op");
  }

  return success();
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformShuffle*
//===----------------------------------------------------------------------===//

template <typename OpTy>
static LogicalResult verifyGroupNonUniformShuffleOp(OpTy op) {
  spirv::Scope scope = op.getExecutionScope();
  if (scope != spirv::Scope::Workgroup && scope != spirv::Scope::Subgroup)
    return op.emitOpError("execution scope must be 'Workgroup' or 'Subgroup'");

  if (op.getOperands().back().getType().isSignedInteger())
    return op.emitOpError("second operand must be a singless/unsigned integer");

  return success();
}

LogicalResult GroupNonUniformShuffleOp::verify() {
  return verifyGroupNonUniformShuffleOp(*this);
}
LogicalResult GroupNonUniformShuffleDownOp::verify() {
  return verifyGroupNonUniformShuffleOp(*this);
}
LogicalResult GroupNonUniformShuffleUpOp::verify() {
  return verifyGroupNonUniformShuffleOp(*this);
}
LogicalResult GroupNonUniformShuffleXorOp::verify() {
  return verifyGroupNonUniformShuffleOp(*this);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformElectOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformElectOp::verify() {
  spirv::Scope scope = getExecutionScope();
  if (scope != spirv::Scope::Workgroup && scope != spirv::Scope::Subgroup)
    return emitOpError("execution scope must be 'Workgroup' or 'Subgroup'");

  return success();
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformFAddOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformFAddOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformFAddOp>(*this);
}

ParseResult GroupNonUniformFAddOp::parse(OpAsmParser &parser,
                                         OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformFAddOp>(parser,
                                                                 result);
}

void GroupNonUniformFAddOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformFAddOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformFMaxOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformFMaxOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformFMaxOp>(*this);
}

ParseResult GroupNonUniformFMaxOp::parse(OpAsmParser &parser,
                                         OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformFMaxOp>(parser,
                                                                 result);
}

void GroupNonUniformFMaxOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformFMaxOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformFMinOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformFMinOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformFMinOp>(*this);
}

ParseResult GroupNonUniformFMinOp::parse(OpAsmParser &parser,
                                         OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformFMinOp>(parser,
                                                                 result);
}

void GroupNonUniformFMinOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformFMinOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformFMulOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformFMulOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformFMulOp>(*this);
}

ParseResult GroupNonUniformFMulOp::parse(OpAsmParser &parser,
                                         OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformFMulOp>(parser,
                                                                 result);
}

void GroupNonUniformFMulOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformFMulOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformIAddOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformIAddOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformIAddOp>(*this);
}

ParseResult GroupNonUniformIAddOp::parse(OpAsmParser &parser,
                                         OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformIAddOp>(parser,
                                                                 result);
}

void GroupNonUniformIAddOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformIAddOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformIMulOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformIMulOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformIMulOp>(*this);
}

ParseResult GroupNonUniformIMulOp::parse(OpAsmParser &parser,
                                         OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformIMulOp>(parser,
                                                                 result);
}

void GroupNonUniformIMulOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformIMulOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformSMaxOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformSMaxOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformSMaxOp>(*this);
}

ParseResult GroupNonUniformSMaxOp::parse(OpAsmParser &parser,
                                         OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformSMaxOp>(parser,
                                                                 result);
}

void GroupNonUniformSMaxOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformSMaxOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformSMinOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformSMinOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformSMinOp>(*this);
}

ParseResult GroupNonUniformSMinOp::parse(OpAsmParser &parser,
                                         OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformSMinOp>(parser,
                                                                 result);
}

void GroupNonUniformSMinOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformSMinOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformUMaxOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformUMaxOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformUMaxOp>(*this);
}

ParseResult GroupNonUniformUMaxOp::parse(OpAsmParser &parser,
                                         OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformUMaxOp>(parser,
                                                                 result);
}

void GroupNonUniformUMaxOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformUMaxOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformUMinOp
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformUMinOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformUMinOp>(*this);
}

ParseResult GroupNonUniformUMinOp::parse(OpAsmParser &parser,
                                         OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformUMinOp>(parser,
                                                                 result);
}

void GroupNonUniformUMinOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformUMinOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformBitwiseAnd
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformBitwiseAndOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformBitwiseAndOp>(*this);
}

ParseResult GroupNonUniformBitwiseAndOp::parse(OpAsmParser &parser,
                                               OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformBitwiseAndOp>(parser,
                                                                       result);
}

void GroupNonUniformBitwiseAndOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformBitwiseAndOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformBitwiseOr
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformBitwiseOrOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformBitwiseOrOp>(*this);
}

ParseResult GroupNonUniformBitwiseOrOp::parse(OpAsmParser &parser,
                                              OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformBitwiseOrOp>(parser,
                                                                      result);
}

void GroupNonUniformBitwiseOrOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformBitwiseOrOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformBitwiseXor
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformBitwiseXorOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformBitwiseXorOp>(*this);
}

ParseResult GroupNonUniformBitwiseXorOp::parse(OpAsmParser &parser,
                                               OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformBitwiseXorOp>(parser,
                                                                       result);
}

void GroupNonUniformBitwiseXorOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformBitwiseXorOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformLogicalAnd
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformLogicalAndOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformLogicalAndOp>(*this);
}

ParseResult GroupNonUniformLogicalAndOp::parse(OpAsmParser &parser,
                                               OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformLogicalAndOp>(parser,
                                                                       result);
}

void GroupNonUniformLogicalAndOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformLogicalAndOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformLogicalOr
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformLogicalOrOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformLogicalOrOp>(*this);
}

ParseResult GroupNonUniformLogicalOrOp::parse(OpAsmParser &parser,
                                              OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformLogicalOrOp>(parser,
                                                                      result);
}

void GroupNonUniformLogicalOrOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformLogicalOrOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// spirv.GroupNonUniformLogicalXor
//===----------------------------------------------------------------------===//

LogicalResult GroupNonUniformLogicalXorOp::verify() {
  return verifyGroupNonUniformArithmeticOp<GroupNonUniformLogicalXorOp>(*this);
}

ParseResult GroupNonUniformLogicalXorOp::parse(OpAsmParser &parser,
                                               OperationState &result) {
  return parseGroupNonUniformArithmeticOp<GroupNonUniformLogicalXorOp>(parser,
                                                                       result);
}

void GroupNonUniformLogicalXorOp::print(OpAsmPrinter &p) {
  printGroupNonUniformArithmeticOp<GroupNonUniformLogicalXorOp>(*this, p);
}

//===----------------------------------------------------------------------===//
// Group op verification
//===----------------------------------------------------------------------===//

template <typename Op>
static LogicalResult verifyGroupOp(Op op) {
  spirv::Scope scope = op.getExecutionScope();
  if (scope != spirv::Scope::Workgroup && scope != spirv::Scope::Subgroup)
    return op.emitOpError("execution scope must be 'Workgroup' or 'Subgroup'");

  return success();
}

LogicalResult GroupIAddOp::verify() { return verifyGroupOp(*this); }

LogicalResult GroupFAddOp::verify() { return verifyGroupOp(*this); }

LogicalResult GroupFMinOp::verify() { return verifyGroupOp(*this); }

LogicalResult GroupUMinOp::verify() { return verifyGroupOp(*this); }

LogicalResult GroupSMinOp::verify() { return verifyGroupOp(*this); }

LogicalResult GroupFMaxOp::verify() { return verifyGroupOp(*this); }

LogicalResult GroupUMaxOp::verify() { return verifyGroupOp(*this); }

LogicalResult GroupSMaxOp::verify() { return verifyGroupOp(*this); }

LogicalResult GroupIMulKHROp::verify() { return verifyGroupOp(*this); }

LogicalResult GroupFMulKHROp::verify() { return verifyGroupOp(*this); }

} // namespace mlir::spirv

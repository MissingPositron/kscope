#include "IRRenderer.h"

#include "kscope/AST/BinaryNode.h"
#include "kscope/AST/VariableNode.h"
#include "kscope/Errors/Error.h"

#include "llvm/IR/Value.h"
#include "llvm/IR/Type.h"


llvm::Value *
IRRenderer::render_node(BinaryNode *node) {
  auto &context = get_render_context();
  auto &builder = context.get_builder();

  if ( node->op == '=' ) {
    auto *lhse = static_cast<VariableNode*>(node->lhs);
    if ( ! lhse ) {
      return Error<llvm::Value>::handle("destination of '=' must be a variable");
    }

    auto *val = render(node->rhs);
    if ( val == 0 ) { return nullptr; }

    auto *variable = context.get_named_value(lhse->getName());
    if ( variable == 0 ) {
      return Error<llvm::Value>::handle("Unknown variable name");
    }

    builder.CreateStore(val, variable);

    return val;
  }

  auto *left = render(node->lhs);
  auto *right = render(node->rhs);

  if (left == 0 || right == 0 ) { return nullptr; }

  auto *llvm_double_type = llvm::Type::getDoubleTy(context.get_llvm_context());

  switch (node->op) {
  case '+':
    return builder.CreateFAdd(left, right, "addtmp");
  case '-':
    return builder.CreateFSub(left, right, "subtmp");
  case '*':
    return builder.CreateFMul(left, right, "multmp");
  case '<':
    left = builder.CreateFCmpULT(left, right, "cmptmp");
    return builder.CreateUIToFP(left,
                                llvm_double_type,
                                "booltmp");
  case '>':
    right = builder.CreateFCmpULT(right, left, "cmptmp");
    return builder.CreateUIToFP(right,
                                llvm_double_type,
                                "booltmp");
  default: break;
  }

  return Error<llvm::Value>::handle("Unknown binary operator!");
}
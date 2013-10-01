#include "llvm/IR/Function.h"
#include "llvm/IR/Value.h"

#include "ast/call.h"
#include "renderer.h"
#include "errors.h"

#include <vector>

llvm::Value *
CallNode::codegen(IRRenderer *renderer) {
    llvm::Function *callee_func = renderer->module->getFunction(callee);
    if ( callee_func == 0 ) {
        return ErrorV("Unknown function referenced");
    }

    if ( callee_func->arg_size() != args.size() ) {
        return ErrorV("Incorrect number of arguments passed");
    }

    std::vector<llvm::Value*> arg_values;
    for ( unsigned i = 0, num_args = args.size(); i < num_args; ++i ) {
        arg_values.push_back(args[i]->codegen(context));
        if ( arg_values.back() == 0 ) { return 0; }
    }

    return renderer->builder->CreateCall(callee_func, arg_values, "calltmp");
}

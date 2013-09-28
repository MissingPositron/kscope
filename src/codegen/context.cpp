#include "llvm/Analysis/Passes.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/PassManager.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Scalar.h"

#include <string>

#include "context.h"

using ::llvm::AllocaInst;
using ::llvm::CloneModule;
using ::llvm::DataLayout;
using ::llvm::ExecutionEngine;
using ::llvm::Function;
using ::llvm::FunctionPassManager;
using ::llvm::IRBuilder;
using ::llvm::LLVMContext;
using ::llvm::Module;
using ::llvm::Type;


Context::Context()
    : Context(new Module("my cool jit", llvm::getGlobalContext())) {}

Context::Context(const Context &other)
    : Context(CloneModule(other.module.get())) {}

Context::Context(Module *module)
    : module(unique_ptr<Module>(module)),
      engine(unique_ptr<ExecutionEngine>(ExecutionEngine::createJIT(module))),
      builder(unique_ptr<IRBuilder<> >(new IRBuilder<>(module->getContext()))),
      pass_manager(unique_ptr<FunctionPassManager>(new FunctionPassManager(module))) {

    pass_manager->add(new DataLayout(*engine->getDataLayout()));
    pass_manager->add(llvm::createBasicAliasAnalysisPass());
    pass_manager->add(llvm::createPromoteMemoryToRegisterPass());
    pass_manager->add(llvm::createInstructionCombiningPass());
    pass_manager->add(llvm::createReassociatePass());
    pass_manager->add(llvm::createGVNPass());
    pass_manager->add(llvm::createCFGSimplificationPass());

    pass_manager->doInitialization();
}

Context::Context(Context &&other) {
    module = std::move(other.module);
    engine = std::move(other.engine);
    builder = std::move(other.builder);
    pass_manager = std::move(other.pass_manager);
    other.module = nullptr;
    other.engine = nullptr;
    other.builder = nullptr;
    other.pass_manager = nullptr;
}

Context &
Context::operator =(Context other) {
    std::swap(module, other.module);
    std::swap(engine, other.engine);
    std::swap(builder, other.builder);
    std::swap(pass_manager, other.pass_manager);
    return *this;
}

Context::~Context() {
    pass_manager.reset();
    builder.reset();
    engine.reset();
    module.reset();
}

llvm::LLVMContext &
Context::llvm_context() { return module->getContext(); }

llvm::AllocaInst *
Context::get_named_value (const std::string &name){
    return named_values[name];
}

void
Context::set_named_value(const std::string &name, llvm::AllocaInst *value) {
    named_values[name] = value;
}

void
Context::clear_named_value(const std::string &name) {
    named_values.erase(name);
}

void
Context::clear_all_named_values() {
    named_values.clear();
}

AllocaInst *
Context::create_entry_block_alloca(Function *func, const std::string &name) {
    IRBuilder<> tmp_builder(&func->getEntryBlock(),
                            func->getEntryBlock().begin());

    return tmp_builder.CreateAlloca(Type::getDoubleTy(module->getContext()),
                                    0,
                                    name.c_str());
}

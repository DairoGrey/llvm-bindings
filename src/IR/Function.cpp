#include "IR/Function.h"
#include "IR/GlobalObject.h"
#include "IR/FunctionType.h"
#include "IR/Module.h"
#include "Util/Inherit.h"

void Function::Init(Napi::Env env, Napi::Object &exports) {
    Napi::HandleScope scope(env);
    Napi::Function func = DefineClass(env, "Function", {
            StaticMethod("Create", &Function::create)
    });
    constructor = Napi::Persistent(func);
    constructor.SuppressDestruct();
    Inherit(env, constructor.Value(), GlobalObject::constructor.Value());
    exports.Set("Function", func);
}

Napi::Object Function::New(Napi::Env env, llvm::Function *function) {
    return constructor.New({Napi::External<llvm::Function>::New(env, function)});
}

bool Function::IsClassOf(const Napi::Value &value) {
    return value.As<Napi::Object>().InstanceOf(constructor.Value());
}

llvm::Function *Function::Extract(const Napi::Value &value) {
    return Unwrap(value.As<Napi::Object>())->getLLVMPrimitive();
}

Function::Function(const Napi::CallbackInfo &info) : ObjectWrap(info) {
    Napi::Env env = info.Env();
    if (!info.IsConstructCall()) {
        throw Napi::TypeError::New(env, "Constructor needs to be called with new");
    }
    if (info.Length() < 1 || !info[0].IsExternal()) {
        throw Napi::TypeError::New(env, "Expected function pointer");
    }
    auto external = info[0].As<Napi::External<llvm::Function>>();
    function = external.Data();
}

Napi::Value Function::create(const Napi::CallbackInfo &info) {
    Napi::Env env = info.Env();
    int argsLen = info.Length();
    const std::string &errMsg = "Function.Create needs to be called with: (funcType: FunctionType, linkage: LinkageTypes, name: string, module: Module)";
    if (argsLen < 2 || !FunctionType::IsClassOf(info[0]) || !info[1].IsNumber()) {
        throw Napi::TypeError::New(env, errMsg);
    }
    llvm::FunctionType *funcType = FunctionType::Extract(info[0]);
    llvm::GlobalValue::LinkageTypes linkage = static_cast<llvm::GlobalValue::LinkageTypes>(info[1].As<Napi::Number>().Uint32Value());
    std::string name;
    llvm::Module *module = nullptr;
    if (argsLen >= 3) {
        if (!info[2].IsString()) {
            throw Napi::TypeError::New(env, errMsg);
        }
        name = info[2].As<Napi::String>();
    }
    if (argsLen >= 4) {
        if (!Module::IsClassOf(info[3])) {
            throw Napi::TypeError::New(env, errMsg);
        }
        module = Module::Extract(info[3]);
    }
    llvm::Function *function = llvm::Function::Create(funcType, linkage, name, module);
    return Function::New(env, function);
}

llvm::Function *Function::getLLVMPrimitive() {
    return function;
}

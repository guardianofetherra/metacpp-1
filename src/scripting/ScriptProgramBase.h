#ifndef SCRIPTPROGRAMBASE_H
#define SCRIPTPROGRAMBASE_H
#include "config.h"
#include <iostream>
#include <memory>
#include <StringBase.h>
#include <Variant.h>

namespace metacpp {
namespace scripting {

class ScriptThreadBase;

class ScriptProgramBase {
public:
    ScriptProgramBase();
    virtual ~ScriptProgramBase();

    SharedObjectPointer<ScriptThreadBase> createThread(const String& functionName = String(),
                                                  const VariantArray& args = VariantArray());

    template<typename... TArgs>
    SharedObjectPointer<ScriptThreadBase> createThread(const String& functionName,
                                                   TArgs... args)
    {
        return createThread(functionName, {args...});
    }

public:
    virtual void compile(std::istream& is, const String& filename);

    virtual void compile(const String& sourceFile);
    virtual void compile(const void *pBuffer, size_t size, const String& fileName) = 0;
protected:
    virtual ScriptThreadBase *createThreadImpl(const String& functionName,
                                           const VariantArray& args = VariantArray()) = 0;
    virtual void closeThreadImpl(ScriptThreadBase *thread) = 0;

};

} // namespace scripting
} // namespace metacpp

#endif // SCRIPTPROGRAMBASE_H

#include "ScriptThreadBase.h"
#include <thread>

namespace metacpp {
namespace scripting {

ScriptThreadBase::ScriptThreadBase() {
}

ScriptThreadBase::~ScriptThreadBase() {
}

bool ScriptThreadBase::runAsync(const std::function<void (const Variant &)> &onFinished, const std::function<void (const std::__exception_ptr::exception_ptr)> &onError)
{
    std::thread th([&]{
        try
        {
            onFinished(this->run());
        }
        catch (...)
        {
            onError(std::current_exception());
        }
    });
    th.detach();

    return true;
}

const char *TerminationException::what() const noexcept
{
    return "Terminated";
}

} // namespace scripting
} // namespace metacpp
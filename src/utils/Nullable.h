/****************************************************************************
* Copyright 2014-2015 Trefilov Dmitrij                                      *
*                                                                           *
* Licensed under the Apache License, Version 2.0 (the "License");           *
* you may not use this file except in compliance with the License.          *
* You may obtain a copy of the License at                                   *
*                                                                           *
*    http://www.apache.org/licenses/LICENSE-2.0                             *
*                                                                           *
* Unless required by applicable law or agreed to in writing, software       *
* distributed under the License is distributed on an "AS IS" BASIS,         *
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
* See the License for the specific language governing permissions and       *
* limitations under the License.                                            *
****************************************************************************/
#ifndef NULLABLE_H
#define NULLABLE_H
#include <stdexcept>
#include <type_traits>

template<typename T/*, typename = typename std::enable_if<std::is_pod<T>::type>::value*/>
class Nullable
{
public:
    Nullable() : m_isSet(false), m_value() {}
    Nullable(const T& value) : m_isSet(true), m_value(value) {}
    Nullable(const Nullable& other) : m_isSet(other.m_isSet), m_value(other.m_value) {}

    Nullable& operator=(const Nullable& other)
    {
        m_isSet = other.m_isSet;
        m_value = other.m_value;
        return *this;
    }

    T& operator=(const T& value) { set(value); return m_value; }
    bool operator==(const T& value) { return m_isSet && value == m_value; }
    const T& operator *() const { return get(); }
    T& operator *() { return get(); }
    operator bool() const { return m_isSet; }
    bool isSet() const { return m_isSet; }

    const T& get() const
    {
        if (m_isSet) return m_value;
        throw std::logic_error("Value of Nullable is not set.");
    }

    T& get()
    {
        if (m_isSet) return m_value;
        throw std::logic_error("Value of Nullable is not set.");
    }

    void reset(bool set = false) { m_isSet = set; }

    const T& take(const T& _default = T()) const { return m_isSet ? get() : _default; }

private:
    void set(const T& value) { m_value = value; m_isSet = true; }

private:
    bool m_isSet;
    T m_value;
};

#endif // NULLABLE_H

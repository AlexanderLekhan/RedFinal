#ifndef UNITTESTS_H_
#define UNITTESTS_H_

#include <sstream>
#include <iostream>
#include <exception>
#include <string>
#include <set>
#include <map>
#include <vector>

//===========================================================================//
// Declarations
//---------------------------------------------------------------------------//
struct FileLineFunc
{
    FileLineFunc(const std::string& _file = {},
                 unsigned _line = 0,
                 const std::string& _func = {})
        : file(_file), line(_line), func(_func)
    {
    }
    const std::string& file;
    unsigned line;
    const std::string& func;
};

//===========================================================================//
template <class T>
std::ostream& operator << (std::ostream& os, const std::set<T>& s);

//---------------------------------------------------------------------------//
template <class K, class V>
std::ostream& operator << (std::ostream& os, const std::map<K, V>& m);

//---------------------------------------------------------------------------//
std::ostream& operator << (std::ostream& os, const FileLineFunc& flf);

//===========================================================================//
// Assertion functions
//---------------------------------------------------------------------------//
template<class T, class U>
void AssertEqual(const T& t,
                 const U& u,
                 const std::string& lharg = {},
                 const std::string& rharg = {},
                 const FileLineFunc& flf = {},
                 const std::string& hint = {});

//---------------------------------------------------------------------------//
void Assert(bool b,
            const std::string& expr = {},
            const FileLineFunc& flf = FileLineFunc(),
            const std::string& hint = {});

//---------------------------------------------------------------------------//
#define ASSERT_EQUAL_HINT(t, u, hint) \
    AssertEqual(t, u, #t, #u, {__FILE__, __LINE__, __func__}, hint);

//---------------------------------------------------------------------------//
#define ASSERT_EQUAL(t, u) \
    AssertEqual(t, u, #t, #u, {__FILE__, __LINE__, ""}, "");
    //AssertEqual(t, u, #t, #u, __FILE__, __LINE__, __func__, "");

//---------------------------------------------------------------------------//
#define ASSERT_HINT(t, hint) \
    Assert(t, #t, {__FILE__, __LINE__, ""}, hint);
    //Assert(t, #t, __FILE__, __LINE__, __func__, hint);

//---------------------------------------------------------------------------//
#define ASSERT(t) \
    Assert(t, #t, {__FILE__, __LINE__, ""}, "");

//===========================================================================//
class TestRunner
{
public:
    ~TestRunner();
    template <class TestFunc>
    void RunTest(TestFunc func, const std::string& test_name);

private:
    int fail_count = 0;
};

//===========================================================================//
// Definitions
//===========================================================================//
class SmartDelim
{
public:
    SmartDelim(const std::string& delim)
        : m_delim(delim)
    {
    }
    SmartDelim& operator()(const std::string& newDelim)
    {
        m_delim = newDelim;
        return *this;
    }
private:
    friend std::ostream& operator << (std::ostream& os, SmartDelim& sd);
    bool m_first = true;
    std::string m_delim;
};

//===========================================================================//
template <class T>
std::ostream& operator << (std::ostream& os, const std::set<T>& s)
{
    SmartDelim comma(", ");
    os << "{";

    for (const auto& x : s)
    {
        os << comma << x;
    }
    return os << "}";
}

//---------------------------------------------------------------------------//
template <class T>
std::ostream& operator << (std::ostream& os, const std::vector<T>& s)
{
    SmartDelim comma(", ");
    os << "{";

    for (const auto& x : s)
    {
        os << comma << x;
    }
    return os << "}";
}

//---------------------------------------------------------------------------//
template <class K, class V>
std::ostream& operator << (std::ostream& os, const std::map<K, V>& m)
{
    SmartDelim comma(", ");
    os << "{";

    for (const auto& kv : m)
    {
        os << comma << kv.first << ": " << kv.second;
    }
    return os << "}";
}

//===========================================================================//
template<class T, class U>
void AssertEqual(
        const T& t,
        const U& u,
        const std::string& lharg,
        const std::string& rharg,
        const FileLineFunc& flf,
        const std::string& hint)
{
    if (!(t == u))
    {
        std::ostringstream os;
        os << flf;
        if (!os.str().empty())
            os << ". ";

        os << "Assertion: (" << lharg << " == " << rharg
           //<< ") failed.";
           << ") failed: " << t << " != " << u;

        if (!hint.empty())
           os << " hint: " << hint;

        throw std::runtime_error(os.str());
    }
}

//===========================================================================//
template <class TestFunc>
void TestRunner::RunTest(TestFunc func, const std::string& test_name)
{
    try
    {
        func();
        std::cerr << test_name << " OK" << std::endl;
    }
    catch (std::runtime_error& e)
    {
        ++fail_count;
        std::cerr << test_name << " fail. " << e.what() << std::endl;
    }
}

//---------------------------------------------------------------------------//
#define RUN_TEST(testRunner, testFunction) \
        testRunner.RunTest(testFunction, #testFunction);

#endif /* UNITTESTS_H_ */

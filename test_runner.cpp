#include "test_runner.h"

using namespace std;

//---------------------------------------------------------------------------//
ostream& operator << (ostream& os, SmartDelim& sd)
{
    if (sd.m_first)
        sd.m_first = false;
    else
        os << sd.m_delim;
    return os;
}
//---------------------------------------------------------------------------//
ostream& operator << (ostream& os, const FileLineFunc& flf)
{
    SmartDelim sep(", ");

    if (!flf.file.empty())
        os << sep << "File: " << flf.file;

    if (flf.line > 0)
       os << sep << "Line: " << flf.line;

    if (!flf.func.empty())
       os << sep << "Function: " << flf.func;

    return os;
}

//---------------------------------------------------------------------------//
void Assert(bool b,
            const string& expr,
            const FileLineFunc& flf,
            const string& hint)
{
    if (!b)
    {
        ostringstream os;
        os << flf;
        if (!os.str().empty())
            os << ". ";

        os << "Assertion failed: (" << expr << ")";

        if (!hint.empty())
           os << " hint: " << hint;

        throw runtime_error(os.str());
    }
}

//---------------------------------------------------------------------------//
TestRunner::~TestRunner()
{
    if (fail_count > 0)
    {
        cerr << fail_count << " unit tests failed. Terminate" << endl;
        exit(1);
    }
}

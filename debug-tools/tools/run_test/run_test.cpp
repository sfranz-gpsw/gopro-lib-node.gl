#include <cstdio>
#include <Python.h>
#include <vector>
#include <string>
#ifdef _WIN32
#define setenv(name, value, p) _putenv_s(name, value)
#define NGL_TEST L"../nodegl-env/Scripts/ngl-test-script.py"
#include <direct.h>
#else
#define NGL_TEST L"../nodegl-env/bin/ngl-test"
#endif
using namespace std;

void py_cmd(vector<wstring> args) {
    vector<wchar_t*> wargs(args.size());
    for (uint32_t j = 0; j<args.size(); j++) wargs[j] = args[j].data();
    Py_Initialize();
    Py_Main(wargs.size(), wargs.data());
    Py_Finalize();
}
wstring to_wstring(char* s) {
    string str = s;
    return wstring(str.begin(), str.end());
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "ERROR: usage run_test <backend> <test>\n"
                "Example: run_test ngfx blending_all_diamond\n");
        return 1;
    }
    chdir(TESTS_DIR);
    setenv("PYTHONPATH", "../pynodegl:../pynodegl-utils:../nodegl-env/lib/python3.7/site-packages", 1);
    setenv("BACKEND", argv[1], 1);
    wstring test_name = to_wstring(argv[2]);
    wstring test_file = test_name.substr(0, test_name.find(L'_')) + L".py";
    vector<wstring> wargs = { to_wstring(argv[0]), NGL_TEST, test_file, test_name, L"refs/" + test_name + L".ref" };
    py_cmd(wargs);
    return 0;
}

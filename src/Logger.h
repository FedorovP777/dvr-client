// This is a personal academic project. Dear PVS-Studio, please check it.

// PVS-Studio Static Code Analyzer for C, C++, C#, and Java:
// https://pvs-studio.com

#ifndef LIBAV_TEST_LOGGER_H
#define LIBAV_TEST_LOGGER_H

#define LOG(msg) std::cout << __FILE__ << ":" << __LINE__ << " " << msg << endl;
#define LOG_ERR(msg) std::cerr << __FILE__ << ":" << __LINE__ << " " << msg << endl;

using namespace std;

#include <ostream>
#include <string>

class Logger
{
public:
    explicit Logger(ostream & output_stream) : os(output_stream)
    {
        this->file.clear();
        this->line = 0;
    }

    void SetLogLine(bool value) { log_line = value; }

    void SetLogFile(bool value) { log_file = value; }

    void SetFile(const string & filename) { file = filename; }

    void SetLine(int line_number) { line = line_number; }

    void Log(const string & message)
    {
        os << file << ':' << line << ' ' << message << '\n';
        ;
    }

private:
    ostream & os;
    bool log_line = false;
    bool log_file = false;
    string file;
    int line;
};

#endif // LIBAV_TEST_LOGGER_H
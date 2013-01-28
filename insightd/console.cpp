#include "console.h"
#include <cstdio>

// static variables
QFile Console::_stdin;
QFile Console::_stdout;
QFile Console::_stderr;
QTextStream Console::_out;
QTextStream Console::_err;
//QDataStream Console::_bin;
//QDataStream Console::_ret;
bool Console::_interrupted = false;
bool Console::_interactive = false;
InputHandlerYesNoQuestion Console::_hdlrYesNoQuestion = 0;
ColorPalette Console::_colors;


void Console::initConsoleInOut()
{
    // Open the console devices
    if (!_stdin.isOpen())
        _stdin.open(stdin, QIODevice::ReadOnly);
    if (!_stdout.isOpen())
        _stdout.open(stdout, QIODevice::WriteOnly);
    if (!_stderr.isOpen())
        _stderr.open(stderr, QIODevice::WriteOnly);

    _out.setDevice(&_stdout);
    _err.setDevice(&_stderr);
}


void Console::errMsg(const QString &msg, bool newline)
{
    _err << color(ctErrorLight) << msg << color(ctReset);
    if (newline)
        _err << endl;
    else
        _err << flush;
}


void Console::errMsg(const QString &hdr, const QString &msg, bool newline)
{
    _err << color(ctErrorLight) << hdr << ":" << color(ctReset)
         << " " << color(ctError) << msg << color(ctReset);
    if (newline)
        _err << endl;
    else
        _err << flush;
}


void Console::errMsg(const char *s, bool newline)
{
    _err << color(ctErrorLight) << s << color(ctReset);
    if (newline)
        _err << endl;
    else
        _err << flush;
}


void Console::warnMsg(const QString &msg, bool newline)
{
    _err << color(ctWarning) << msg << color(ctReset);
    if (newline)
        _err << endl;
    else
        _err << flush;
}


void Console::warnMsg(const QString &hdr, const QString &msg, bool newline)
{
    _err << color(ctWarningLight) << hdr << ":" << color(ctReset)
         << " " << color(ctWarning) << msg << color(ctReset);
    if (newline)
        _err << endl;
    else
        _err << flush;
}


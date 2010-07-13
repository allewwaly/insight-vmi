/*
 * memconstparser.cpp
 *
 *  Created on: 21.06.2010
 *      Author: chrschn
 */

#include "memspecparser.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegExp>
#include "debug.h"

const char* memspec_src =
    "#include <linux/kernel.h>\n"
    "#include <linux/module.h>\n"
    "#include <linux/mm.h>\n"
    "\n"
    "// cleanup some symbols\n"
    "#undef __always_inline\n"
    "\n"
    "#include <stdio.h>\n"
    "\n"
    "// As long as there is no VMEMMAP_END defined, we have to define\n"
    "// it ourselves (see Documentation/x86_64/mm.txt)\n"
    "#if defined(VMEMMAP_START) && ! defined(VMEMMAP_END)\n"
    "#define VMEMMAP_BITS 40\n"
    "#define VMEMMAP_END (VMEMMAP_START | ((1UL << VMEMMAP_BITS)-1))\n"
    "#endif\n"
    "\n"
    "#ifdef CONFIG_X86_64\n"
    "#define ARCHITECTURE \"x86_64\"\n"
    "#else\n"
    "#define ARCHITECTURE \"i386\"\n"
    "#endif\n"
    "\n"
    "int init_module(void) { return 0; }\n"
    "\n"
    "void cleanup_module(void) {}\n"
    "\n"
    "int main() {\n"
    "  // Define potentially unresolved symbols\n"
    "  unsigned long __FIXADDR_TOP = 0;\n"
    "  unsigned long high_memory = 0;\n"
    "\n"
    "%MAIN_BODY%"       // this placeholder gets replaced later on
    "  return 0;\n"
    "}\n";

const char* memspec_makefile =
    "EXTRA_CFLAGS += -I/usr/include\n"
    "KDIR := %KDIR%\n"  // this placeholder gets replaced later on
    "MDIR := %MDIR%\n"  // this placeholder gets replaced later on
    "\n"
    "obj-m += memspec.o\n"
    "\n"
    "all:\n"
    "\tmake -C $(KDIR) M=$(MDIR) modules\n"
    "\n"
    "clean:\n"
    "\tmake -C $(KDIR) M=$(MDIR) clean\n"
    "\t@rm -fv memspec\n"
    "\n"
    "memspec: memspec.o\n"
    //      Just try both 64 bit and 32 bit cross-compilation
    "\tgcc -m64 -o memspec memspec.o || gcc -m32 -o memspec memspec.o\n";


MemSpecParser::MemSpecParser(const QString& kernelSrcDir, const QString& systemMapFile)
    : _autoRemoveBuildDir(true), _kernelSrcDir(kernelSrcDir), _systemMapFile(systemMapFile)
{
}


MemSpecParser::~MemSpecParser()
{
    if (_autoRemoveBuildDir && !_buildDir.isEmpty()) {
        rmdirRek(_buildDir);
    }
}

QString MemSpecParser::buildDir() const
{
    return _buildDir;
}


bool MemSpecParser::rmdirRek(const QString& dir) const
{
    bool ret = true;

    QDir d(dir);
    QFileInfoList entries = d.entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::System|QDir::Hidden);

    // Delete all entries recursively
    for (int i = 0; ret && i < entries.size(); ++i) {
        // Just to make this bullet proof
        if (d.absolutePath() != entries[i].absolutePath())
            continue;
        if (entries[i].isDir())
            // Recures into sub-directories
            ret = ret && rmdirRek(entries[i].absoluteFilePath());
        else
            ret = ret && d.remove(entries[i].fileName());
    }

    // Finally, remove the (hopefully empty) directory
    QString dirName = d.dirName();
    ret = ret && d.cdUp();

    return ret && d.rmdir(dirName);
}


bool MemSpecParser::autoRemoveBuildDir() const
{
    return _autoRemoveBuildDir;
}


void MemSpecParser::setAutoRemoveBuildDir(bool autoRemove)
{
    _autoRemoveBuildDir = autoRemove;
}


const QByteArray& MemSpecParser::errorOutput() const
{
    return _errorOutput;
}


void MemSpecParser::setupBuildDir()
{
    char buf[] = "/tmp/memtool.XXXXXX";
    if (!mkdtemp(buf)) {
        int e = errno;
        memSpecParserError(QString("Error creating temporary directory: %1").arg(strerror(e)));
    }
    _buildDir = QString(buf);

    // Prepare the source file
    QString src;
    KernelMemSpecList list = MemSpecs::supportedMemSpecs();
    for (int i = 0; i < list.size(); ++i) {
        if (list[i].macroCond.isEmpty())
            src += QString("  printf(\"%1 = %3\\n\", (%2));\n")
                        .arg(list[i].keyFmt)
                        .arg(list[i].valueFmt)
                        .arg(list[i].outputFmt);
        else
            src += QString("#if %0\n  printf(\"%1 = %3\\n\", (%2));\n#endif\n")
                        .arg(list[i].macroCond)
                        .arg(list[i].keyFmt)
                        .arg(list[i].valueFmt)
                        .arg(list[i].outputFmt);
    }
    src = QString(memspec_src).replace("%MAIN_BODY%", src);

    QFile srcfile(_buildDir + "/memspec.c");
    if (!srcfile.open(QIODevice::WriteOnly))
        memSpecParserError(QString("Cannot open file \"%1\" for writing.").arg(srcfile.fileName()));
    srcfile.write(src.toAscii());
    srcfile.close();

    // Prepare the make file
    QString makesrc = QString(memspec_makefile)
            .replace("%KDIR%", _kernelSrcDir)
            .replace("%MDIR%", _buildDir);


    QFile makefile(_buildDir + "/Makefile");
    if (!makefile.open(QIODevice::WriteOnly))
        memSpecParserError(QString("Cannot open file \"%1\" for writing.").arg(makefile.fileName()));
    makefile.write(makesrc.toAscii());
    makefile.close();
}


void MemSpecParser::buildHelperProg()
{
    QProcess proc;
    QStringList cmdlines;
    cmdlines += QString("make KDIR=%1").arg(_kernelSrcDir);
    cmdlines += QString("make memspec");

    for (int i = 0; i < cmdlines.size(); ++i) {
        // Invoke the command
        QStringList args = cmdlines[i].split(QChar(' '));
        QString cmd = args.front();
        args.pop_front();

        proc.setWorkingDirectory(_buildDir);
        proc.start(cmd, args);
        proc.waitForFinished(-1);

        if (proc.exitCode() != 0) {
            _errorOutput = proc.readAllStandardError();
            memSpecParserError(
                    QString("Command failed with exit code %3: %1 %2")
                        .arg(cmd)
                        .arg(args.join(" "))
                        .arg(proc.exitCode()));
        }

        proc.close();
    }
}


void MemSpecParser::parseHelperProg(MemSpecs* specs)
{
    QProcess proc;
    proc.setWorkingDirectory(_buildDir);

    // Invoke the command
    QString cmd = "./memspec";

    proc.start(cmd);
    proc.waitForFinished(-1);

    if (proc.exitCode() != 0) {
        _errorOutput = proc.readAllStandardError();
        memSpecParserError(
                QString("Command failed with exit code %2: %1")
                    .arg(cmd)
                    .arg(proc.exitCode()));
    }

    const int bufsize = 1024;
    char buf[1024];

    QRegExp re("^\\s*([^\\s]+)\\s*=\\s*(.*)$");
    while (!proc.atEnd() && proc.readLine(buf, bufsize) > 0) {
        if (re.exactMatch(buf))
            // Update the given MemSpecs object with the parsed key-value pair
            specs->setFromKeyValue(re.cap(1), re.cap(2));
    }
}


void MemSpecParser::parseSystemMap(MemSpecs* specs)
{
    const int bufsize = 1024;
    char buf[1024];

    QFile sysMap(_systemMapFile);
    if (!sysMap.open(QIODevice::ReadOnly))
        memSpecParserError(QString("Cannot open file \"%1\" for reading.").arg(_systemMapFile));

    bool lvl4_ok = false, swp_ok = false;
    QRegExp re("^\\s*([0-9a-fA-F]+)\\s+.\\s+(.*)$");

    while (!lvl4_ok && !sysMap.atEnd() && sysMap.readLine(buf, bufsize) > 0) {
        QString line(buf);
        if (line.contains("init_level4_pgt") && re.exactMatch(line))
            // Update the given MemSpecs object with the parsed key-value pair
            specs->initLevel4Pgt = re.cap(1).toULong(&lvl4_ok, 16);
        else if (line.contains("swapper_pg_dir") && re.exactMatch(line))
            // Update the given MemSpecs object with the parsed key-value pair
            specs->swapperPgDir = re.cap(1).toULong(&swp_ok, 16);
    }

    sysMap.close();

    // We expect to parse exactly one of them
    if ( (!lvl4_ok && !swp_ok) || (lvl4_ok && swp_ok) ) {
        memSpecParserError(QString("Could not parse one of the required values "
                "\"init_level4_pgt\" or \"swapper_pg_dir\" from file \"%1\"")
                .arg(_systemMapFile));
    }
}


MemSpecs MemSpecParser::parse()
{
    // Check the kernel source path
    QDir kernelSrc(_kernelSrcDir);
    if (! (kernelSrc.exists() &&
           kernelSrc.exists("Makefile") &&
           kernelSrc.exists("Kbuild")) )
        memSpecParserError(QString("Directory \"%1\" does not seem to be a kernel source or header tree.").arg(kernelSrc.absolutePath()));
    if (! (kernelSrc.exists(".config") &&
                kernelSrc.exists("Module.symvers")) )
        memSpecParserError(QString("Kernel source in \"%1\" does not seem to be configured properly.").arg(kernelSrc.absolutePath()));

    // Check the System.map file
    if (!QFile::exists(_systemMapFile))
        memSpecParserError(QString("The file \"%1\" does not exist.").arg(_systemMapFile));

    MemSpecs specs;

    // Process the System.map file
    parseSystemMap(&specs);

    // Process the kernel source tree
    setupBuildDir();
    buildHelperProg();
    parseHelperProg(&specs);

    return specs;
}

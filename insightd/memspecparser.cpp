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

#define MEMSPEC_C_BODY "%MEMSPEC_BODY%"
const char* memspec_c_file = "memspec.c";
const char* memspec_c_src =
    "#include <linux/kernel.h>\n"
    "#include <linux/module.h>\n"
    "#include <linux/mm.h>\n"
	"#include <asm/highmem.h>\n"
    "#include \"memprint.h\"\n"
    "\n"
    "// cleanup some symbols\n"
    "#undef __always_inline\n"
    "\n"
    "// As long as there is no VMEMMAP_END defined, we have to define\n"
    "// it ourselves (see Documentation/x86_64/mm.txt)\n"
    "#if defined(VMEMMAP_START) && ! defined(VMEMMAP_END)\n"
    "#define VMEMMAP_BITS 40\n"
    "#define VMEMMAP_END (VMEMMAP_START | ((1UL << VMEMMAP_BITS)-1))\n"
    "#endif\n"
    "\n"
    "int init_module(void) { return 0; }\n"
    "\n"
    "void cleanup_module(void) {}\n"
    "\n"
    "int main(int argc, char** argv) {\n"
    "  // Define potentially unresolved symbols\n"
	"#ifndef __FIXADDR_TOP\n"
    "  unsigned long __FIXADDR_TOP = 0;\n"
	"#endif\n"
    "  unsigned long high_memory = 0;\n"
	"  unsigned long vmalloc_earlyreserve = 0;\n"
    "\n"
    "  mt_initmem();\n"
    MEMSPEC_C_BODY       // this placeholder gets replaced later on
    "  mt_printmem();\n"
    "  return 0;\n"
    "}\n"
    "\n"
    ;

#define MEMPRINT_STRUCT_NAME "mt_mem_vars"
#define MEMPRINT_H_ENUM "%MEMPRINT_ENUM%"
#define MEMPRINT_H_STRUCT "%MEMPRINT_STRUCT%"
const char* memprint_h_file = "memprint.h";
const char* memprint_h_src =
    "#ifndef MEMPRINT_H\n"
    "#define MEMPRINT_H 1\n"
    "\n"
    "enum Defines {\n"
    MEMPRINT_H_ENUM
    "};\n"
    "\n"
    "struct MemVars {\n"
    "    int defines;\n"
    MEMPRINT_H_STRUCT
    "};\n"
    "\n"
    "extern struct MemVars " MEMPRINT_STRUCT_NAME ";\n"
    "\n"
    "void mt_initmem(void);\n"
    "void mt_printmem(void);\n"
    "\n"
    "#endif\n"
    "\n"
    ;

#define MEMPRINT_C_BODY "%MEMPRINT_BODY%"
const char* memprint_c_file = "memprint.c";
const char* memprint_c_src =
    "#include \"memprint.h\"\n"
    "#include <stdio.h>\n"
    "#include <string.h>\n"
    "\n"
    "struct MemVars " MEMPRINT_STRUCT_NAME ";\n"
    "\n"
    "void mt_initmem(void)\n"
    "{\n"
    "    memset(&" MEMPRINT_STRUCT_NAME ", 0, sizeof(struct MemVars));\n"
    "}\n"
    "\n"
    "void mt_printmem(void)\n"
    "{\n"
    MEMPRINT_C_BODY
    "}\n"
    "\n"
    ;

#define MAKEFILE_KDIR "%KDIR%"
#define MAKEFILE_MDIR "%MDIR%"
const char* memspec_makefile =
    "EXTRA_CFLAGS += -I/usr/include -I/usr/include/x86_64-linux-gnu\n"
    "KDIR := " MAKEFILE_KDIR "\n"  // this placeholder gets replaced later on
    "MDIR := " MAKEFILE_MDIR "\n"  // this placeholder gets replaced later on
    "OBJ := memspec.o memprint.o\n"
    "\n"
    "obj-m += $(OBJ)\n"
    "\n"
    "all:\n"
    "\tmake -C $(KDIR) M=$(MDIR) modules\n"
    "\n"
    "clean:\n"
    "\tmake -C $(KDIR) M=$(MDIR) clean\n"
    "\t@rm -fv memspec\n"
    "\n"
    "memspec: $(OBJ)\n"
    //      Just try both 64 bit and 32 bit cross-compilation
    "\tgcc -m64 -o memspec $(OBJ) || gcc -m32 -o memspec $(OBJ)\n";




MemSpecParser::MemSpecParser(const QString& kernelSrcDir, const QString& systemMapFile)
    : _autoRemoveBuildDir(true), _kernelSrcDir(kernelSrcDir),
      _systemMapFile(systemMapFile), _vmallocEarlyreserve(0)
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


void MemSpecParser::writeSrcFile(const QString& fileName, const QString& src)
{
    QFile srcfile(fileName);
    if (!srcfile.open(QIODevice::WriteOnly))
        memSpecParserError(
                QString("Cannot open file \"%1\" for writing.")
                    .arg(srcfile.fileName()));
    srcfile.write(src.toAscii());
    srcfile.close();
}


void MemSpecParser::setupBuildDir()
{
    char buf[] = "/tmp/insight.XXXXXX";
    if (!mkdtemp(buf)) {
        int e = errno;
        memSpecParserError(QString("Error creating temporary directory: %1").arg(strerror(e)));
    }
    _buildDir = QString(buf);

    // Prepare the source files
    QString ms_body, pm_enum, pm_struct, pm_body;
    int shmt = 0;
    KernelMemSpecList list = MemSpecs::supportedMemSpecs();
    for (int i = 0; i < list.size(); ++i) {
        pm_struct += QString("  unsigned long long %1;\n")
                    .arg(list[i].keyFmt.toLower());

        if (list[i].macroCond.isEmpty()) {
            pm_body += QString("  printf(\"%1 = %2\\n\", " MEMPRINT_STRUCT_NAME ".%3);\n")
                    .arg(list[i].keyFmt)
                    .arg(list[i].outputFmt)
                    .arg(list[i].keyFmt.toLower());

            ms_body += QString("  " MEMPRINT_STRUCT_NAME ".%1 = (%2);\n")
                        .arg(list[i].keyFmt.toLower())
                        .arg(list[i].valueFmt);
        }
        else {
            pm_enum += QString("  def_%1 = (1 << %2),\n")
                    .arg(list[i].keyFmt.toLower())
                    .arg(shmt++);

            pm_body += QString("  if (" MEMPRINT_STRUCT_NAME ".defines & def_%3)\n"
                               "    printf(\"%1 = %2\\n\", " MEMPRINT_STRUCT_NAME ".%3);\n")
                    .arg(list[i].keyFmt)
                    .arg(list[i].outputFmt)
                    .arg(list[i].keyFmt.toLower());

            ms_body += QString("#if %0\n"
                               "  " MEMPRINT_STRUCT_NAME ".defines |= def_%1;\n"
                               "  " MEMPRINT_STRUCT_NAME ".%1 = (%2);\n"
                               "#endif\n")
                        .arg(list[i].macroCond)
                        .arg(list[i].keyFmt.toLower())
                        .arg(list[i].valueFmt);
        }
    }
    // Replace tokens and write source files
    QString pm_h_src = QString(memprint_h_src)
            .replace(MEMPRINT_H_ENUM, pm_enum)
            .replace(MEMPRINT_H_STRUCT, pm_struct);
    writeSrcFile(_buildDir + "/" + memprint_h_file, pm_h_src);

    QString pm_c_src = QString(memprint_c_src)
            .replace(MEMPRINT_C_BODY, pm_body);
    writeSrcFile(_buildDir + "/" + memprint_c_file, pm_c_src);

    QString ms_c_src = QString(memspec_c_src)
    		.replace(MEMSPEC_C_BODY, ms_body);
    writeSrcFile(_buildDir + "/" + memspec_c_file, ms_c_src);

    QString makesrc = QString(memspec_makefile)
            .replace(MAKEFILE_KDIR, _kernelSrcDir)
            .replace(MAKEFILE_MDIR, _buildDir);
    writeSrcFile(_buildDir + "/Makefile", makesrc);
}


void MemSpecParser::buildHelperProg(const MemSpecs& specs)
{
    QProcess proc;
    QStringList cmdlines;
    QString arch = (specs.arch & MemSpecs::ar_x86_64) ? "x86_64" : "i386";

    cmdlines += QString("make KDIR=%1 ARCH=%2")
            .arg(QDir::current().absoluteFilePath(_kernelSrcDir))
            .arg(arch);
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

    bool lvl4_ok = false, swp_ok = false, ok;
    QRegExp re("^\\s*([0-9a-fA-F]+)\\s+.\\s+(.*)$");

    while (!lvl4_ok && !sysMap.atEnd() && sysMap.readLine(buf, bufsize) > 0) {
        QString line(buf);
        if (line.contains("init_level4_pgt") && re.exactMatch(line))
            // Update the given MemSpecs object with the parsed key-value pair
            specs->initLevel4Pgt = re.cap(1).toULong(&lvl4_ok, 16);
        else if (line.contains("swapper_pg_dir") && re.exactMatch(line))
            // Update the given MemSpecs object with the parsed key-value pair
            specs->swapperPgDir = re.cap(1).toULong(&swp_ok, 16);
        // TODO This is overwritten in MemoryDump::init() anyway, why check here?
        else if (line.contains("vmalloc_earlyreserve") && re.exactMatch(line))
            _vmallocEarlyreserve = re.cap(1).toULong(&ok, 16);
    }

    sysMap.close();

    // We expect to parse exactly one of them
    if ( ((specs->arch & MemSpecs::ar_x86_64) && !lvl4_ok) ||
         ((specs->arch & MemSpecs::ar_i386) && !swp_ok)) {
        memSpecParserError(QString("Could not parse one of the required values "
                "\"init_level4_pgt\" or \"swapper_pg_dir\" from file \"%1\"")
                .arg(_systemMapFile));
    }
}


void MemSpecParser::parseKernelConfig(MemSpecs* specs)
{
	QFile config(_kernelSrcDir + "/.config");
	if (!config.open(QIODevice::ReadOnly))
		memSpecParserError(QString("Cannot open file \"%1\" for reading")
				.arg(config.fileName()));

	QString line;
	const int bufsize = 1024;
	char buf[1024];

	QString s_i386  = "CONFIG_X86_32=";
	QString s_x8664 = "CONFIG_X86_64=";
	QString s_pae   = "CONFIG_X86_PAE=";

	specs->arch = MemSpecs::ar_undefined;

	while (!config.atEnd()) {
		config.readLine(buf, bufsize);
		line = QString(buf).trimmed();
		if (line.startsWith(QChar('#')))
			continue;

		if (line.startsWith(s_i386)) {
			specs->arch |= MemSpecs::ar_i386;
		}
		else if (line.startsWith(s_x8664)) {
			specs->arch |= MemSpecs::ar_x86_64;
		}
		else if (line.startsWith(s_pae)) {
			specs->arch |= MemSpecs::ar_pae_enabled;
        }
	}

	// Make sure we found the architecture
	if (specs->arch == MemSpecs::ar_undefined)
        memSpecParserError(QString("Could not determine configured target architecture"
                "from file \"%1\"").arg(config.fileName()));
}


MemSpecs MemSpecParser::parse()
{
    // Check the kernel source path
    QDir kernelSrc(_kernelSrcDir);
    if (! (kernelSrc.exists() && kernelSrc.exists("Makefile")) )
        memSpecParserError(QString("Directory \"%1\" does not seem to be a kernel source or header tree.").arg(kernelSrc.absolutePath()));
    if (! (kernelSrc.exists(".config") &&
                kernelSrc.exists("include/linux/version.h")) )
        memSpecParserError(QString("Kernel source in \"%1\" does not seem to be configured properly.").arg(kernelSrc.absolutePath()));

    // Check the System.map file
    if (!QFile::exists(_systemMapFile))
        memSpecParserError(QString("The file \"%1\" does not exist.").arg(_systemMapFile));

    MemSpecs specs;

    // Determine configured system architecture
    parseKernelConfig(&specs);

    // Process the System.map file first to get potentially value for
    // _vmallocEarlyreserve
    parseSystemMap(&specs);

    // Process the kernel source tree
    setupBuildDir();
    buildHelperProg(specs);
    parseHelperProg(&specs);

    return specs;
}

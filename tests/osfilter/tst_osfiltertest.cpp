#include <QString>
#include <QtTest>
#include <insight/osfilter.h>

class OsfilterTest : public QObject
{
    Q_OBJECT
    
public:
    OsfilterTest();

private Q_SLOTS:
    void parseArchitectures();
    void setArchitectures();

    void parseOsTypes();
    void setOsTypes();

    void parseMinVersion();
    void setMinVersion();

    void parseMaxVersion();
    void setMaxVersion();

    void matchArchitecture();
    void matchOsType();
    void matchMinVersion();
    void matchMaxVersion();
    void matchAll();
};


OsfilterTest::OsfilterTest()
{
}


void OsfilterTest::parseArchitectures()
{
    OsFilter filter;

    QCOMPARE(filter.architectures(), (int)OsSpecs::arIgnore);

    // Single architecture
    filter.parseOption("architecture", "x86");
    QCOMPARE(filter.architectures(), (int)OsSpecs::arX86);
    filter.clear();

    filter.parseOption("architecture", "x86_pae");
    QCOMPARE(filter.architectures(), (int)OsSpecs::arX86PAE);
    filter.clear();

    filter.parseOption("architecture", "AMD64");
    QCOMPARE(filter.architectures(), (int)OsSpecs::arAMD64);
    filter.clear();

    filter.parseOption("aRcHiTeCtUrE", "aMd64"); // case insensitive?
    QCOMPARE(filter.architectures(), (int)OsSpecs::arAMD64);
    filter.clear();

    filter.parseOption("architecture", "  amd64"); // white space?
    QCOMPARE(filter.architectures(), (int)OsSpecs::arAMD64);
    filter.clear();

    filter.parseOption("architecture", "amd64  "); // white space?
    QCOMPARE(filter.architectures(), (int)OsSpecs::arAMD64);
    filter.clear();

    filter.parseOption("architecture", "  amd64  "); // white space?
    QCOMPARE(filter.architectures(), (int)OsSpecs::arAMD64);
    filter.clear();

    // combined architectures
    filter.parseOption("architecture", "x86,x86_pae");
    QCOMPARE(filter.architectures(), (int)(OsSpecs::arX86|OsSpecs::arX86PAE));

    filter.parseOption("architecture", "x86,x86_pae,amd64");
    QCOMPARE(filter.architectures(),
             (int)(OsSpecs::arX86|OsSpecs::arX86PAE|OsSpecs::arAMD64));
}


void OsfilterTest::setArchitectures()
{
    OsFilter filter;

    // Single architecture
    filter.setArchitectures(OsSpecs::arX86);
    QCOMPARE(filter.architectures(), (int)OsSpecs::arX86);
    filter.clear();

    filter.setArchitectures(OsSpecs::arX86PAE);
    QCOMPARE(filter.architectures(), (int)OsSpecs::arX86PAE);
    filter.clear();

    filter.setArchitectures(OsSpecs::arAMD64);
    QCOMPARE(filter.architectures(), (int)OsSpecs::arAMD64);
    filter.clear();

    filter.setArchitectures(OsSpecs::arAMD64|OsSpecs::arX86PAE|OsSpecs::arX86);
    QCOMPARE(filter.architectures(),
             (int)(OsSpecs::arAMD64|OsSpecs::arX86PAE|OsSpecs::arX86));
    filter.clear();
}


void OsfilterTest::parseOsTypes()
{
    OsFilter filter;

    QCOMPARE(filter.osFamilies(), (int)OsSpecs::ofIgnore);

    // single OS
    filter.parseOption("os", "linux");
    QCOMPARE(filter.osFamilies(), (int)OsSpecs::ofLinux);
    filter.clear();

    filter.parseOption("os", "windows");
    QCOMPARE(filter.osFamilies(), (int)OsSpecs::ofWindows);
    filter.clear();

    filter.parseOption("os", "WiNdOwS"); // case sensitive?
    QCOMPARE(filter.osFamilies(), (int)OsSpecs::ofWindows);
    filter.clear();

    filter.parseOption("OS", "windows"); // case sensitive?
    QCOMPARE(filter.osFamilies(), (int)OsSpecs::ofWindows);
    filter.clear();

    filter.parseOption("OS", "  windows"); // white space?
    QCOMPARE(filter.osFamilies(), (int)OsSpecs::ofWindows);
    filter.clear();

    filter.parseOption("OS", "windows  "); // white space?
    QCOMPARE(filter.osFamilies(), (int)OsSpecs::ofWindows);
    filter.clear();

    filter.parseOption("OS", " windows  "); // white space?
    QCOMPARE(filter.osFamilies(), (int)OsSpecs::ofWindows);
    filter.clear();

    // combined OS types
    filter.parseOption("os", "linux,windows");
    QCOMPARE(filter.osFamilies(), (int)(OsSpecs::ofLinux|OsSpecs::ofWindows));
    filter.clear();

    filter.parseOption("os", "windows,linux");
    QCOMPARE(filter.osFamilies(), (int)(OsSpecs::ofLinux|OsSpecs::ofWindows));
    filter.clear();
}


void OsfilterTest::setOsTypes()
{
    OsFilter filter;
    filter.setOsFamilies(OsSpecs::ofWindows);
    QCOMPARE(filter.osFamilies(), (int)OsSpecs::ofWindows);
    filter.clear();

    filter.setOsFamilies(OsSpecs::ofLinux);
    QCOMPARE(filter.osFamilies(), (int)OsSpecs::ofLinux);
    filter.clear();
}


void OsfilterTest::parseMinVersion()
{
    QStringList ver;
    ver << "2" << "6" << "32" << "5c";

    OsFilter filter;
    QVERIFY(filter.minVersion().isEmpty());

    filter.parseOption("minosversion", ver.join("."));
    QCOMPARE(filter.minVersion(), ver);
    filter.clear();

    filter.parseOption("minosversion", ver.join(","));
    QCOMPARE(filter.minVersion(), ver);
    filter.clear();

    filter.parseOption("minosversion", ver.join("-"));
    QCOMPARE(filter.minVersion(), ver);
    filter.clear();
}


void OsfilterTest::setMinVersion()
{
    QStringList ver;
    ver << "2" << "6" << "32" << "5c";

    OsFilter filter;
    filter.setMinVersion(ver);
    QCOMPARE(filter.minVersion(), ver);
}


void OsfilterTest::parseMaxVersion()
{
    QStringList ver;
    ver << "2" << "6" << "32" << "5c";

    OsFilter filter;
    QVERIFY(filter.maxVersion().isEmpty());

    filter.parseOption("maxosversion", ver.join("."));
    QCOMPARE(filter.maxVersion(), ver);
    filter.clear();

    filter.parseOption("maxosversion", ver.join(","));
    QCOMPARE(filter.maxVersion(), ver);
    filter.clear();

    filter.parseOption("maxosversion", ver.join("-"));
    QCOMPARE(filter.maxVersion(), ver);
    filter.clear();
}


void OsfilterTest::setMaxVersion()
{
    QStringList ver;
    ver << "2" << "6" << "32" << "5c";

    OsFilter filter;
    filter.setMaxVersion(ver);
    QCOMPARE(filter.maxVersion(), ver);
}


void OsfilterTest::matchArchitecture()
{
    OsFilter f;
    OsSpecs s;

    QVERIFY(f.match(s));

    // Match architectures
    f.setArchitectures(OsSpecs::arX86);
    QVERIFY(f.match(s));
    s.setArchitecture(OsSpecs::arX86);
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.setArchitectures(OsSpecs::arAMD64);
    QVERIFY(f.match(s));
    s.setArchitecture(OsSpecs::arAMD64);
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.setArchitectures(OsSpecs::arAMD64|OsSpecs::arX86);
    QVERIFY(f.match(s));
    s.setArchitecture(OsSpecs::arAMD64);
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.setArchitectures(OsSpecs::arX86);
    QVERIFY(f.match(s));
    s.setArchitecture(OsSpecs::arX86PAE);
    QVERIFY(!f.match(s));
    f.clear(); s.clear();
}


void OsfilterTest::matchOsType()
{
    OsFilter f;
    OsSpecs s;

    // Match OS type
    f.setOsFamilies(OsSpecs::ofLinux);
    QVERIFY(f.match(s));
    s.setOsFamily(OsSpecs::ofLinux);
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.setOsFamilies(OsSpecs::ofWindows);
    QVERIFY(f.match(s));
    s.setOsFamily(OsSpecs::ofWindows);
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.setOsFamilies(OsSpecs::ofLinux|OsSpecs::ofWindows);
    QVERIFY(f.match(s));
    s.setOsFamily(OsSpecs::ofLinux);
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.setOsFamilies(OsSpecs::ofLinux);
    QVERIFY(f.match(s));
    s.setOsFamily(OsSpecs::ofWindows);
    QVERIFY(!f.match(s));
    f.clear(); s.clear();
}


void OsfilterTest::matchMinVersion()
{
    OsFilter f;
    OsSpecs s;

    f.parseOption("minosversion", "2.6.31");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31");
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.parseOption("minosversion", "2.6.31");
    QVERIFY(f.match(s));
    s.setVersion("2.6.32");
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.parseOption("minosversion", "2.6.31");
    QVERIFY(f.match(s));
    s.setVersion("2.7.31");
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.parseOption("minosversion", "2.6.31");
    QVERIFY(f.match(s));
    s.setVersion("3.6.31");
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.parseOption("minosversion", "2.6.31");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31.1");
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.parseOption("minosversion", "2.6.31");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31-a");
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.parseOption("minosversion", "2.6.31-a");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31-b");
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.parseOption("minosversion", "2.6.31-1");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31-b");
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.parseOption("minosversion", "02.06.31");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31");
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.parseOption("minosversion", "02.06.031");
    QVERIFY(f.match(s));
    s.setVersion("2.6.32");
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.parseOption("minosversion", "2.6.31-a");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31-b");
    QVERIFY(f.match(s));
    f.clear(); s.clear();
}


void OsfilterTest::matchMaxVersion()
{
    OsFilter f;
    OsSpecs s;

    f.parseOption("maxosversion", "2.6.31");
    QVERIFY(f.match(s));
    s.setVersion("2.6.32");
    QVERIFY(!f.match(s));
    f.clear(); s.clear();

    f.parseOption("maxosversion", "2.6.31");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31");
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.parseOption("maxosversion", "2.6.31");
    QVERIFY(f.match(s));
    s.setVersion("2.7.31");
    QVERIFY(!f.match(s));
    f.clear(); s.clear();

    f.parseOption("maxosversion", "2.6.31");
    QVERIFY(f.match(s));
    s.setVersion("3.6.31");
    QVERIFY(!f.match(s));
    f.clear(); s.clear();

    f.parseOption("maxosversion", "2.6.31");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31.1");
    QVERIFY(!f.match(s));
    f.clear(); s.clear();

    f.parseOption("maxosversion", "2.6.31");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31-a");
    QVERIFY(!f.match(s));
    f.clear(); s.clear();

    f.parseOption("maxosversion", "2.6.31-a");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31-b");
    QVERIFY(!f.match(s));
    f.clear(); s.clear();

    f.parseOption("maxosversion", "2.6.31-1");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31-b");
    QVERIFY(!f.match(s));
    f.clear(); s.clear();

    f.parseOption("maxosversion", "02.06.31");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31");
    QVERIFY(f.match(s));
    f.clear(); s.clear();

    f.parseOption("maxosversion", "02.06.031");
    QVERIFY(f.match(s));
    s.setVersion("2.6.32");
    QVERIFY(!f.match(s));
    f.clear(); s.clear();

    f.parseOption("maxosversion", "2.6.31-a");
    QVERIFY(f.match(s));
    s.setVersion("2.6.31-b");
    QVERIFY(!f.match(s));
}


void OsfilterTest::matchAll()
{
    OsFilter f;
    OsSpecs s;

    f.setArchitectures(OsSpecs::arX86|OsSpecs::arX86PAE);
    f.setOsFamilies(OsSpecs::ofLinux);
    f.parseOption("minosversion", "2.6.30");
    f.parseOption("maxosversion", "2.6.40");

    s.setVersion("2.6.5");
    QVERIFY(!f.match(s));

    s.setVersion("3.0");
    QVERIFY(!f.match(s));

    s.setVersion("2.6.32");
    QVERIFY(f.match(s));

    s.setVersion("2.6.32.1");
    QVERIFY(f.match(s));

    s.setVersion("2.6.32.a");
    QVERIFY(f.match(s));

    s.setArchitecture(OsSpecs::arAMD64);
    QVERIFY(!f.match(s));

    s.setArchitecture(OsSpecs::arX86);
    QVERIFY(f.match(s));

    s.setArchitecture(OsSpecs::arX86PAE);
    QVERIFY(f.match(s));

    s.setOsFamily(OsSpecs::ofWindows);
    QVERIFY(!f.match(s));

    s.setOsFamily(OsSpecs::ofLinux);
    QVERIFY(f.match(s));
}


QTEST_APPLESS_MAIN(OsfilterTest)

#include "tst_osfiltertest.moc"

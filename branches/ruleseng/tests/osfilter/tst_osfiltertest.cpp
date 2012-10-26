#include <QString>
#include <QtTest>
#include <osfilter.h>

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

    QCOMPARE(filter.architectures(), (int)OsFilter::arIgnore);

    // Single architecture
    filter.parseOption("architecture", "x86");
    QCOMPARE(filter.architectures(), (int)OsFilter::arX86);
    filter.clear();

    filter.parseOption("architecture", "x86_pae");
    QCOMPARE(filter.architectures(), (int)OsFilter::arX86PAE);
    filter.clear();

    filter.parseOption("architecture", "AMD64");
    QCOMPARE(filter.architectures(), (int)OsFilter::arAMD64);
    filter.clear();

    filter.parseOption("aRcHiTeCtUrE", "aMd64"); // case insensitive?
    QCOMPARE(filter.architectures(), (int)OsFilter::arAMD64);
    filter.clear();

    filter.parseOption("architecture", "  amd64"); // white space?
    QCOMPARE(filter.architectures(), (int)OsFilter::arAMD64);
    filter.clear();

    filter.parseOption("architecture", "amd64  "); // white space?
    QCOMPARE(filter.architectures(), (int)OsFilter::arAMD64);
    filter.clear();

    filter.parseOption("architecture", "  amd64  "); // white space?
    QCOMPARE(filter.architectures(), (int)OsFilter::arAMD64);
    filter.clear();

    // combined architectures
    filter.parseOption("architecture", "x86,x86_pae");
    QCOMPARE(filter.architectures(), (int)(OsFilter::arX86|OsFilter::arX86PAE));

    filter.parseOption("architecture", "x86,x86_pae,amd64");
    QCOMPARE(filter.architectures(),
             (int)(OsFilter::arX86|OsFilter::arX86PAE|OsFilter::arAMD64));
}


void OsfilterTest::setArchitectures()
{
    OsFilter filter;

    // Single architecture
    filter.setArchitectures(OsFilter::arX86);
    QCOMPARE(filter.architectures(), (int)OsFilter::arX86);
    filter.clear();

    filter.setArchitectures(OsFilter::arX86PAE);
    QCOMPARE(filter.architectures(), (int)OsFilter::arX86PAE);
    filter.clear();

    filter.setArchitectures(OsFilter::arAMD64);
    QCOMPARE(filter.architectures(), (int)OsFilter::arAMD64);
    filter.clear();

    filter.setArchitectures(OsFilter::arAMD64|OsFilter::arX86PAE|OsFilter::arX86);
    QCOMPARE(filter.architectures(),
             (int)(OsFilter::arAMD64|OsFilter::arX86PAE|OsFilter::arX86));
    filter.clear();
}


void OsfilterTest::parseOsTypes()
{
    OsFilter filter;

    QCOMPARE(filter.osTypes(), (int)OsFilter::osIgnore);

    // single OS
    filter.parseOption("os", "linux");
    QCOMPARE(filter.osTypes(), (int)OsFilter::osLinux);
    filter.clear();

    filter.parseOption("os", "windows");
    QCOMPARE(filter.osTypes(), (int)OsFilter::osWindows);
    filter.clear();

    filter.parseOption("os", "WiNdOwS"); // case sensitive?
    QCOMPARE(filter.osTypes(), (int)OsFilter::osWindows);
    filter.clear();

    filter.parseOption("OS", "windows"); // case sensitive?
    QCOMPARE(filter.osTypes(), (int)OsFilter::osWindows);
    filter.clear();

    filter.parseOption("OS", "  windows"); // white space?
    QCOMPARE(filter.osTypes(), (int)OsFilter::osWindows);
    filter.clear();

    filter.parseOption("OS", "windows  "); // white space?
    QCOMPARE(filter.osTypes(), (int)OsFilter::osWindows);
    filter.clear();

    filter.parseOption("OS", " windows  "); // white space?
    QCOMPARE(filter.osTypes(), (int)OsFilter::osWindows);
    filter.clear();

    // combined OS types
    filter.parseOption("os", "linux,windows");
    QCOMPARE(filter.osTypes(), (int)(OsFilter::osLinux|OsFilter::osWindows));
    filter.clear();

    filter.parseOption("os", "windows,linux");
    QCOMPARE(filter.osTypes(), (int)(OsFilter::osLinux|OsFilter::osWindows));
    filter.clear();
}


void OsfilterTest::setOsTypes()
{
    OsFilter filter;
    filter.setOsTypes(OsFilter::osWindows);
    QCOMPARE(filter.osTypes(), (int)OsFilter::osWindows);
    filter.clear();

    filter.setOsTypes(OsFilter::osLinux);
    QCOMPARE(filter.osTypes(), (int)OsFilter::osLinux);
    filter.clear();
}


void OsfilterTest::parseMinVersion()
{
    QStringList ver;
    ver << "2" << "6" << "32" << "5c";

    OsFilter filter;
    QVERIFY(filter.minVersion().isEmpty());

    filter.parseOption("minversion", ver.join("."));
    QCOMPARE(filter.minVersion(), ver);
    filter.clear();

    filter.parseOption("minversion", ver.join(","));
    QCOMPARE(filter.minVersion(), ver);
    filter.clear();

    filter.parseOption("minversion", ver.join("-"));
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

    filter.parseOption("maxversion", ver.join("."));
    QCOMPARE(filter.maxVersion(), ver);
    filter.clear();

    filter.parseOption("maxversion", ver.join(","));
    QCOMPARE(filter.maxVersion(), ver);
    filter.clear();

    filter.parseOption("maxversion", ver.join("-"));
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
    OsFilter f1, f2;

    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));

    // Match architectures
    f1.setArchitectures(OsFilter::arX86);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.setArchitectures(OsFilter::arX86);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.setArchitectures(OsFilter::arAMD64);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.setArchitectures(OsFilter::arAMD64);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.setArchitectures(OsFilter::arAMD64|OsFilter::arX86);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.setArchitectures(OsFilter::arAMD64);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.setArchitectures(OsFilter::arX86);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.setArchitectures(OsFilter::arX86PAE);
    QVERIFY(!f1.match(f2));
    QVERIFY(!f2.match(f1));
    f1.clear(); f2.clear();
}


void OsfilterTest::matchOsType()
{
    OsFilter f1, f2;

    // Match OS type
    f1.setOsTypes(OsFilter::osLinux);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.setOsTypes(OsFilter::osLinux);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.setOsTypes(OsFilter::osWindows);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.setOsTypes(OsFilter::osWindows);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.setOsTypes(OsFilter::osLinux|OsFilter::osWindows);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.setOsTypes(OsFilter::osLinux);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.setOsTypes(OsFilter::osLinux);
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.setOsTypes(OsFilter::osWindows);
    QVERIFY(!f1.match(f2));
    QVERIFY(!f2.match(f1));
    f1.clear(); f2.clear();
}


void OsfilterTest::matchMinVersion()
{
    OsFilter f1, f2;

    f1.parseOption("minversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("minversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("minversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("minversion", "2.6.32");
    QVERIFY(f1.match(f2));
    QVERIFY(!f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("minversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("minversion", "2.7.31");
    QVERIFY(f1.match(f2));
    QVERIFY(!f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("minversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("minversion", "3.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(!f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("minversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("minversion", "2.6.31.1");
    QVERIFY(f1.match(f2));
    QVERIFY(!f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("minversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("minversion", "2.6.31-a");
    QVERIFY(f1.match(f2));
    QVERIFY(!f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("minversion", "2.6.31-a");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("minversion", "2.6.31-b");
    QVERIFY(f1.match(f2));
    QVERIFY(!f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("minversion", "2.6.31-1");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("minversion", "2.6.31-b");
    QVERIFY(f1.match(f2));
    QVERIFY(!f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("minversion", "02.06.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("minversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("minversion", "02.06.031");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("minversion", "2.6.32");
    QVERIFY(f1.match(f2));
    QVERIFY(!f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("minversion", "2.6.31-a");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("minversion", "2.6.31-b");
    QVERIFY(f1.match(f2));
    QVERIFY(!f2.match(f1));
    f1.clear(); f2.clear();
}


void OsfilterTest::matchMaxVersion()
{
    OsFilter f1, f2;

    f1.parseOption("maxversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("maxversion", "2.6.32");
    QVERIFY(!f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("maxversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("maxversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("maxversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("maxversion", "2.7.31");
    QVERIFY(!f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("maxversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("maxversion", "3.6.31");
    QVERIFY(!f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("maxversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("maxversion", "2.6.31.1");
    QVERIFY(!f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("maxversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("maxversion", "2.6.31-a");
    QVERIFY(!f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("maxversion", "2.6.31-a");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("maxversion", "2.6.31-b");
    QVERIFY(!f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("maxversion", "2.6.31-1");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("maxversion", "2.6.31-b");
    QVERIFY(!f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("maxversion", "02.06.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("maxversion", "2.6.31");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("maxversion", "02.06.031");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("maxversion", "2.6.32");
    QVERIFY(!f1.match(f2));
    QVERIFY(f2.match(f1));
    f1.clear(); f2.clear();

    f1.parseOption("maxversion", "2.6.31-a");
    QVERIFY(f1.match(f2));
    QVERIFY(f2.match(f1));
    f2.parseOption("maxversion", "2.6.31-b");
    QVERIFY(!f1.match(f2));
    QVERIFY(f2.match(f1));
}


void OsfilterTest::matchAll()
{
    OsFilter f1, f2;

    f1.setArchitectures(OsFilter::arX86|OsFilter::arX86PAE);
    f1.setOsTypes(OsFilter::osLinux);
    f1.parseOption("minversion", "2.6.30");
    f1.parseOption("maxversion", "2.6.40");

    f2.parseOption("minversion", "2.6.5");
    f2.parseOption("maxversion", "2.6.5");
    QVERIFY(!f1.match(f2));
    QVERIFY(!f2.match(f1));

    f2.parseOption("minversion", "3.0");
    f2.parseOption("maxversion", "3.0");
    QVERIFY(!f1.match(f2));
    QVERIFY(!f2.match(f1));

    f2.parseOption("minversion", "2.6.32");
    f2.parseOption("maxversion", "2.6.32");
    QVERIFY(f1.match(f2));
    QVERIFY(!f2.match(f1));

    f2.parseOption("minversion", "2.6.32.1");
    f2.parseOption("maxversion", "2.6.32.1");
    QVERIFY(f1.match(f2));
    QVERIFY(!f2.match(f1));

    f2.parseOption("minversion", "2.6.32.a");
    f2.parseOption("maxversion", "2.6.32.a");
    QVERIFY(f1.match(f2));
    QVERIFY(!f2.match(f1));

    f2.setArchitectures(OsFilter::arAMD64);
    QVERIFY(!f1.match(f2));
    QVERIFY(!f2.match(f1));

    f2.setArchitectures(OsFilter::arX86);
    QVERIFY(f1.match(f2));
//    QVERIFY(f2.match(f1));

    f2.setArchitectures(OsFilter::arX86PAE);
    QVERIFY(f1.match(f2));
//    QVERIFY(f2.match(f1));

    f2.setOsTypes(OsFilter::osWindows);
    QVERIFY(!f1.match(f2));
//    QVERIFY(!f2.match(f1));

    f2.setOsTypes(OsFilter::osLinux);
    QVERIFY(f1.match(f2));
//    QVERIFY(f2.match(f1));
}


QTEST_APPLESS_MAIN(OsfilterTest)

#include "tst_osfiltertest.moc"

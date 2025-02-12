// HEX SDK

#include <string>

#include <hex/log.h>
#include <hex/license.h>
#include <hex/test.h>

int main() {
    HexLogInit("test_license_check_01", 1 /*logToStdErr*/);
    //HexLogDebugLevel = 3;
    system("rm -f /etc/update/license.dat /etc/update/license.sig");
    system("rm -f /etc/update/license-app.dat /etc/update/license-app.sig");
    system("rm -f /etc/update/test-app.dat /etc/update/test-app.sig");

    std::string type, serial;

    // missing license files
    HEX_TEST(HexLicenseCheck("def", &type, &serial) == LICENSE_NOEXIST);

    system("touch /etc/update/license.dat");
    HEX_TEST(HexLicenseCheck("def", &type, &serial) == LICENSE_NOEXIST);

    // invalid license files
    system("touch /etc/update/license.sig");
    HEX_TEST(HexLicenseCheck("def", &type, &serial) == LICENSE_BADSIG);

    // verify valid license files
    system("cp -f /etc/update/license.dat.perpetual.good /etc/update/license.dat");
    system("cp -f /etc/update/license.sig.perpetual.good /etc/update/license.sig");
    HEX_TEST(HexLicenseCheck("def", &type, &serial) == 60 /* remaining 60 days */);
    HEX_TEST(type == "perpetual");
    system("cp -f /etc/update/license.dat.subscribed.good /etc/update/license.dat");
    system("cp -f /etc/update/license.sig.subscribed.good /etc/update/license.sig");
    HEX_TEST(HexLicenseCheck("def", &type, &serial) == 60 /* remaining 60 days */);
    HEX_TEST(type == "subscribed");
    system("cp -f /etc/update/license.dat.subscribed-poorform.good /etc/update/license.dat");
    system("cp -f /etc/update/license.sig.subscribed-poorform.good /etc/update/license.sig");
    HEX_TEST(HexLicenseCheck("def", &type, &serial) == 60 /* remaining 60 days */);
    HEX_TEST(type == "subscribed");
    system("cp -f /etc/update/license.dat.trial.good /etc/update/license.dat");
    system("cp -f /etc/update/license.sig.trial.good /etc/update/license.sig");
    HEX_TEST(HexLicenseCheck("def", &type, &serial) == 60 /* remaining 60 days */);
    HEX_TEST(type == "trial");

    // verify tampered license files
    system("cp -f /etc/update/license.dat.perpetual.good /etc/update/license.dat");
    system("cp -f /etc/update/license.sig.perpetual.good /etc/update/license.sig");
    system("echo 'abc=123' >> /etc/update/license.dat");
    HEX_TEST(HexLicenseCheck("def", &type, &serial) == LICENSE_BADSIG);

    // verify expired license files
    system("cp -f /etc/update/license.dat.perpetual.expired /etc/update/license.dat");
    system("cp -f /etc/update/license.sig.perpetual.expired /etc/update/license.sig");
    HEX_TEST(HexLicenseCheck("def", &type, &serial) == LICENSE_EXPIRED);
    HEX_TEST(type == "perpetual");

    // verify bad hardware license files
    system("cp -f /etc/update/license.dat.perpetual.badhw /etc/update/license.dat");
    system("cp -f /etc/update/license.sig.perpetual.badhw /etc/update/license.sig");
    HEX_TEST(HexLicenseCheck("def", &type, &serial) == LICENSE_BADHW);
    HEX_TEST(type == "perpetual");

    // verify valid license files in custom name
    system("cp -f /etc/update/license.dat.perpetual.good /etc/update/test.dat");
    system("cp -f /etc/update/license.sig.perpetual.good /etc/update/test.sig");
    HEX_TEST(HexLicenseCheck("def", &type, &serial, "/etc/update/test") == 60 /* remaining 60 days */);
    HEX_TEST(type == "perpetual");

    // verify expired license files in custom name
    system("cp -f /etc/update/license.dat.perpetual.expired /etc/update/test.dat");
    system("cp -f /etc/update/license.sig.perpetual.expired /etc/update/test.sig");
    HEX_TEST(HexLicenseCheck("def", &type, &serial, "/etc/update/test") == LICENSE_EXPIRED);
    HEX_TEST(type == "perpetual");

    // verify expired license files with not default app
    HEX_TEST(HexLicenseCheck("app", &type, &serial) == LICENSE_NOEXIST);
    system("cp -f /etc/update/license.dat.perpetual.expired /etc/update/license-app.dat");
    system("cp -f /etc/update/license.sig.perpetual.expired /etc/update/license-app.sig");
    HEX_TEST(HexLicenseCheck("app", &type, &serial) == LICENSE_EXPIRED);

    // verify expired license files in custom name and with not default app
    system("cp -f /etc/update/license.dat.perpetual.expired /etc/update/test-app.dat");
    system("cp -f /etc/update/license.sig.perpetual.expired /etc/update/test-app.sig");
    HEX_TEST(HexLicenseCheck("app", &type, &serial, "/etc/update/test") == LICENSE_EXPIRED);
    HEX_TEST(type == "perpetual");

    return HexTestResult;
}
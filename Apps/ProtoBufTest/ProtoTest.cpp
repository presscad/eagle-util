// ProtoTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <google/protobuf/stubs/common.h>

extern int Test_AddressBook();
extern int Test_VehicleRecords();
extern int Test_HttpPostWithVehicleRecords();

int main()
{
    printf("ProtoBuf Test Start ...\n");

    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

#if 0
    if (0 != Test_AddressBook())
    {
        printf("Test_AddressBook: testing failed!\n");
    }
#endif
#if 0
    if (0 != Test_VehicleRecords())
    {
        printf("Test_VehicleRecords: testing failed!\n");
    }
#endif
#if 1
    if (0 != Test_HttpPostWithVehicleRecords())
    {
        printf("Test_HttpPostWithVehicleRecords: testing failed!\n");
    }
#endif
    // Optional:  Delete all global objects allocated by libprotobuf.
    google::protobuf::ShutdownProtobufLibrary();

    printf("ProtoBuf Test End ...\n");
    return 0;
}

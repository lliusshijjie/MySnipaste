#include "test_framework.h"

#include "ocr/OcrThreadRunner.h"

#include <objbase.h>
#include <thread>

TEST_CASE(OcrThreadRunner_ExecutesOperationOnMtaWorker) {
    const auto callerThread = std::this_thread::get_id();
    std::thread::id workerThread{};
    APTTYPE apartmentType = APTTYPE_CURRENT;
    APTTYPEQUALIFIER apartmentQualifier = APTTYPEQUALIFIER_NONE;

    const bool succeeded = mysnip::ocr::OcrThreadRunner::Run([&] {
        workerThread = std::this_thread::get_id();
        return SUCCEEDED(CoGetApartmentType(&apartmentType, &apartmentQualifier));
    });

    REQUIRE(succeeded);
    REQUIRE(workerThread != callerThread);
    REQUIRE(apartmentType == APTTYPE_MTA);
}

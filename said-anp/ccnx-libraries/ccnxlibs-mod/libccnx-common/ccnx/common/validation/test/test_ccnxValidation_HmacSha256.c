/*
 * Copyright (c) 2017 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 */

// Include the file(s) containing the functions to be tested.
// This permits internal static functions to be visible to this Test Framework.
#include "../ccnxValidation_HmacSha256.c"
#include <parc/algol/parc_SafeMemory.h>

#include <parc/algol/parc_Object.h>

#include <LongBow/unit-test.h>
#include "testrig_validation.c"

LONGBOW_TEST_RUNNER(ccnxValidation_HmacSha256)
{
    // The following Test Fixtures will run their corresponding Test Cases.
    // Test Fixtures are run in the order specified, but all tests should be idempotent.
    // Never rely on the execution order of tests or share state between them.
    LONGBOW_RUN_TEST_FIXTURE(Global);
    LONGBOW_RUN_TEST_FIXTURE(Local);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(ccnxValidation_HmacSha256)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(ccnxValidation_HmacSha256)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Global)
{
    LONGBOW_RUN_TEST_CASE(Global, ccnxValidationHmacSha256_Set);
    LONGBOW_RUN_TEST_CASE(Global, ccnxValidationHmacSha256_CreateSigner);
    LONGBOW_RUN_TEST_CASE(Global, ccnxValidationHmacSha256_DictionaryCryptoSuiteValue);
}

LONGBOW_TEST_FIXTURE_SETUP(Global)
{
    longBowTestCase_SetClipBoardData(testCase, commonSetup());
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Global)
{
    commonTeardown(longBowTestCase_GetClipBoardData(testCase));

    uint32_t outstandingAllocations = parcSafeMemory_ReportAllocation(STDERR_FILENO);
    if (outstandingAllocations != 0) {
        printf("%s leaks memory by %d allocations\n", longBowTestCase_GetName(testCase), outstandingAllocations);
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(Global, ccnxValidationHmacSha256_Set)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);
    testValidationSet_KeyId(data, ccnxValidationHmacSha256_Set, ccnxValidationHmacSha256_Test, true, true);
}

LONGBOW_TEST_CASE(Global, ccnxValidationHmacSha256_CreateSigner)
{
    char secretKeyString[] = "0123456789ABCDEF0123456789ABCDEF";
    PARCBuffer *secretKey = bufferFromString(strlen(secretKeyString), secretKeyString);

    PARCSigner *signer = ccnxValidationHmacSha256_CreateSigner(secretKey);
    assertNotNull(signer, "Got null signer");

    parcSigner_Release(&signer);
    parcBuffer_Release(&secretKey);
}

LONGBOW_TEST_CASE(Global, ccnxValidationHmacSha256_DictionaryCryptoSuiteValue)
{
    TestData *data = longBowTestCase_GetClipBoardData(testCase);

    CCNxTlvDictionary *dictionary = ccnxContentObject_CreateWithImplAndPayload(&CCNxContentObjectFacadeV1_Implementation,
                                                                               data->keyname,
                                                                               CCNxPayloadType_DATA,
                                                                               NULL);
    ccnxValidationHmacSha256_Set(dictionary, data->keyid);
    uint64_t cryptosuite = ccnxTlvDictionary_GetInteger(dictionary, CCNxCodecSchemaV1TlvDictionary_ValidationFastArray_CRYPTO_SUITE);
    assertTrue(cryptosuite == PARCCryptoSuite_HMAC_SHA256, "Unexpected PARCCryptoSuite value in dictionary");

    ccnxTlvDictionary_Release(&dictionary);
}

LONGBOW_TEST_FIXTURE(Local)
{
}

LONGBOW_TEST_FIXTURE_SETUP(Local)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Local)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

int
main(int argc, char *argv[])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(ccnxValidation_HmacSha256);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}

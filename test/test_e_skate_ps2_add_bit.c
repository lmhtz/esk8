#include "test_e_skate.h"

#include <e_skate_ps2.h>
#include <e_skate_ps2_utils.h>

#include <unity.h>
#include <stdint.h>


void test_e_skate_ps2_add_bit()
{
    e_skate_ps2_handle_t ps2Handle;
    e_skate_ps2_reset_handle(
        &ps2Handle
    );


    uint8_t expectedValues[] = {
        0,
        123,
        255,
        34
    };

    uint8_t expectedValuesBits[][8] = {
        {0, 0, 0, 0, 0, 0, 0, 0},
        {0, 1, 1, 1, 1, 0, 1, 1},
        {1, 1, 1, 1, 1, 1, 1, 1},
        {0, 0, 1, 0, 0, 0, 1, 0},
    };


    for (int i=0; i<4; i++)
    {
        for (int j=0; j<8; j++)
        {
            e_skate_ps2_add_bit(
                &ps2Handle,
                expectedValuesBits[i][7 - j] // Least significant bit first
            );
        }

        TEST_ASSERT_EQUAL(expectedValues[i], ps2Handle.newByte);
        e_skate_ps2_reset_handle(&ps2Handle);
    }

    e_skate_err_t errCode;
    for (int i=0; i<7; i++)
    {
        errCode = e_skate_ps2_add_bit(
            &ps2Handle,
            0
            );

        TEST_ASSERT_EQUAL(E_SKATE_SUCCESS, errCode);
    }

    errCode = e_skate_ps2_add_bit(
        &ps2Handle,
        0
        );
    
    TEST_ASSERT_EQUAL(E_SKATE_PS2_ERR_VALUE_READY, errCode);
}
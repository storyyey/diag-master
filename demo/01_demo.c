#include <stdio.h>

#include "demo.h"
#include "yapi_dm_simple.h"

int main(int argc, char *argv[])
{
    /* ================================================================= */
    /* ----------------------------------------------------------------- */
    /* ����UDS�ͻ���API */
    YAPI_DM om_api = yapi_dm_create();
    if (om_api) {
        demo_printf("UDS client api create success. \n");
    }    
    /* ----------------------------------------------------------------- */
    /* ================================================================= */

    return 0;
}


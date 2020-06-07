#include "HotTub.h"

int HotTub::decodeTemperature(unsigned int temperatureCommand)
{
    switch (temperatureCommand)
    {
    case TEMP_40C:
        return 40;
    case TEMP_39C:
        return 39;
    case TEMP_38C:
        return 38;
    case TEMP_37C:
        return 37;
    case TEMP_36C:
        return 36;
    case TEMP_35C:
        return 35;
    case TEMP_34C:
        return 34;
    case TEMP_33C:
        return 33;
    case TEMP_32C:
        return 32;
    case TEMP_31C:
        return 31;
    case TEMP_30C:
        return 30;
    case TEMP_29C:
        return 29;
    case TEMP_28C:
        return 28;
    case TEMP_27C:
        return 27;
    case TEMP_26C:
        return 26;
    case TEMP_25C:
        return 25;
    case TEMP_24C:
        return 24;
    case TEMP_23C:
        return 23;
    case TEMP_22C:
        return 22;
    case TEMP_21C:
        return 21;
    case TEMP_20C:
        return 20;
    case TEMP_19C:
        return 19;
    case TEMP_18C:
        return 18;
    case TEMP_17C:
        return 17;
    case TEMP_16C:
        return 16;
    case TEMP_15C:
        return 15;
    case TEMP_14C:
        return 14;
    case TEMP_13C:
        return 13;
    case TEMP_12C:
        return 12;
    case TEMP_11C:
        return 11;
    case TEMP_10C:
        return 10;
    case TEMP_9C:
        return 9;

    case TEMP_104F:
        return 104;
    case TEMP_102F:
        return 102;
    case TEMP_101F:
        return 101;
    case TEMP_99F:
        return 99;
    case TEMP_97F:
        return 97;
    case TEMP_95F:
        return 95;
    case TEMP_93F:
        return 93;
    case TEMP_92F:
        return 92;
    case TEMP_90F:
        return 90;
    case TEMP_88F:
        return 88;
    case TEMP_86F:
        return 86;
    case TEMP_84F:
        return 84;
    case TEMP_83F:
        return 83;
    case TEMP_81F:
        return 81;
    case TEMP_79F:
        return 79;
    case TEMP_77F:
        return 77;
    case TEMP_75F:
        return 75;
    case TEMP_74F:
        return 74;
    case TEMP_72F:
        return 72;
    case TEMP_70F:
        return 70;
    case TEMP_66F:
        return 66;
    case TEMP_65F:
        return 65;
    case TEMP_63F:
        return 63;
    case TEMP_61F:
        return 61;
    case TEMP_59F:
        return 59;
    case TEMP_57F:
        return 57;
    case TEMP_56F:
        return 56;
    case TEMP_54F:
        return 54;
    case TEMP_52F:
        return 52;
    default:
        return -1;
    }
}

int HotTub::decodeStatus(unsigned int statusCommand)
{
    switch (statusCommand)
    {
    case CMD_STATE_ALL_OFF_F:
    case CMD_STATE_ALL_OFF_C:
        return PUMP_OFF;

    case CMD_STATE_PUMP_ON_C:
    case CMD_STATE_PUMP_ON_F:
        return PUMP_FILTERING;

    case CMD_STATE_BLOWER_ON_C:
    case CMD_STATE_BLOWER_ON_F:
        return PUMP_BUBBLES;

    case CMD_STATE_PUMP_ON_HEATER_ON:
        return PUMP_HEATER_STANDBY;

    case CMD_STATE_PUMP_ON_HEATER_HEATING:
        return PUMP_HEATING;

    case CMD_FLASH:
        return PUMP_FLASH;

    case CMD_ERROR_PKT1:
    case CMD_ERROR_PKT2:
        return PUMP_ERROR;

    default:
        return PUMP_UNKNOWN;
    }
}

int HotTub::decodeError(unsigned int errorCommand)
{
    switch (errorCommand)
    {
    case CMD_ERROR_1:
        return 1;
    case CMD_ERROR_2:
        return 2;
    case CMD_ERROR_3:
        return 3;
    case CMD_ERROR_4:
        return 4;
    case CMD_ERROR_5:
        return 5;
    case CMD_ERROR_PKT1:
    case CMD_ERROR_PKT2:
    default:
        return 0;
    }
}

char *HotTub::errorToString(int errorCode)
{
    switch (errorCode)
    {
    case 1:
        return (char *)"Flow sensor stuck in on state";
    case 2:
        return (char *)"Flow sensor stuck in off state";
    case 3:
        return (char *)"Water temperature below 4C";
    case 4:
        return (char *)"Water temperature above 50C";
    case 5:
        return (char *)"Unsuitable power supply";
    default:
        return (char *)"UNKNOWN!!!";
    }
}

char *HotTub::buttonToString(int buttonCommand)
{
    switch (buttonCommand)
    {
    case CMD_BTN_HEAT:
        return (char *)"Heater";
    case CMD_BTN_PUMP:
        return (char *)"Pump";
    case CMD_BTN_BLOW:
        return (char *)"Blower";
    case CMD_BTN_TEMP_DN:
        return (char *)"Temperature down";
    case CMD_BTN_TEMP_UP:
        return (char *)"Temperature up";
    default:
        return (char *)"UNKNOWN!!!";
    }
}

char *HotTub::stateToString(int pumpState)
{
    switch (pumpState)
    {
    case PUMP_OFF:
        return (char *)"Off";
    case PUMP_FILTERING:
        return (char *)"Filtering";
    case PUMP_HEATING:
        return (char *)"Heating";
    case PUMP_HEATER_STANDBY:
        return (char *)"Heater Standby";
    case PUMP_BUBBLES:
        return (char *)"Bubbles";
    default:
        return (char *)"UNKNOWN!!!";
    }
}

char *HotTub::tubModeToString(int tubMode)
{
    switch (tubMode)
    {
    case TM_NORMAL:
        return (char *)"Normal";
    case TM_FLASH_DETECTED:
        return (char *)"Flash detected";
    case TM_FLASHING:
        return (char *)"Flashing";
    case TM_TEMP_BUTTON_DETECTED:
        return (char *)"Temp button detected";
    case TM_TEMP_MANUAL_CHANGE:
        return (char *)"Temp manual change";
    default:
        return (char *)"UNKNOWN!!";
    }
}
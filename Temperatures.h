#define TEMP_40C 0xB5F
#define TEMP_39C 0xB63
#define TEMP_38C 0xB66
#define TEMP_37C 0xB69
#define TEMP_36C 0xB6C
#define TEMP_35C 0xB72
#define TEMP_34C 0xB77
#define TEMP_33C 0xB78
#define TEMP_32C 0xB7D
#define TEMP_31C 0xB82
#define TEMP_30C 0xB87
#define TEMP_29C 0xB88
#define TEMP_28C 0xB8D
#define TEMP_27C 0xB93
#define TEMP_26C 0xB96
#define TEMP_25C 0xB99
#define TEMP_24C 0xB9C
#define TEMP_23C 0xBA0
#define TEMP_22C 0xBA5
#define TEMP_21C 0xBAA
#define TEMP_20C 0xBAF
#define TEMP_19C 0xBB1
#define TEMP_18C 0xBB4
#define TEMP_17C 0xBBB
#define TEMP_16C 0xBBE
#define TEMP_15C 0xBC3
#define TEMP_14C 0xBC6
#define TEMP_13C 0xBC9
#define TEMP_12C 0xBCC
#define TEMP_11C 0xBD2
#define TEMP_10C 0xBD7
#define TEMP_9C  0xBD8


#define TEMP_104F 0xA5E
#define TEMP_102F 0xA67
#define TEMP_101F 0xA68
#define TEMP_99F  0xA73
#define TEMP_97F  0xA79
#define TEMP_95F  0xA83
#define TEMP_93F  0xA89
/*
#define TEMP_92F  0x
#define TEMP_90F  0x
#define TEMP_88F  0x
#define TEMP_86F  0x
#define TEMP_84F  0x
#define TEMP_83F  0x
#define TEMP_81F  0x
#define TEMP_79F  0x
#define TEMP_77F  0x
#define TEMP_75F  0x
#define TEMP_74F  0x
#define TEMP_72F  0x
#define TEMP_70F  0x
#define TEMP_66F  0x
#define TEMP_65F  0x
#define TEMP_63F  0x
#define TEMP_61F  0x
#define TEMP_59F  0x
#define TEMP_57F  0x
#define TEMP_56F  0x
#define TEMP_54F  0x
#define TEMP_52F  0x
*/

int decodeTemperature(word command) {
  switch (command) {
    case TEMP_40C: return 40;  
    case TEMP_39C: return 39;
    case TEMP_38C: return 38;    
    case TEMP_37C: return 37;
    case TEMP_36C: return 36;  
    case TEMP_35C: return 35;   
    case TEMP_34C: return 34;
    case TEMP_33C: return 33;
    case TEMP_32C: return 32;
    case TEMP_31C: return 31;
    case TEMP_30C: return 30;
    case TEMP_29C: return 29;
    case TEMP_28C: return 28;
    case TEMP_27C: return 27;
    case TEMP_26C: return 26;
    case TEMP_25C: return 25;
    case TEMP_24C: return 24;
    case TEMP_23C: return 23;
    case TEMP_22C: return 22;
    case TEMP_21C: return 21;
    case TEMP_20C: return 20;
    case TEMP_19C: return 19;
    case TEMP_18C: return 18;
    case TEMP_17C: return 17;
    case TEMP_16C: return 16;
    case TEMP_15C: return 15;
    case TEMP_14C: return 14;
    case TEMP_13C: return 13;
    case TEMP_12C: return 12;
    case TEMP_11C: return 11;
    case TEMP_10C: return 10;
    case TEMP_9C : return 9;

    case TEMP_104F: return 104;
    case TEMP_102F: return 102;
    case TEMP_101F: return 101;
    case TEMP_99F: return 99;
    case TEMP_97F: return 97;
    case TEMP_95F: return 95;
    case TEMP_93F: return 93;
  }
}

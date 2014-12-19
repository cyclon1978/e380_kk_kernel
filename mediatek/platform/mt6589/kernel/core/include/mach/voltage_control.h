#define RELEASE_MODE_SPECIAL 0

#if RELEASE_MODE_SPECIAL == 1
  #define MT_FORCE_1200_MHZ 1 // force 1200 MHz variant -> kills temperature checks!
  #define MTK_VOLTAGE_LEVEL_LOWERED 0 // no voltage lowering with this faulty cpu
#else
  #define MT_FORCE_1200_MHZ 0
  #define MTK_VOLTAGE_LEVEL_LOWERED 1 // use voltage lowering
#endif

// #define MT_OVERCLOCK 1 // see mt_devs.c
// #define MT_SHOW_CPU_FLAGS 1 // see sbchk_base.c
// #define MT_FORCE_1200_MHZ_SOFT 1 // forces cpu recognition correction for faulty cpus -> deprecated, remove (not working)

#define MT_VC_VOLTAGE_0_5D 0x5D // 1.28125v ONLY USED FOR mt_ptp VMAX, do not change...
#define MT_VC_VOLTAGE_0_58 0x58 // 1.25V VPROC SHOULD NOT BE USED ON 1.2 GHz chip (g_cpufreq_get_ptp_level == 0 on normal temp)

#if MTK_VOLTAGE_LEVEL_LOWERED == 1

// lowered voltages

  // slightly undervolt -8, -6, -4, -2, +-0

  #define MT_VC_VOLTAGE_0_50 0x48 // 1.20V VPROC (Efuse = 9, 1GHz/1.2V)
  #define MT_VC_VOLTAGE_0_48 0x42 // 1.15V VPROC
  #define MT_VC_VOLTAGE_0_38 0x34 // 1.05V VPROC
  #define MT_VC_VOLTAGE_0_28 0x26 // 0.95V VPROC

  #define MT_VC_VOLTAGE_0_18 0x16 // 0.85V VPROC NOT USED IN mt_ptp VMIN ???

#else

// default values

 #define MT_VC_VOLTAGE_0_50 0x50 // 1.20V VPROC (Efuse = 9, 1GHz/1.2V)
 #define MT_VC_VOLTAGE_0_48 0x48 // 1.15V VPROC
 #define MT_VC_VOLTAGE_0_38 0x38 // 1.05V VPROC
 #define MT_VC_VOLTAGE_0_28 0x28 // 0.95V VPROC
 #define MT_VC_VOLTAGE_0_18 0x18 // 0.85V VPROC NOT USED IN mt_ptp VMIN ???

#endif

/*
1.20V (0x50) -> 1.15   V (0x48) MAX VOLTAGE @ 1.2 GHz
1.15V (0x48) -> 1.1125 V (0x42) 
1.05V (0x38) -> 1.025  V (0x34)
0.95V (0x28) -> 0,9375 V (0x26)
0.85V (0x18) -> 0,8375 V (0x16) DEEP SLEEP

0x18 24 0.85 V
0x28 40 0.95 V
0x38 56 1.05 V
0x48 72 1.15 V
0x50 80 1.20 V
0x58 88 1.25 V
0x5D 93 1.28125 V

-> + 0x10 16 -> + 0.1 V			0,00625 per 1 
-> + 0x08  8 -> + 0.05 V		
-> + 0x05  5 -> + 0.03125 V		
*/

/* documentation

Core voltage 1.05V
DVFS voltages 0.95 - 1.26
Sleep mode 0.85V
GPU voltage 1.05V

*/

/* device info: working device 1.2 GHz

<4>[   16.751701] (1)[139:zygote][devinfo-data], indx[0]:0x0
<4>[   16.751716] (1)[139:zygote][devinfo-data], indx[1]:0x0
<4>[   16.751726] (1)[139:zygote][devinfo-data], indx[2]:0x0
<4>[   16.751738] (1)[139:zygote][devinfo-data], indx[3]:0x80060244
<4>[   16.751748] (1)[139:zygote][devinfo-data], indx[4]:0x0
<4>[   16.751759] (1)[139:zygote][devinfo-data], indx[5]:0x0
<4>[   16.751769] (1)[139:zygote][devinfo-data], indx[6]:0x0
<4>[   16.751780] (1)[139:zygote][devinfo-data], indx[7]:0x5d
<4>[   16.751791] (1)[139:zygote][devinfo-data], indx[8]:0xc94b7580
<4>[   16.751802] (1)[139:zygote][devinfo-data], indx[9]:0x0
<4>[   16.751812] (1)[139:zygote][devinfo-data], indx[10]:0x0
<4>[   16.751823] (1)[139:zygote][devinfo-data], indx[11]:0x40
<4>[   16.751833] (1)[139:zygote][devinfo-data], indx[12]:0x69ed359f
<4>[   16.751845] (1)[139:zygote][devinfo-data], indx[13]:0x8817eddd
<4>[   16.751856] (1)[139:zygote][devinfo-data], indx[14]:0x0
<4>[   16.751867] (1)[139:zygote][devinfo-data], indx[15]:0x0
<4>[   16.751877] (1)[139:zygote][devinfo-data], indx[16]:0x10302f07
<4>[   16.751888] (1)[139:zygote][devinfo-data], indx[17]:0xfc555555
<4>[   16.751899] (1)[139:zygote][devinfo-data], indx[18]:0x0
<4>[   16.751910] (1)[139:zygote][devinfo-data], indx[19]:0x28630400
<4>[   16.751921] (1)[139:zygote][devinfo-data], indx[20]:0x6583

device info: 1 GHz

<4>[ 606.706047] (1)[746:Binder_2][devinfo-data], indx[0]:0x0
<4>[ 606.706060] (1)[746:Binder_2][devinfo-data], indx[1]:0x0
<4>[ 606.706073] (1)[746:Binder_2][devinfo-data], indx[2]:0x0
<4>[ 606.706085] (1)[746:Binder_2][devinfo-data], indx[3]:0x80060246
<4>[ 606.706098] (1)[746:Binder_2][devinfo-data], indx[4]:0x0
<4>[ 606.706110] (1)[746:Binder_2][devinfo-data], indx[5]:0x0
<4>[ 606.706123] (1)[746:Binder_2][devinfo-data], indx[6]:0x0
<4>[ 606.706135] (1)[746:Binder_2][devinfo-data], indx[7]:0x65
<4>[ 606.706148] (1)[746:Binder_2][devinfo-data], indx[8]:0xcf6a8400
<4>[ 606.706161] (1)[746:Binder_2][devinfo-data], indx[9]:0x0
<4>[ 606.706173] (1)[746:Binder_2][devinfo-data], indx[10]:0x60
<4>[ 606.706186] (1)[746:Binder_2][devinfo-data], indx[11]:0x40
<4>[ 606.706198] (1)[746:Binder_2][devinfo-data], indx[12]:0xc4c9d783
<4>[ 606.706212] (1)[746:Binder_2][devinfo-data], indx[13]:0x3ea12976
<4>[ 606.706224] (1)[746:Binder_2][devinfo-data], indx[14]:0x0
<4>[ 606.706237] (1)[746:Binder_2][devinfo-data], indx[15]:0x0
<4>[ 606.706249] (1)[746:Binder_2][devinfo-data], indx[16]:0x10165807
<4>[ 606.706262] (1)[746:Binder_2][devinfo-data], indx[17]:0xff555555
<4>[ 606.706275] (1)[746:Binder_2][devinfo-data], indx[18]:0x0
<4>[ 606.706288] (1)[746:Binder_2][devinfo-data], indx[19]:0x284b0400
<4>[ 606.706301] (1)[746:Binder_2][devinfo-data], indx[20]:0x6583
*/

/*
u32 val_0 = 0x14f76907; // 16: 0x10302f07
u32 val_1 = 0xf6AAAAAA; // 17: 0xfc555555
u32 val_2 = 0x14AAAAAA; // 18: 0x0
u32 val_3 = 0x60260000; // 19: 0x28630400
*/

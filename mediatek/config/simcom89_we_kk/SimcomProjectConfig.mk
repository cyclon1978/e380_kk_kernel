#
# 　请在此注释下增加控制常量
# 　控制常量命名规则：
#   控制常量共分四段，如下所示：
#   LAYER_CUSTOMER_PROJECT_MODULE_FUNCTION
#   LAYER段：　此段分
#      MMI：　仅涉及上层ＡＰＫ的修改使用此标识
#      FMWK: 仅涉及Framework层的修改使用此标识
#      CMN：　common缩写，涉及上层ＡＰＫ与framework等多层的修改使用此标识
#      
#   CUSTOMER_PROJECT段：
#      此段是客户名称与项目名称的全称，如针对联想　A390e项目的功能, 应写为 LENOVO_A390E
#      如果此功能应用于此客户的所有项目的话，可发不写项目名称，如针对联想所有项目的功能，应写为 LENOVO
#      如果此功能是平台性功能，可能会应用于所有客户的话，应写为 PLATFORM
#      
#   MODULE段：
#      此段为模块名称，如电话本模块的修改就写为 CONTACT,
#      
#   FUNCTION段：
#      此段为功能名称
#      
#   示例：
#      电话号１１位匹配功能，需要修改framework代码，应用于平台
#      FMWK_PLATFORM_TELEPHONY_PHONE_NUMBER_11_MATCH
#      
#      电话号码１３位匹配功能，需要修改framewor代码，　应用于联想，Ａ３项目
#      FMWK_LENOVO_A3_TELEPHONY_PHONE_NUMBER_13_MATCH
#      
#      去电彩铃音量抵制功能，需要修改　phone 这个ＡＰＫ中的代码，应用于平台
#      MMI_PLATFORM_PHONE_RINGTONE_VOLUMEN_LIMITED
#             
#    
#
    
#客户项目单拉时需要对此值做修改
CMN_PLATFORM_CUSTOMER_NAME = ACER

#客户项目单拉时需要对此值做修改
CMN_PLATFORM_CUSTOMER_AND_PROJECT_NAME = ACER_C17
#add by zhangjin for Magic button function for C17
MMI_PLATFORM_MAGIC_BUTTON_SUPPORT = yes

#wpf add for accer, 当支持hallsensor时，合盖,通话屏以小屏方式显示
MMI_PHONE_HALLSENSOR_SMALL_IN_CALL_SCREEN = yes
    
#add by zhujing for dual ringtone, 2014-04-23
MMI_PLATFORM_PROFILE_DUAL_RINGTONE = yes

#add by NavyGuo for vibrate when headset in.
MMI_VIB_HEADSET_IN = yes

#add by zhujing 2014-04-23 FM支持有内置天线
MTK_FM_SHORTANTENNA_SUPPORT  = no

#add by wentao for hall sensor home 
MMI_SIMCOM_HALL_SENSOR = yes

MMI_PLATFORM_QUICK_TOUCH_SUPPORT = yes

# 省电模式部分功能开关，默认关闭
MMI_SUPPORT_POWERSAVE_MODE = yes

#add by xuxiaohui for LED
FMWK_ACER_SHOW_LED = yes

#Acer data roaming option
MMI_ROAMING_OPTION = no

#qiaoxiujun,for 3uk
MMI_MMS_CAPTURE_FEATURE = no

#xuxiaohui for 3UK
CMN_PLATFORM_CUSTOMER_C17_3UK_SKU = no
#Caobo, 3UK APN switch
MMI_APN_MODE_OPTION = no

#include "APP_Ethernetif.h"
#include "cmsis_os2.h"
#include "FreeRTOS.h"
#include "task.h"
#include "shell_port.h"
int32_t ETH_PHY_ReadReg(uint16_t reg,uint32_t *regva)
{
    return HAL_ETH_ReadPHYRegister(&heth, LAN8720A_ADDR,reg, regva);
}

int32_t ETH_PHY_WriteReg(uint16_t reg,uint32_t regvaluea)
{
    return HAL_ETH_WritePHYRegister(&heth, LAN8720A_ADDR,reg, regvaluea);
}
void LAN8720A_StartAutoNego (void)
{
    uint32_t regvalue = 0;
    /* 读取PHY寄存器值 */
    ETH_PHY_ReadReg(LAN8720A_BCR, &regvalue);
    /* 设置自动协商使能位 */
    regvalue |= LAN8720A_BCR_AUTONEGO_EN;
    /* 写回PHY寄存器 */
    ETH_PHY_WriteReg(LAN8720A_BCR, regvalue);
}
/**
  * 函数功能: 获取LAN8720A的连接状态
  * 输入参数: 无
  * 返 回 值: LAN8720A_STATUS_LINK_DOWN           ： 连接断开 
              LAN8720A_STATUS_AUTONEGO_NOTDONE    ： 自动协商完成
              LAN8720A_STATUS_100MBITS_FULLDUPLEX ： 100M全双工
              LAN8720A_STATUS_100MBITS_HALFDUPLEX ： 100M半双工
              LAN8720A_STATUS_10MBITS_FULLDUPLEX  ： 10M全双工
              LAN8720A_STATUS_10MBITS_HALFDUPLEX  ： 10M半双工              
  * 说    明: 无
  */
uint32_t ETH_link_check_state(void)
{
    uint32_t regvalue = 0;
    /* 读取PHY寄存器值 */
    ETH_PHY_ReadReg(LAN8720A_BSR, &regvalue);   /* 检查链接状态位 */
    if ((regvalue & LAN8720A_BSR_LINK_STATUS) == 0)
    {
        return LAN8720A_STATUS_LINK_DOWN; // 链接断开
    }
        /* 获取自动协商状态 */
    ETH_PHY_ReadReg(LAN8720A_BCR,&regvalue);
    if ((regvalue & LAN8720A_BCR_AUTONEGO_EN) != LAN8720A_BCR_AUTONEGO_EN)//自动协商未启用
    {
        if(((regvalue & LAN8720A_BCR_DUPLEX_MODE) == LAN8720A_BCR_DUPLEX_MODE)
        && ((regvalue & LAN8720A_BCR_SPEED_SELECT) == LAN8720A_BCR_SPEED_SELECT))//全双工
        {
            return LAN8720A_STATUS_100MBITS_FULLDUPLEX;
        }else if(((regvalue & LAN8720A_BCR_SPEED_SELECT) == LAN8720A_BCR_SPEED_SELECT))
        {
            return LAN8720A_STATUS_100MBITS_HALFDUPLEX;
        }else if(((regvalue & LAN8720A_BCR_DUPLEX_MODE) == LAN8720A_BCR_DUPLEX_MODE))
        {
            return LAN8720A_STATUS_10MBITS_FULLDUPLEX;
        }else{
            return LAN8720A_STATUS_10MBITS_HALFDUPLEX;
        }
    }else{
        ETH_PHY_ReadReg(LAN8720A_PHYSCSR,&regvalue);//获取自动协商状态
        if(regvalue&LAN8720A_PHYSCSR_AUTONEGO_DONE == 0)
        {
            return LAN8720A_STATUS_AUTONEGO_NOTDONE; //自动协商未完成
        }
        if((regvalue & LAN8720A_PHYSCSR_HCDSPEEDMASK) == LAN8720A_PHYSCSR_100BTX_FD)
        {
            return LAN8720A_STATUS_100MBITS_FULLDUPLEX;//100M全双工
        }else if((regvalue & LAN8720A_PHYSCSR_HCDSPEEDMASK) == LAN8720A_PHYSCSR_100BTX_HD)
        {
            return LAN8720A_STATUS_100MBITS_HALFDUPLEX;//100M半双工
        }else if((regvalue & LAN8720A_PHYSCSR_HCDSPEEDMASK) == LAN8720A_PHYSCSR_10BT_FD)
        {
            return LAN8720A_STATUS_10MBITS_FULLDUPLEX;//10M全双工
        }else
        {
            return LAN8720A_STATUS_10MBITS_HALFDUPLEX;//默认10M半双工
        }
    }
}
#ifdef USE_DHCP
void DHCP_Thread(void const *argument)
{

}
#endif

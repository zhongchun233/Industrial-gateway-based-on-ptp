#include "stm32h7xx_hal.h"
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/timeouts.h"
#include "lwip/tcpip.h"
#include "lwip/snmp.h"
#include "netif/ethernet.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include <string.h>
#include "APP_Ethernetif.h"// 应用层以太网接口
#include "cmsis_os2.h"

#include "shell_port.h"
#define ETH_RX_BUFFER_SIZE                     (1536UL)
#define IFNAME0 's'// 接口名称的第一个字母
#define IFNAME1 't'// 接口名称的第二个字母
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma location=0x30040000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x30040060
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */
#pragma location=0x30040200
uint8_t Rx_Buff[ETH_RX_DESC_CNT][ETH_RX_BUFFER_SIZE]; /* Ethernet Receive Buffers */

#elif defined ( __CC_ARM )  /* MDK ARM Compiler */
// 注意：MDK编译器需要使用属性指定内存位置
// 确保这些缓冲区位置与链接脚本中的正确地址
__attribute__((at(0x30040000))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((at(0x30040060))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */
__attribute__((at(0x30040200))) uint8_t Rx_Buff[ETH_RX_DESC_CNT][ETH_RX_BUFFER_SIZE]; /* Ethernet Receive Buffer */
#endif
LWIP_MEMPOOL_DECLARE(RX_POOL, 20, sizeof(struct pbuf_custom), "Zero-copy RX PBUF pool");//零拷贝接收池

/* 私有变量 ------------------------------------------------------------------*/

uint32_t current_pbuf_idx = 0;// 当前使用的pbuf索引
ETH_HandleTypeDef heth;// 以太网句柄
ETH_TxPacketConfig TxConfig;// 以太网发送配置结构体

osSemaphoreId_t  RxPktSemaphore = NULL; /* Semaphore to signal incoming packets *///接收信号量

// sys_sem_t tx_sem;// 发送信号量
// sys_mbox_t eth_tx_mb = NULL;// 以太网发送消息邮箱

osSemaphoreId_t  sem1;// 信号量

/**
 * @brief 配置MPU以保护内存区域，确保以太网DMA能够正确访问内存区域
 * @note  此函数专门用于配置以太网DMA内存访问的正确性和一致性
 */
void NET_MPU_config(void)
{
    // 首先禁用当前的MPU
    HAL_MPU_Disable();
    // 1. 配置以太网相关的内存区域为非缓存
    MPU_Region_InitTypeDef MPU_InitStruct = {0};
    // 使用MPU区域
    MPU_InitStruct.Enable = MPU_REGION_ENABLE;
    // 设置内存起始地址为0x30040000 (位于D2 SRAM的ETH专用RAM)
    MPU_InitStruct.BaseAddress = 0x30040000;
    MPU_InitStruct.Size = MPU_REGION_SIZE_256B; // 以太网使用256B内存
    // 设置权限：全访问(可读可写)
    MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
    // 设置缓冲：为DMA使用缓冲开启
    MPU_InitStruct.IsBufferable = MPU_ACCESS_BUFFERABLE;
    // 禁止Cache缓存，防止ETH DMA无法访问到Cache中的RAM数据，导致数据异常
    MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
    // 禁止共享
    MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
    // 使用MPU区域0
    MPU_InitStruct.Number = MPU_REGION_NUMBER0;
    // 内存类型扩展属性，默认LEVEL0
    MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
    // 子区域禁用(全部使用)
    MPU_InitStruct.SubRegionDisable = 0x00;
    // 禁止取指执行(不影响数据访问)
    MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
    HAL_MPU_ConfigRegion(&MPU_InitStruct);
    // 使用MPU并使用特权默认权限
    HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}
int32_t YS_LAN8720_Init(void)
{
    uint32_t regValue;
    int32_t timeout=0;
    uint32_t phylink=0;
    int32_t status=LAN8720A_STATUS_OK;
    uint8_t macaddress[6]= {ETH_MAC_ADDR0, ETH_MAC_ADDR1, ETH_MAC_ADDR2, ETH_MAC_ADDR3, ETH_MAC_ADDR4, ETH_MAC_ADDR5};
    // 1. 配置 LAN8720 和 MAC 地址


    NET_MPU_config(); // 配置MPU以确保以太网DMA能够正确访问内存区域
    // 2. 初始化以太网驱动
    ETH_HandleTypeDef heth;

    heth.Instance = ETH;
    heth.Init.MediaInterface = HAL_ETH_RMII_MODE;// 使用RMII接口
    heth.Init.MACAddr = macaddress;// MAC 地址
    heth.Init.TxDesc = DMATxDscrTab;// 发送描述符
    heth.Init.RxDesc = DMARxDscrTab;// 接收描述符
    heth.Init.RxBuffLen = ETH_RX_BUFFER_SIZE;// 接收缓冲区大小

    if (HAL_ETH_Init(&heth) != HAL_OK)
    {
        Error_Handler();
    }
    HAL_ETH_SetMDIOClockRange(&heth); // 设置MDIO时钟范围
    // 3. 配置以太网PHY
        /* LAN8720A软件复位 */
    if(ETH_PHY_WriteReg(LAN8720A_BCR,LAN8720A_BCR_SOFT_RESET)>=0)
    {
        if(ETH_PHY_ReadReg(LAN8720A_BCR,&regValue) != HAL_OK)
        {
            status=LAN8720A_STATUS_WRITE_ERROR; // 写错误
        }else
        {
            while(regValue & LAN8720A_BCR_SOFT_RESET)
            {
                if(ETH_PHY_ReadReg(LAN8720A_BCR,&regValue) != HAL_OK)
                {
                    status=LAN8720A_STATUS_READ_ERROR; // 读取错误
                    break;
                }
                HAL_Delay(1); // 等待复位完成
                if(++timeout>100)
                {
                    status=LAN8720A_STATUS_RESET_TIMEOUT; // 复位超时
                    break;
                }
            }

        }
    }else
    {
        status=LAN8720A_STATUS_WRITE_ERROR; // 写错误
    }

    LAN8720A_StartAutoNego();                /*启动自动协商功能 */

    if(status==LAN8720A_STATUS_OK)           /* 如果前面初始化成功，等待1s */
        HAL_Delay(1000);                     /* 等待1s */
    while(ETH_link_check_state()<=LAN8720A_STATUS_LINK_DOWN)
    {
        HAL_Delay(10);
        timeout++;
        if(timeout>=LAN8720A_TIMEOUT)
        {
            status=LAN8720A_STATUS_LINK_DOWN;
            break; /* 超时退出,5S */
        }
    }
    phylink=ETH_link_check_state();
    if(phylink==LAN8720A_STATUS_100MBITS_FULLDUPLEX)
        debugShellPrintf("LAN8720A:100Mb/s FullDuplex\r\n");
    else if(phylink==LAN8720A_STATUS_100MBITS_HALFDUPLEX)
        debugShellPrintf("LAN8720A:100Mb/s HalfDuplex\r\n");
    else if(phylink==LAN8720A_STATUS_10MBITS_FULLDUPLEX)
        debugShellPrintf("LAN8720A:10Mb/s FullDuplex\r\n");
    else if(phylink==LAN8720A_STATUS_10MBITS_HALFDUPLEX)
        debugShellPrintf("LAN8720A:10Mb/s HalfDuplex\r\n");
    return status; // 返回状态
}
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
  osSemaphoreRelease(RxPktSemaphore);// 接收完成后释放信号量，通知以太网接收线程继续处理
}
/**
 * 自定义释放接收缓冲区的函数
 * H7 专用，因为有 Cache
 * 用法：释放pbuf前先清除CPU缓存，防止DMA数据不一致
 */
void pbuf_free_custom(struct pbuf *p)
{
  struct pbuf_custom* custom_pbuf = (struct pbuf_custom*)p;

  /* 清除数据缓存，防止 DMA 和 CPU 数据不一致，H7专用！*/
  SCB_InvalidateDCache_by_Addr((uint32_t *)p->payload, p->tot_len);

  /* 将pbuf归还内存池，使用动态内存管理*/
  LWIP_MEMPOOL_FREE(RX_POOL, custom_pbuf);
}
static struct pbuf * low_level_input(struct netif *netif)
{
  struct pbuf *p = NULL;
  void *pAppBuff = NULL;
  uint32_t framelength = 0;
  struct pbuf_custom* custom_pbuf;

  if (HAL_ETH_ReadData(&heth, &pAppBuff) == HAL_OK)
  {
    // 清除和无效化数据缓存
    SCB_CleanInvalidateDCache();

    framelength = heth.RxDescList.RxDataLength;

    custom_pbuf = (struct pbuf_custom*)LWIP_MEMPOOL_ALLOC(RX_POOL);
    if (custom_pbuf != NULL)
    {
      custom_pbuf->custom_free_function = pbuf_free_custom;
      p = pbuf_alloced_custom(PBUF_RAW, framelength, PBUF_REF, custom_pbuf, pAppBuff, ETH_RX_BUFFER_SIZE);
    }
  }
  return p;
}
/**
 * @brief 以太网接口接收处理函数
 * @param argument 接口(netif)结构体指针
 * @note 该函数是一个无限循环，在其中不断接收以太网数据包
 */
void ethernetif_input(void const * argument)
{
  struct pbuf *p;          // 用于存储接收的数据包指针
  struct netif *netif = (struct netif *) argument;  // 从参数中获取网络接口结构体指针

  for( ;; )  // 无限循环，不断接收数据包
  {
    // 等待接收数据包信号量
    if (osSemaphoreAcquire(RxPktSemaphore, osWaitForever) == osOK)
    {
      /* move received packet into a new pbuf */
      osKernelLock();
TRY_GET_NEXT_FRAGMENT:
      p = low_level_input(netif);
      osKernelUnlock();
      /* points to packet payload, which starts with an Ethernet header */
      if(p != NULL)
      {
        osKernelLock();
        if (netif->input(p, netif) != ERR_OK)
        {
          LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
          pbuf_free(p);
          p = NULL;
        }
        else
        {
          goto TRY_GET_NEXT_FRAGMENT;
        }
        osKernelUnlock();
      }
    }
  }

}
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    uint32_t i=0, framelen = 0;
    struct pbuf *q;
    err_t errval = ERR_OK;
    ETH_BufferTypeDef Txbuffer[ETH_TX_DESC_CNT];

    memset(Txbuffer,0,ETH_TX_DESC_CNT *sizeof(ETH_BufferTypeDef));

    for(q = p;q !=NULL;q = q->next )// 遍历pbuf链表，将数据放到发送缓冲区
    {
        if(i >= ETH_TX_DESC_CNT)
            return ERR_IF;
        Txbuffer[i].buffer = (uint8_t*)q->payload;
        Txbuffer[i].len = q->len;
        framelen += q->len;

        if(i>0)
        {
            Txbuffer[i-1].next = &Txbuffer[i];
        }
        if(q->next == NULL)
        {
            Txbuffer[i].next = NULL;
        }
        i++;
    }
    TxConfig.Length = framelen;
    TxConfig.TxBuffer = Txbuffer;

    SCB_CleanInvalidateDCache();// 清除前面的缓存，确保DMA能够读到正确的数据，H7专用！

    HAL_ETH_Transmit_IT(&heth, &TxConfig);

    return errval;
}

static err_t low_level_init(struct netif *netif)
{
    uint32_t idx = 0;// 循环计数
    ETH_MACConfigTypeDef MACConf={0};// MAC配置结构体
    uint32_t PHYLinkState;// PHY链路状态
    uint32_t speed=0,duplex=0;// 链路速度和双工模式
    /* 设置MAC地址 */
    netif->hwaddr_len = ETH_HWADDR_LEN;
    netif->hwaddr[0] = ETH_MAC_ADDR0;
    netif->hwaddr[1] = ETH_MAC_ADDR1;
    netif->hwaddr[2] = ETH_MAC_ADDR2;
    netif->hwaddr[3] = ETH_MAC_ADDR3;
    netif->hwaddr[4] = ETH_MAC_ADDR4;
    netif->hwaddr[5] = ETH_MAC_ADDR5;
    netif->mtu=ETH_MAX_PAYLOAD;/* 设置最大传输单元,支持广播和ARP协议 */

    netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP ;// 支持广播和ARP协议

    memset(&TxConfig,0,sizeof(ETH_TxPacketConfig));
    TxConfig.Attributes=ETH_TX_PACKETS_FEATURES_CSUM|ETH_TX_PACKETS_FEATURES_CRCPAD;// 启用校验和和CRC/PAD功能
    TxConfig.ChecksumCtrl=ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;// 计算IP头和数据的校验和,MAC计算
    TxConfig.CRCPadCtrl=ETH_CRC_PAD_INSERT;// 设置CRC和填充

    LWIP_MEMPOOL_INIT(RX_POOL);// 初始化内存池
    RxPktSemaphore = osSemaphoreNew(1, 1, NULL);// 创建一个二值信号量,初始值为1,最大值为1,用于以太网接收线程同步

    osThreadNew(ethernetif_input, netif, NULL);// 创建以太网接收线程,将netif结构体指针作为参数

    PHYLinkState=ETH_link_check_state();
    switch (PHYLinkState)
    {
        case LAN8720A_STATUS_100MBITS_FULLDUPLEX:    /* 100M全双工 */
            duplex=ETH_FULLDUPLEX_MODE;
            speed=ETH_SPEED_100M;
            break;
        case LAN8720A_STATUS_100MBITS_HALFDUPLEX:    /* 100M半双工 */
            duplex=ETH_HALFDUPLEX_MODE;
            speed=ETH_SPEED_100M;
            break;
        case LAN8720A_STATUS_10MBITS_FULLDUPLEX:     /* 10M全双工 */
            duplex=ETH_FULLDUPLEX_MODE;
            speed=ETH_SPEED_10M;
            break;
        case LAN8720A_STATUS_10MBITS_HALFDUPLEX:     /* 10M半双工 */
            duplex=ETH_HALFDUPLEX_MODE;
            speed=ETH_SPEED_10M;
            break;
        default:
            break;
    }

    HAL_ETH_GetMACConfig(&heth,&MACConf); // 获取MAC配置
    MACConf.DuplexMode=duplex;// 设置双工模式
    MACConf.Speed=speed;// 设置速度

    HAL_ETH_SetMACConfig(&heth,&MACConf);       /* 设置MAC */
    HAL_ETH_Start_IT(&heth);// 启动以太网接收中断0

    netif_set_up(netif);                        /* 设置网络 */
    netif_set_link_up(netif);                   /* 设置网络链接 */
    return ERR_OK;
}

/*初始化以太网接口 注意链接脚本*/
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */

#if LWIP_IPV4
#if LWIP_ARP || LWIP_ETHERNET
#if LWIP_ARP
  netif->output = etharp_output;
#else
  /* The user should write ist own code in low_level_output_arp_off function */
  netif->output = low_level_output_arp_off;
#endif /* LWIP_ARP */
#endif /* LWIP_ARP || LWIP_ETHERNET */
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */

  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

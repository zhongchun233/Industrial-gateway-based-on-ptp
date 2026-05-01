 #include "w25q128.h"
 #include "quadspi.h"
 #include <string.h>

  extern QSPI_HandleTypeDef hqspi;

 /* cc936 运行时状态，由 CC936_Init() 填充，供 cc936.c 中的 ff_convert 使用 */
 uint8_t        cc936_ready     = 0;
 uint32_t       cc936_uni2oem_pairs = 0;
 uint32_t       cc936_oem2uni_pairs = 0;
 const uint16_t *cc936_p_uni2oem   = NULL;
 const uint16_t *cc936_p_oem2uni   = NULL;

 /* -------------------------------------------------------------------------- */
 /* 基础命令层                                                                  */
 /* -------------------------------------------------------------------------- */

 void W25Q128_Init(void)
 {
     uint32_t id = W25Q128_ReadID();
     (void)id;  /* 可在此处加断言: assert(id == 0xEF4018) */
 }

 uint32_t W25Q128_ReadID(void)
 {
     QSPI_CommandTypeDef cmd = {0};
     uint8_t id[3] = {0};

     cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
     cmd.Instruction       = W25Q128_JEDEC_ID;
     cmd.AddressMode       = QSPI_ADDRESS_NONE;
     cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
     cmd.DataMode          = QSPI_DATA_1_LINE;
     cmd.DummyCycles       = 0;
     cmd.NbData            = 3;
     cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
     cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

     HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
     HAL_QSPI_Receive(&hqspi, id, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
     return (id[0] << 16) | (id[1] << 8) | id[2];
 }

 void W25Q128_WaitBusy(void)
 {
     QSPI_CommandTypeDef cmd = {0};
     uint8_t status = 0;

     cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
     cmd.Instruction       = W25Q128_READ_STATUS_REG1;
     cmd.AddressMode       = QSPI_ADDRESS_NONE;
     cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
     cmd.DataMode          = QSPI_DATA_1_LINE;
     cmd.DummyCycles       = 0;
     cmd.NbData            = 1;
     cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
     cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

     do {
         HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
         HAL_QSPI_Receive(&hqspi, &status, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
     } while (status & 0x01);
 }

 static void W25Q128_WriteEnable(void)
 {
     QSPI_CommandTypeDef cmd = {0};

     cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
     cmd.Instruction       = W25Q128_WRITE_ENABLE;
     cmd.AddressMode       = QSPI_ADDRESS_NONE;
     cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
     cmd.DataMode          = QSPI_DATA_NONE;
     cmd.DummyCycles       = 0;
     cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
     cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

     HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
 }

 void W25Q128_Read(uint32_t addr, uint8_t *buf, uint32_t len)
 {
     QSPI_CommandTypeDef cmd = {0};

     cmd.InstructionMode      = QSPI_INSTRUCTION_1_LINE;
     cmd.Instruction          = W25Q128_FAST_READ_QUAD_IO;
     cmd.AddressMode          = QSPI_ADDRESS_4_LINES;
     cmd.AddressSize          = QSPI_ADDRESS_24_BITS;
     cmd.Address              = addr;
     cmd.AlternateByteMode    = QSPI_ALTERNATE_BYTES_4_LINES;
     cmd.AlternateBytesSize   = QSPI_ALTERNATE_BYTES_8_BITS;
     cmd.AlternateBytes       = 0xFF;
     cmd.DataMode             = QSPI_DATA_4_LINES;
     cmd.DummyCycles          = 4;
     cmd.NbData               = len;
     cmd.DdrMode              = QSPI_DDR_MODE_DISABLE;
     cmd.SIOOMode             = QSPI_SIOO_INST_EVERY_CMD;

     HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
     HAL_QSPI_Receive(&hqspi, buf, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
 }

 void W25Q128_Write(uint32_t addr, uint8_t *buf, uint32_t len)
 {
     uint32_t end_addr     = addr + len;
     uint32_t current_addr = addr;

     while (current_addr < end_addr) {
         uint32_t page_end  = (current_addr | (W25Q128_PAGE_SIZE - 1)) + 1;
         uint32_t write_len = (page_end < end_addr) ? (page_end - current_addr)
                                                      : (end_addr - current_addr);

         W25Q128_WriteEnable();

         QSPI_CommandTypeDef cmd = {0};
         cmd.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
         cmd.Instruction        = W25Q128_QUAD_PAGE_PROGRAM;
         cmd.AddressMode        = QSPI_ADDRESS_1_LINE;
         cmd.AddressSize        = QSPI_ADDRESS_24_BITS;
         cmd.Address            = current_addr;
         cmd.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
         cmd.DataMode           = QSPI_DATA_4_LINES;
         cmd.DummyCycles        = 0;
         cmd.NbData             = write_len;
         cmd.DdrMode            = QSPI_DDR_MODE_DISABLE;
         cmd.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;

         HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
         HAL_QSPI_Transmit(&hqspi, buf + (current_addr - addr),
                           HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
         W25Q128_WaitBusy();

         current_addr += write_len;
     }
 }

 void W25Q128_EraseSector(uint32_t addr)
 {
     W25Q128_WriteEnable();

     QSPI_CommandTypeDef cmd = {0};
     cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
     cmd.Instruction       = W25Q128_SECTOR_ERASE_4K;
     cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
     cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
     cmd.Address           = addr;
     cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
     cmd.DataMode          = QSPI_DATA_NONE;
     cmd.DummyCycles       = 0;
     cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
     cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

     HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
     W25Q128_WaitBusy();
 }

 void W25Q128_EraseBlock(uint32_t addr)
 {
     W25Q128_WriteEnable();

     QSPI_CommandTypeDef cmd = {0};
     cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
     cmd.Instruction       = W25Q128_BLOCK_ERASE_64K;
     cmd.AddressMode       = QSPI_ADDRESS_1_LINE;
     cmd.AddressSize       = QSPI_ADDRESS_24_BITS;
     cmd.Address           = addr;
     cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
     cmd.DataMode          = QSPI_DATA_NONE;
     cmd.DummyCycles       = 0;
     cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
     cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

     HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
     W25Q128_WaitBusy();
 }

 void W25Q128_EraseChip(void)
 {
     W25Q128_WriteEnable();

     QSPI_CommandTypeDef cmd = {0};
     cmd.InstructionMode   = QSPI_INSTRUCTION_1_LINE;
     cmd.Instruction       = W25Q128_CHIP_ERASE;
     cmd.AddressMode       = QSPI_ADDRESS_NONE;
     cmd.AlternateByteMode = QSPI_ALTERNATE_BYTES_NONE;
     cmd.DataMode          = QSPI_DATA_NONE;
     cmd.DummyCycles       = 0;
     cmd.DdrMode           = QSPI_DDR_MODE_DISABLE;
     cmd.SIOOMode          = QSPI_SIOO_INST_EVERY_CMD;

     HAL_QSPI_Command(&hqspi, &cmd, HAL_QPSI_TIMEOUT_DEFAULT_VALUE);
     W25Q128_WaitBusy();
 }

 /* -------------------------------------------------------------------------- */
 /* 内存映射模式                                                                 */
 /* -------------------------------------------------------------------------- */

 void W25Q128_EnterMemoryMapped(void)
 {
     QSPI_CommandTypeDef      cmd = {0};
     QSPI_MemoryMappedTypeDef cfg = {0};

     cmd.InstructionMode    = QSPI_INSTRUCTION_1_LINE;
     cmd.Instruction        = W25Q128_FAST_READ_QUAD_IO;
     cmd.AddressMode        = QSPI_ADDRESS_4_LINES;
     cmd.AddressSize        = QSPI_ADDRESS_24_BITS;
     cmd.AlternateByteMode  = QSPI_ALTERNATE_BYTES_4_LINES;
     cmd.AlternateBytesSize = QSPI_ALTERNATE_BYTES_8_BITS;
     cmd.AlternateBytes     = 0xFF;
     cmd.DataMode           = QSPI_DATA_4_LINES;
     cmd.DummyCycles        = 4;
     cmd.DdrMode            = QSPI_DDR_MODE_DISABLE;
     cmd.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;

     cfg.TimeOutActivation = QSPI_TIMEOUT_COUNTER_DISABLE;

     HAL_QSPI_MemoryMapped(&hqspi, &cmd, &cfg);
 }

 void W25Q128_ExitMemoryMapped(void)
 {
     HAL_QSPI_Abort(&hqspi);
 }

 /* -------------------------------------------------------------------------- */
 /* CC936 初始化                                                                 */
 /* -------------------------------------------------------------------------- */

 /*
  * CC936_Init: 检测 W25Q128 中是否已有合法的 cc936 数据。
  *   成功: 进入内存映射模式，设置 ff_convert 所需的全局指针，cc936_ready=1
  *   失败: cc936_ready=0，ff_convert 对非 ASCII 字符返回 0 (不崩溃)
  *
  * 注意: 调用前必须确保不在内存映射模式（首次启动或调用 ExitMemoryMapped 后）
  */
 void CC936_Init(void)
 {
     cc936_ready = 0;
     cc936_p_uni2oem = NULL;
     cc936_p_oem2uni = NULL;

     /* 通过普通命令读魔数 (16 字节: 8 magic + 4 uni2oem_pairs + 4 oem2uni_pairs) */
     uint8_t header[16];
     W25Q128_Read(CC936_W25Q128_OFFSET, header, sizeof(header));

     if (memcmp(header, CC936_MAGIC, 8) != 0) {
         return;  /* 未烧录或数据损坏 */
     }

     uint32_t u_pairs = header[8]  | ((uint32_t)header[9]  << 8)
                      | ((uint32_t)header[10] << 16) | ((uint32_t)header[11] << 24);
     uint32_t o_pairs = header[12] | ((uint32_t)header[13] << 8)
                      | ((uint32_t)header[14] << 16) | ((uint32_t)header[15] << 24);

     /* 合理性检查 */
     if (u_pairs == 0 || u_pairs > 30000 || o_pairs == 0 || o_pairs > 30000) {
         return;
     }

     /* 进入内存映射模式，W25Q128 可见于 0x90000000 */
     W25Q128_EnterMemoryMapped();

     cc936_uni2oem_pairs = u_pairs;
     cc936_oem2uni_pairs = o_pairs;

     /* 数据紧接在 16 字节头之后 */
     cc936_p_uni2oem = (const uint16_t *)(W25Q128_MAPPED_BASE + CC936_W25Q128_OFFSET + 16);
     cc936_p_oem2uni = (const uint16_t *)(W25Q128_MAPPED_BASE + CC936_W25Q128_OFFSET + 16
                                           + u_pairs * 4U);
     cc936_ready = 1;
 }

 uint8_t CC936_IsReady(void)
 {
     return cc936_ready;
 }

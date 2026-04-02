#include "bsp_oled.h"
#include <string.h>

/* g_oled_buffer（OLED 显存缓冲区）
 * 说明：
 * OLED 常见分 8 页，每页 128 列，每个字节对应纵向 8 个像素
 * 所有显示内容先写入缓冲区，最后统一刷新到屏幕
 */
static uint8_t g_oled_buffer[OLED_PAGE_NUM][OLED_WIDTH];

/* BSP_OLED_WriteCmd（发送命令）
 * 作用：向 OLED 发送控制命令
 */
static void BSP_OLED_WriteCmd(uint8_t cmd)
{
    uint8_t buf[2];

    buf[0] = 0x00;   /* 控制字节：0x00 表示后面是命令 */
    buf[1] = cmd;

    HAL_I2C_Master_Transmit(&OLED_I2C_HANDLE, OLED_I2C_ADDR, buf, 2, 100);
}

/* BSP_OLED_WriteDataBlock（发送一块数据）
 * 作用：向 OLED 连续发送数据字节
 */
static void BSP_OLED_WriteDataBlock(uint8_t *data, uint16_t len)
{
    uint8_t tx_buf[129];
    uint16_t i;

    if (len > 128)
    {
        len = 128;
    }

    tx_buf[0] = 0x40;   /* 控制字节：0x40 表示后面是数据 */

    for (i = 0; i < len; i++)
    {
        tx_buf[i + 1] = data[i];
    }

    HAL_I2C_Master_Transmit(&OLED_I2C_HANDLE, OLED_I2C_ADDR, tx_buf, len + 1, 100);
}

/* BSP_OLED_SetPos（设置页地址和列地址） */
static void BSP_OLED_SetPos(uint8_t page, uint8_t col)
{
    BSP_OLED_WriteCmd(0xB0 + page);                 /* 设置页地址 */
    BSP_OLED_WriteCmd(0x10 | ((col >> 4) & 0x0F)); /* 设置列高 4 位 */
    BSP_OLED_WriteCmd(0x00 | (col & 0x0F));        /* 设置列低 4 位 */
}

/* BSP_OLED_GetFont6x8（获取 6x8 字符点阵）
 * 说明：
 * 1. 当前阶段只支持你页面里会用到的字符
 * 2. 每个字符占 6 列，其中前 5 列是字形，第 6 列留空作间隔
 * 3. 如果后面要扩展中文或完整 ASCII，再单独补字库
 */
static const uint8_t *BSP_OLED_GetFont6x8(char ch)
{
    static const uint8_t font_space[6] = {0x00,0x00,0x00,0x00,0x00,0x00};
    static const uint8_t font_dot[6]   = {0x00,0x60,0x60,0x00,0x00,0x00};
    static const uint8_t font_colon[6] = {0x00,0x36,0x36,0x00,0x00,0x00};
    static const uint8_t font_dash[6]  = {0x08,0x08,0x08,0x08,0x08,0x00};
    static const uint8_t font_line[6]  = {0x80,0x80,0x80,0x80,0x80,0x00};
    static const uint8_t font_percent[6] = {0x62,0x64,0x08,0x13,0x23,0x00};

    static const uint8_t font_0[6] = {0x3E,0x51,0x49,0x45,0x3E,0x00};
    static const uint8_t font_1[6] = {0x00,0x42,0x7F,0x40,0x00,0x00};
    static const uint8_t font_2[6] = {0x42,0x61,0x51,0x49,0x46,0x00};
    static const uint8_t font_3[6] = {0x21,0x41,0x45,0x4B,0x31,0x00};
    static const uint8_t font_4[6] = {0x18,0x14,0x12,0x7F,0x10,0x00};
    static const uint8_t font_5[6] = {0x27,0x45,0x45,0x45,0x39,0x00};
    static const uint8_t font_6[6] = {0x3C,0x4A,0x49,0x49,0x30,0x00};
    static const uint8_t font_7[6] = {0x01,0x71,0x09,0x05,0x03,0x00};
    static const uint8_t font_8[6] = {0x36,0x49,0x49,0x49,0x36,0x00};
    static const uint8_t font_9[6] = {0x06,0x49,0x49,0x29,0x1E,0x00};

    static const uint8_t font_A[6] = {0x7E,0x11,0x11,0x11,0x7E,0x00};
    static const uint8_t font_B[6] = {0x7F,0x49,0x49,0x49,0x36,0x00};
    static const uint8_t font_C[6] = {0x3E,0x41,0x41,0x41,0x22,0x00};
    static const uint8_t font_D[6] = {0x7F,0x41,0x41,0x22,0x1C,0x00};
    static const uint8_t font_E[6] = {0x7F,0x49,0x49,0x49,0x41,0x00};
    static const uint8_t font_F[6] = {0x7F,0x09,0x09,0x09,0x01,0x00};
    static const uint8_t font_G[6] = {0x3E,0x41,0x49,0x49,0x7A,0x00};
    static const uint8_t font_H[6] = {0x7F,0x08,0x08,0x08,0x7F,0x00};
    static const uint8_t font_I[6] = {0x00,0x41,0x7F,0x41,0x00,0x00};
    static const uint8_t font_J[6] = {0x20,0x40,0x41,0x3F,0x01,0x00};
    static const uint8_t font_K[6] = {0x7F,0x08,0x14,0x22,0x41,0x00};
    static const uint8_t font_L[6] = {0x7F,0x40,0x40,0x40,0x40,0x00};
    static const uint8_t font_M[6] = {0x7F,0x02,0x0C,0x02,0x7F,0x00};
    static const uint8_t font_N[6] = {0x7F,0x04,0x08,0x10,0x7F,0x00};
    static const uint8_t font_O[6] = {0x3E,0x41,0x41,0x41,0x3E,0x00};
    static const uint8_t font_P[6] = {0x7F,0x09,0x09,0x09,0x06,0x00};
    static const uint8_t font_Q[6] = {0x3E,0x41,0x51,0x21,0x5E,0x00};
    static const uint8_t font_R[6] = {0x7F,0x09,0x19,0x29,0x46,0x00};
    static const uint8_t font_S[6] = {0x46,0x49,0x49,0x49,0x31,0x00};
    static const uint8_t font_T[6] = {0x01,0x01,0x7F,0x01,0x01,0x00};
    static const uint8_t font_U[6] = {0x3F,0x40,0x40,0x40,0x3F,0x00};
    static const uint8_t font_V[6] = {0x1F,0x20,0x40,0x20,0x1F,0x00};
    static const uint8_t font_W[6] = {0x7F,0x20,0x18,0x20,0x7F,0x00};
    static const uint8_t font_X[6] = {0x63,0x14,0x08,0x14,0x63,0x00};
    static const uint8_t font_Y[6] = {0x03,0x04,0x78,0x04,0x03,0x00};
    static const uint8_t font_Z[6] = {0x61,0x51,0x49,0x45,0x43,0x00};

    switch (ch)
    {
        case ' ': return font_space;
        case '.': return font_dot;
        case ':': return font_colon;
        case '-': return font_dash;
        case '_': return font_line;
        case '%': return font_percent;

        case '0': return font_0;
        case '1': return font_1;
        case '2': return font_2;
        case '3': return font_3;
        case '4': return font_4;
        case '5': return font_5;
        case '6': return font_6;
        case '7': return font_7;
        case '8': return font_8;
        case '9': return font_9;

        case 'A': return font_A;
        case 'B': return font_B;
        case 'C': return font_C;
        case 'D': return font_D;
        case 'E': return font_E;
        case 'F': return font_F;
        case 'G': return font_G;
        case 'H': return font_H;
        case 'I': return font_I;
        case 'J': return font_J;
        case 'K': return font_K;
        case 'L': return font_L;
        case 'M': return font_M;
        case 'N': return font_N;
        case 'O': return font_O;
        case 'P': return font_P;
        case 'Q': return font_Q;
        case 'R': return font_R;
        case 'S': return font_S;
        case 'T': return font_T;
        case 'U': return font_U;
        case 'V': return font_V;
        case 'W': return font_W;
        case 'X': return font_X;
        case 'Y': return font_Y;
        case 'Z': return font_Z;

        default:
            return font_space;
    }
}

/* BSP_OLED_Init（OLED 初始化）
 * 说明：
 * 这里按常见 SSD1306 0.96寸 128x64 I2C 屏初始化
 * 如果后面发现你的模块是 SH1106，也只需要小改初始化和列地址逻辑
 */
void BSP_OLED_Init(void)
{
    HAL_Delay(100);

    BSP_OLED_WriteCmd(0xAE); /* 关闭显示 */

    BSP_OLED_WriteCmd(0x20); /* 设置内存寻址模式 */
    BSP_OLED_WriteCmd(0x10); /* 页寻址模式 */

    BSP_OLED_WriteCmd(0xB0); /* 页起始地址 */
    BSP_OLED_WriteCmd(0xC8); /* COM 扫描方向 remap */
    BSP_OLED_WriteCmd(0x00); /* 列低地址 */
    BSP_OLED_WriteCmd(0x10); /* 列高地址 */

    BSP_OLED_WriteCmd(0x40); /* 起始行地址 */
    BSP_OLED_WriteCmd(0x81); /* 对比度控制 */
    BSP_OLED_WriteCmd(0x7F);

    BSP_OLED_WriteCmd(0xA1); /* 段重映射 */
    BSP_OLED_WriteCmd(0xA6); /* 正常显示 */

    BSP_OLED_WriteCmd(0xA8); /* 多路复用率 */
    BSP_OLED_WriteCmd(0x3F);

    BSP_OLED_WriteCmd(0xA4); /* 显示跟随 RAM */
    BSP_OLED_WriteCmd(0xD3); /* 显示偏移 */
    BSP_OLED_WriteCmd(0x00);

    BSP_OLED_WriteCmd(0xD5); /* 时钟分频 */
    BSP_OLED_WriteCmd(0xF0);

    BSP_OLED_WriteCmd(0xD9); /* 预充电周期 */
    BSP_OLED_WriteCmd(0x22);

    BSP_OLED_WriteCmd(0xDA); /* COM 硬件配置 */
    BSP_OLED_WriteCmd(0x12);

    BSP_OLED_WriteCmd(0xDB); /* VCOMH 电压倍率 */
    BSP_OLED_WriteCmd(0x20);

    BSP_OLED_WriteCmd(0x8D); /* 电荷泵设置 */
    BSP_OLED_WriteCmd(0x14);

    BSP_OLED_WriteCmd(0xAF); /* 开启显示 */

    BSP_OLED_Clear();
    BSP_OLED_Refresh();
}

/* BSP_OLED_Clear（清空缓冲区） */
void BSP_OLED_Clear(void)
{
    memset(g_oled_buffer, 0, sizeof(g_oled_buffer));
}

/* BSP_OLED_Fill（整屏填充） */
void BSP_OLED_Fill(uint8_t fill_data)
{
    memset(g_oled_buffer, fill_data, sizeof(g_oled_buffer));
}

/* BSP_OLED_Refresh（刷新到屏幕） */
void BSP_OLED_Refresh(void)
{
    uint8_t page;

    for (page = 0; page < OLED_PAGE_NUM; page++)
    {
        BSP_OLED_SetPos(page, 0);
        BSP_OLED_WriteDataBlock(g_oled_buffer[page], OLED_WIDTH);
    }
}

/* BSP_OLED_ShowChar（显示单个字符）
 * 说明：
 * 1. 当前采用 6x8 英文数字字库
 * 2. x 为列坐标，page 为页坐标
 */
void BSP_OLED_ShowChar(uint8_t x, uint8_t page, char ch)
{
    const uint8_t *font;
    uint8_t i;

    if ((page >= OLED_PAGE_NUM) || (x > (OLED_WIDTH - 6)))
    {
        return;
    }

    font = BSP_OLED_GetFont6x8(ch);

    for (i = 0; i < 6; i++)
    {
        g_oled_buffer[page][x + i] = font[i];
    }
}

/* BSP_OLED_ShowString（显示字符串）
 * 说明：
 * 1. 每个字符宽度固定为 6 像素
 * 2. 当前仅支持字库中已有字符
 */
void BSP_OLED_ShowString(uint8_t x, uint8_t page, const char *str)
{
    if (str == 0)
    {
        return;
    }

    while ((*str != '\0') && (x <= (OLED_WIDTH - 6)))
    {
        BSP_OLED_ShowChar(x, page, *str);
        x += 6;
        str++;
    }
}

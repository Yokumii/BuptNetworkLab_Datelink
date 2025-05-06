#include <stdio.h>
#include <stdint.h>

// crc32 多项式: x^32 + x^26 + x^23 + x^22 + x^16 + x^12 + x^11 + x^10 + x^8 + x^7 + x^5 + x^4 + x^2 + x + 1
#define CRC32_POLYNOMIAL 0xedb88320

// 生成crc32查表
void generate_crc32_table(uint32_t table[256]) {
    uint32_t crc, i, j;
    
    for (i = 0; i < 256; i++) {
        crc = i;
        
        // 对每一位执行模2除法
        for (j = 0; j < 8; j++) {
            // 检查最低位
            if (crc & 1) {
                crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
            } else {
                crc = crc >> 1;
            }
        }
        
        // 存储结果
        table[i] = crc;
    }
}

int main() {
    uint32_t crc_table[256];
    int i, col = 0;
    
    // 生成crc32表
    generate_crc32_table(crc_table);
    
    // 打印表格
    printf("static const unsigned int crc_table[256] = {\n    ");
    
    // 按指定格式输出
    for (i = 0; i < 256; i++) {
        printf("0x%08xlL", crc_table[i]);
        
        if (i < 255) {
            printf(", ");
        }
        
        col++;
        if (col == 5 && i < 255) {
            printf("\n    ");
            col = 0;
        }
    }
    
    printf("\n};\n");
    
    return 0;
}
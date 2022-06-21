typedef struct {
  uint32_t          scene_vrom_start;         /* 0x0000 */
  uint32_t          scene_vrom_end;           /* 0x0004 */
  uint32_t          title_vrom_start;         /* 0x0008 */
  uint32_t          title_vrom_end;           /* 0x000C */
  char              unk_0x10[0x0001];         /* 0x0010 */
  uint8_t           scene_config;             /* 0x0011 */
  char              unk_0x12[0x0001];         /* 0x0012 */
  char              pad_0x13[0x0001];         /* 0x0013 */
                                              /* 0x0014 */
} z64_scene_table_t;

typedef struct {
  uint8_t code, data1;
  uint8_t padding[2];
  int32_t data2;
} z64_scene_command_t;
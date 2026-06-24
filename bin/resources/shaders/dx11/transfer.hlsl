#define PSMCT32   0 // 0000-0000
#define PSMCT24   1 // 0000-0001
#define PSMCT16   2 // 0000-0010
#define PSMCT16S 10 // 0000-1010
#define PSMT8    19 // 0001-0011
#define PSMT4    20 // 0001-0100
#define PSMT8H   27 // 0001-1011
#define PSMT4HL  36 // 0010-0100
#define PSMT4HH  44 // 0010-1100
#define PSMZ32   48 // 0011-0000
#define PSMZ24   49 // 0011-0001
#define PSMZ16   50 // 0011-0010
#define PSMZ16S  58 // 0011-1010

int2 get_page_size(uint psm)
{
  switch (psm)
  {
    case PSMCT32:
    case PSMCT24:
    case PSMZ32:
    case PSMZ24:
    case PSMT8H:
    case PSMT4HL:
    case PSMT4HH:
    default:
      return int2(64, 32);
    case PSMCT16:
    case PSMCT16S:
    case PSMZ16:
    case PSMZ16S:
      return int2(64, 64);
    case PSMT8:
      return int2(128, 64);
    case PSMT4:
      return int2(128, 128);
  }
}

cbuffer cb : register(b0)
{
  uint DBP;  // Block base pointer.
  uint DBW;  // Buffer width (pixel width  / 64).
  uint DPSM; // Pixel storage format.
  int dx;
  int dy;
  int width;
  int height;
};

RWByteAddressBuffer GSMemory : register(u0);
RWByteAddressBuffer Data     : register(u1);

#define INTERLEAVE_BIT_X(src_bit, dst_bit) ((((xy.x) >> src_bit) & 1) << dst_bit)
#define INTERLEAVE_BIT_Y(src_bit, dst_bit) ((((xy.y) >> src_bit) & 1) << dst_bit)

uint float4_to_word(float4 c)
{
  uint4 u = uint4(c * 255.5f) & 0xFFu;
  return u.r | (u.g << 8) | (u.b << 16) | (u.a << 24);
}

uint float4_to_short(float4 c)
{
  uint4 u = uint4(c * 255.5f) & uint4(0xF8u, 0xF8u, 0xF8u, 0x80u);
  return (u.r >> 3) | (u.g << 2) | (u.b << 7) | (u.a << 8);
}

float4 word_to_float4(uint word)
{
  return float4(word & 0xFFu, (word >> 8) & 0xFFu, (word >> 16) & 0xFFu, (word >> 24) & 0xFFu) / 255.0f;
}

float4 short_to_float4(uint shrt)
{
  return float4((shrt << 3) & 0xF8u, (shrt >> 2) & 0xF8u, (shrt >> 7) & 0xF8u, (shrt >> 8) & 0x80u) / 255.0f;
}

uint LoadWord(uint word_addr)
{
  return GSMemory.Load(word_addr * 4);
}

uint LoadShort(uint short_addr)
{
  return (GSMemory.Load((short_addr * 2) & ~3) >> ((short_addr & 1) * 16)) & 0xFFFFu;
}

uint LoadByte(uint byte_addr)
{
  return (GSMemory.Load(byte_addr & ~3) >> ((byte_addr & 3) * 8)) & 0xFFu;
}

uint LoadNibble(uint nibble_addr)
{
  return (GSMemory.Load((nibble_addr / 2) & ~3) >> ((nibble_addr & 7) * 4)) & 0xFu;
}

uint insert_short(uint dst, uint src, uint pos)
{
  uint shift = (pos & 1) * 16;
  uint mask = 0xFFFFu << shift;
  return (dst & ~mask) | ((src << shift) & mask);
}

uint insert_byte(uint dst, uint src, uint pos)
{
  uint shift = (pos & 3) * 8;
  uint mask = 0xFFu << shift;
  return (dst & ~mask) | ((src << shift) & mask);
}

uint insert_nibble(uint dst, uint src, uint pos)
{
  uint shift = (pos & 7) * 16;
  uint mask = 0xFu << shift;
  return (dst & ~mask) | ((src << shift) & mask);
}

void StoreWord(uint word_addr, uint word)
{
  GSMemory.Store(word_addr * 4, word);
}

void StoreWord24(uint word_addr, uint word)
{
  uint curr = GSMemory.Load(word_addr * 4);
  GSMemory.Store(word_addr * 4, (curr & 0xFF000000u) | (word & 0xFFFFFFu));
}

void StoreShort(uint short_addr, uint shrt)
{
  uint full_addr = (short_addr * 2) & ~3;
  uint curr = GSMemory.Load(full_addr);
  curr = insert_short(curr, shrt, short_addr & 1);
  GSMemory.Store(full_addr, curr);
}

void StoreByte(uint byte_addr, uint byte)
{
  uint full_addr = byte_addr & ~3;
  uint curr = GSMemory.Load(full_addr);
  curr = insert_byte(curr, byte, byte_addr & 3);
  GSMemory.Store(full_addr, curr);
}

void StoreNibble(uint nibble_addr, uint nibble)
{
  uint full_addr = (nibble_addr / 2) & ~3;
  uint curr = GSMemory.Load(full_addr);
  curr = insert_byte(curr, nibble, nibble_addr & 7);
  GSMemory.Store(full_addr, curr);
}

// Word address within a page
uint word_addr_c32(int2 xy)
{
  return INTERLEAVE_BIT_X(5, 10) |
         INTERLEAVE_BIT_Y(4, 9)  |
         INTERLEAVE_BIT_X(4, 8)  |
         INTERLEAVE_BIT_Y(3, 7)  |
         INTERLEAVE_BIT_X(3, 6)  |
         INTERLEAVE_BIT_Y(2, 5)  |
         INTERLEAVE_BIT_Y(1, 4)  |
         INTERLEAVE_BIT_X(2, 3)  |
         INTERLEAVE_BIT_X(1, 2)  |
         INTERLEAVE_BIT_Y(0, 1)  |
         INTERLEAVE_BIT_X(0, 0);
}

uint xor_z32(uint word_addr)
{
  return word_addr ^ (1 << 10) ^ (1 << 9);
}

// Short address in a page
uint short_addr_c16(int2 xy)
{
  return INTERLEAVE_BIT_Y(5, 11) |
         INTERLEAVE_BIT_X(5, 10) |
         INTERLEAVE_BIT_Y(4, 9) |
         INTERLEAVE_BIT_X(4, 8) |
         INTERLEAVE_BIT_Y(3, 7) |
         INTERLEAVE_BIT_Y(2, 6) |
         INTERLEAVE_BIT_Y(1, 5) |
         INTERLEAVE_BIT_X(2, 4) |
         INTERLEAVE_BIT_X(1, 3) |
         INTERLEAVE_BIT_Y(0, 2) |
         INTERLEAVE_BIT_X(0, 1) |
         INTERLEAVE_BIT_X(3, 0);
}

// Short address in a page
uint short_addr_c16s(int2 xy)
{
  return INTERLEAVE_BIT_X(5, 11) |
         INTERLEAVE_BIT_Y(4, 10) |
         INTERLEAVE_BIT_Y(5, 9)  |
         INTERLEAVE_BIT_X(4, 8)  |
         INTERLEAVE_BIT_Y(3, 7)  |
         INTERLEAVE_BIT_Y(2, 6)  |
         INTERLEAVE_BIT_Y(1, 5)  |
         INTERLEAVE_BIT_X(2, 4)  |
         INTERLEAVE_BIT_X(1, 3)  |
         INTERLEAVE_BIT_Y(0, 2)  |
         INTERLEAVE_BIT_X(0, 1)  |
         INTERLEAVE_BIT_X(3, 0);
}

// Short address in a page
uint xor_z16(uint short_addr)
{
  return short_addr ^ (1 << 11) ^ (1 << 10);
}

// Byte address in a page
uint byte_addr_p8(int2 xy)
{
  return INTERLEAVE_BIT_X(6, 12) |
         INTERLEAVE_BIT_Y(5, 11) |
         INTERLEAVE_BIT_X(5, 10) |
         INTERLEAVE_BIT_Y(4, 9)  |
         INTERLEAVE_BIT_X(4, 8)  |
         INTERLEAVE_BIT_Y(3, 7)  |
         INTERLEAVE_BIT_Y(2, 6)  |
         (INTERLEAVE_BIT_X(2, 5) ^ INTERLEAVE_BIT_Y(1, 5) ^ INTERLEAVE_BIT_Y(2, 5))  |
         INTERLEAVE_BIT_X(1, 4)  |
         INTERLEAVE_BIT_Y(0, 3)  |
         INTERLEAVE_BIT_X(0, 2)  |
         INTERLEAVE_BIT_X(3, 1)  |
         INTERLEAVE_BIT_Y(1, 0);
}

// Nibble address in a page
uint nibble_addr_p4(int2 xy)
{
  return INTERLEAVE_BIT_Y(6, 13) |
         INTERLEAVE_BIT_X(6, 12) |
         INTERLEAVE_BIT_Y(5, 11) |
         INTERLEAVE_BIT_X(5, 10) |
         INTERLEAVE_BIT_Y(4, 9)  |
         INTERLEAVE_BIT_Y(3, 8)  |
         INTERLEAVE_BIT_Y(2, 7)  |
         (INTERLEAVE_BIT_X(2, 6) ^ INTERLEAVE_BIT_Y(1, 6) ^ INTERLEAVE_BIT_Y(2, 6)) |
         INTERLEAVE_BIT_X(1, 5)  |
         INTERLEAVE_BIT_Y(0, 4)  |
         INTERLEAVE_BIT_X(0, 3)  |
         INTERLEAVE_BIT_X(4, 2)  |
         INTERLEAVE_BIT_X(3, 1)  |
         INTERLEAVE_BIT_Y(1, 0);
}

[numthreads(16, 16, 1)]
void main(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID)
{
  int2 xy = WorkGroupId.xy * 16 + LocalThreadId.xy; // pixel coords

  if (any(xy < 0) || any(xy >= int2(width, height)))
    return;

  int2 mem_xy = xy + int2(mem_x, mem_y);
  int2 tex_xy = xy + int2(tex_x, tex_y);

  int2 page_size = get_page_size(PSM);

  int2 page_coord = mem_xy / page_size;

  mem_xy -= page_coord * page_size;

  uint buffer_page_width = (BW * 64 + page_size.x - 1) / page_size.x;

  uint page = page_coord.y * buffer_page_width + page_coord.x;
  uint base_word_addr = BP * 64 + page * 2048; // First page

  switch (PSM)
  {
    case PSMCT32:
    case PSMCT24:
    case PSMZ32:
    case PSMZ24:
    {
      uint word_addr = base_word_addr + word_addr_c32(mem_xy);
      if (PSM == PSMZ32 || PSM == PSMZ24)
        word_addr = xor_z32(word_addr);
      if (write)
      {
        float4 input = Texture[tex_xy];
        uint word = float4_to_word(input);
        if (PSM == PSMCT24 || PSM == PSMZ24)
          StoreWord24(word_addr, word);
        else
          StoreWord(word_addr, word);
      }
      else
      {
        uint word = LoadWord(word_addr);
        if (PSM == PSMCT24 || PSM == PSMZ24)
          word &= 0xFFFFFFu;
        float4 color = word_to_float4(word);
        Texture[tex_xy] = color;
      }
    }
    break;
    case PSMT8H:
    case PSMT4HL:
    case PSMT4HH:
    {
      uint word_addr = base_word_addr + word_addr_c32(mem_xy);
      if (!write)
      {
        uint word = LoadWord(word_addr);
        uint shift = PSM == PSMT4HH ? 28 : 24;
        uint mask = PSM == PSMT8H ? 0xFFu : 0xFu;
        uint entry = (word >> shift) & mask;
        float4 color = CLUT[int2(entry, 0)];
        Texture[tex_xy] = color;
      }
    }
    break;
    case PSMCT16:
    case PSMCT16S:
    case PSMZ16:
    case PSMZ16S:
    {
      uint short_addr = base_word_addr * 2;
      if (PSM == PSMCT16 || PSM == PSMZ16)
        short_addr += short_addr_c16(mem_xy);
      else
        short_addr += short_addr_c16s(mem_xy);
      if (PSM == PSMZ16 || PSM == PSMZ16S)
        short_addr = xor_z16(short_addr);
      if (write)
      {
        float4 color = Texture[tex_xy];
        uint shrt = float4_to_short(color);
        StoreShort(short_addr, shrt);
      }
      else
      {
        uint shrt = LoadShort(short_addr);
        float4 color = short_to_float4(shrt);
        Texture[tex_xy] = color;
      }
    }
    break;
    case PSMT8:
    {
      uint byte_addr = base_word_addr * 4 + byte_addr_p8(mem_xy);
      if (!write)
      {
        uint byte = LoadByte(byte_addr);
        float4 color = CLUT[int2(byte, 0)];
        Texture[tex_xy] = color;
      }
    }
    break;
    case PSMT4:
    {
      uint nibble_addr = base_word_addr * 8 + nibble_addr_p4(mem_xy);
      if (!write)
      {
        uint nibble = LoadNibble(nibble_addr);
        float4 color = CLUT[int2(nibble, 0)];
        Texture[tex_xy] = color;
      }
    }
    break;
  }
}

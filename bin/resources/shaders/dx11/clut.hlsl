#define PSMCT32   0 // 0000-0000
#define PSMCT24   1 // 0000-0001
#define PSMCT16   2 // 0000-0010
#define PSMCT16S 10 // 0000-1010
#define PSGPU24  18 // 0001-0010
#define PSMT8    19 // 0001-0011
#define PSMT4    20 // 0001-0100
#define PSMT8H   27 // 0001-1011
#define PSMT4HL  36 // 0010-0100
#define PSMT4HH  44 // 0010-1100
#define PSMZ32   48 // 0011-0000
#define PSMZ24   49 // 0011-0001
#define PSMZ16   50 // 0011-0010
#define PSMZ16S  58 // 0011-1010

#define IDTEX8   0
#define IDTEX4   1
#define CSM1     0
#define CSM2     1
#define INVALID -1

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
  uint PSM;
  uint CBP;  // Block base pointer.
  uint CBW;  // Buffer width (pixel width  / 64).
  uint CPSM; // Pixel storage format.
  uint CSM;  
  uint COU;
  uint COV;
};

RWByteAddressBuffer GSMemory      : register(u0);
RWTexture2D<unorm float4> CLUT    : register(u2);

#define INTERLEAVE_BIT(addr, src_bit, dst_bit) ((((addr) >> src_bit) & 1) << dst_bit)
#define INTERLEAVE_BIT_X(src_bit, dst_bit) ((((xy.x) >> src_bit) & 1) << dst_bit)
#define INTERLEAVE_BIT_Y(src_bit, dst_bit) ((((xy.y) >> src_bit) & 1) << dst_bit)

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
  return (GSMemory.Load((nibble_addr / 2) & ~3) >> ((nibble_addr & 7) * 16)) & 0xFu;
}

int2 get_xy_csm1_idtex8(uint i)
{
  int x = INTERLEAVE_BIT(i, 4, 3) |
          INTERLEAVE_BIT(i, 2, 2) |
          INTERLEAVE_BIT(i, 1, 1) |
          INTERLEAVE_BIT(i, 0, 0);
  int y = INTERLEAVE_BIT(i, 7, 3) |
          INTERLEAVE_BIT(i, 6, 2) |
          INTERLEAVE_BIT(i, 5, 1) |
          INTERLEAVE_BIT(i, 3, 0);
  return int2(x, y);
}

int2 get_xy_csm1_idtex4(uint i)
{
  return int2(i & 7, (i >> 3) & 1);
}

int2 get_xy_csm2(uint i)
{
  return int2(COU + i, COV);
}

int get_idtex()
{
  switch (PSM)
  {
    case PSMT8:
    case PSMT8H:
      return IDTEX8;
    case PSMT4:
    case PSMT4HL:
    case PSMT4HH:
      return IDTEX4;
    default:
      return INVALID;
  }
}

int2 get_xy(uint i, uint idtex)
{
  switch (CSM)
  {
    case CSM1:
      switch (idtex)
      {
        case IDTEX8:
          return get_xy_csm1_idtex8(i);
        case IDTEX4:
          return get_xy_csm1_idtex4(i);
        default:
          return int2(0, 0);
      }
      break;
    case CSM2:
      return get_xy_csm2(i);
    default:
      return int2(0, 0);
  }
}

int get_max_size(int idtex)
{
  switch (idtex)
  {
    case IDTEX8:
      return 256;
    case IDTEX4:
      return 16;
    default:
      return 0;
  }
}

[numthreads(16, 1, 1)]
void main(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID)
{
  int i = WorkGroupId.x * 16 + LocalThreadId.x;

  int idtex = get_idtex();

  int max_size = get_max_size(idtex);

  if (i >= max_size)
    return;

  int2 xy = get_xy(i, idtex);

  int2 page_size = get_page_size(CPSM);

  int2 page_coord = xy / page_size;

  xy -= page_coord * page_size;

  uint buffer_page_width = (CBW * 64 + page_size.x - 1) / page_size.x;

  uint page = page_coord.y * buffer_page_width + page_coord.x;
  uint base_word_addr = CBP * 64 + page * 2048; // First page

  switch (CPSM)
  {
    case PSMCT32:
      {
        uint word_addr = base_word_addr + word_addr_c32(xy);
        uint word = LoadWord(word_addr);
        float4 color = word_to_float4(word);
        CLUT[int2(i, 0)] = color;
      }
      break;
    case PSMCT16S:
    case PSMCT16:
      {
        uint short_addr = base_word_addr * 2;
        if (PSM == PSMCT16)
          short_addr += short_addr_c16(xy);
        else
          short_addr += short_addr_c16s(xy);
        uint shrt = LoadShort(short_addr);
        float4 color = short_to_float4(shrt);
        CLUT[int2(i, 0)] = color;
      }
      break;
    default:
      CLUT[int2(i, 0)] = float4(1, 0, 0, 1);
      break;
  }
}

// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "IopBios2.h"
#include "deci2.h"

struct DECI2_ILOADP_HEADER{
	DECI2_HEADER	h;			//+00
	u8				code,		//+08	cmd
					action,		//+09
					result,		//+0A
					stamp;		//+0B
	u32				moduleId;	//+0C
};			//=10

struct DECI2_ILOADP_INFO{
	u16		version,		//+00
			flags;			//+02
	u32		module_address,	//+04
			text_size,		//+08
			data_size,		//+0C
			bss_size,		//+10
			_pad[3];		//+14
};

void writeInfo(DECI2_ILOADP_INFO *info,
			   u16 version, u16 flags, u32 module_address,
			   u32 text_size, u32 data_size, u32 bss_size){
	info->version		=version;
	info->flags			=flags;
	info->module_address=module_address;
	info->text_size		=text_size;
	info->data_size		=data_size;
	info->bss_size		=bss_size;
	info->_pad[0]=info->_pad[1]=info->_pad[2]=0;
}

void D2_ILOADP(const u8 *inbuffer, u8 *outbuffer, char *message){
	DECI2_ILOADP_HEADER	*in=(DECI2_ILOADP_HEADER*)inbuffer,
				*out=(DECI2_ILOADP_HEADER*)outbuffer;
	u8	*data=(u8*)in+sizeof(DECI2_ILOADP_HEADER);
	const irxImageInfo	*iii;
	static char line[1024];

	memcpy(outbuffer, inbuffer, 128*1024);//BUFFERSIZE
	out->h.length=sizeof(DECI2_ILOADP_HEADER);
	out->code++;
	out->result=0;	//ok
	exchangeSD((DECI2_HEADER*)out);
	switch(in->code){
		case 0:
			sprintf(line, "code=START action=%d stamp=%d moduleId=0x%X", in->action, in->stamp, in->moduleId);
			break;
		case 2:
			sprintf(line, "code=REMOVE action=%d stamp=%d moduleId=0x%X", in->action, in->stamp, in->moduleId);
			break;
		case 4:
			sprintf(line, "code=LIST action=%d stamp=%d moduleId=0x%X", in->action, in->stamp, in->moduleId);
			data=(u8*)out+sizeof(DECI2_ILOADP_HEADER);
				for (iii=iopVirtMemR<irxImageInfo>(0x800); iii; iii=iii->next?iopVirtMemR<irxImageInfo>(iii->next):0, data+=4)
					*(u32*)data=iii->index;

			out->h.length=data-(u8*)out;
			break;
		case 6:
			sprintf(line, "code=INFO action=%d stamp=%d moduleId=0x%X", in->action, in->stamp, in->moduleId);
			data=(u8*)out+sizeof(DECI2_ILOADP_HEADER);
				for(iii=iopVirtMemR<irxImageInfo>(0x800); iii; iii=iii->next?iopVirtMemR<irxImageInfo>(iii->next):0)
					if (iii->index==in->moduleId){
						writeInfo((DECI2_ILOADP_INFO*)data,
							iii->version, iii->flags, iii->vaddr, iii->text_size, iii->data_size, iii->bss_size);
						data+=sizeof(DECI2_ILOADP_INFO);
						strcpy((char*)data, iopVirtMemR<char>(iii->name));
						data+=strlen(iopVirtMemR<char>(iii->name))+4;
						data=(u8*)((int)data & 0xFFFFFFFC);
						break;

			}
			out->h.length=data-(u8*)out;
			break;
		case 8:
			sprintf(line, "code=WATCH action=%d stamp=%d moduleId=0x%X", in->action, in->stamp, in->moduleId);
			break;
		default:
			sprintf(line, "code=%d[unknown]", in->code);
	}
	sprintf(message, "[ILOADP %c->%c/%04X] %s", in->h.source, in->h.destination, in->h.length, line);
	writeData(outbuffer);
}

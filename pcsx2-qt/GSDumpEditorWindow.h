// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GS.h"
#include "GS/GSRegs.h"
#include "ui_GSDumpEditorWindow.h"
#include "GSRegParseFormat.h"
#include <QtWidgets/QDialog>

#include <QtCore/QFile>
#include <QtCore/QString>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextBrowser>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <QtWidgets/QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>


class GSDumpFileParsed
{
public:
	struct GSReg // REGLIST register
	{
		u32 reg_id;
		u64 data;

		static const u8* parseReg(u32 reg_id, const u8** data)
		{
			GSReg reg;
			reg.reg_id = reg_id;
			reg.data = ((u64*)data)[0];
			data += 8;
			return reg;
		}

		std::string getName() const
		{
			return GSRegGetName(reg_id);
		}
	};
	struct GSPackedReg // PACKED register
	{
		u32 reg_id;
		u64 data[2];

		const u8* parseReg(u32 reg_id, const u8* data)
		{
			GSPackedReg reg;
			reg.reg_id = reg_id;
			reg.data[0] = ((u64*)data)[0];
			reg.data[1] = ((u64*)data)[1];
			return data + 16;
		}
	};

	struct GSDumpItemPrivRegs
	{
		GSRegPMODE PMODE;
		GSRegSMODE1 SMODE1;
		GSRegSMODE2 SMODE2;
		GSRegSRFSH SRFSH;
		GSRegSYNCH1 SYNCH1;
		GSRegSYNCH2 SYNCH2;
		GSRegSYNCV SYNCV;
		struct
		{
			GSRegDISPFB DISPFB;
			GSRegDISPLAY DISPLAY;
		} DISP[2];
		GSRegEXTBUF EXTBUF;
		GSRegEXTDATA EXTDATA;
		GSRegEXTWRITE EXTWRITE;
		GSRegBGCOLOR BGCOLOR;
		GSRegCSR CSR;
		GSRegIMR IMR;
		GSRegBUSDIR BUSDIR;
		GSRegSIGLBLID SIGLBLID;
		
		GSDumpItemPrivRegs(GSPrivRegSet* regs)
		{
			PMODE = regs->PMODE;
			SMODE1 = regs->SMODE1;
			SMODE2 = regs->SMODE2;
			SRFSH = regs->SRFSH;
			SYNCH1 = regs->SYNCH1;
			SYNCH2 = regs->SYNCH2;
			SYNCV = regs->SYNCV;
			DISP[0].DISPFB = regs->DISP[0].DISPFB;
			DISP[0].DISPLAY = regs->DISP[0].DISPLAY;
			DISP[1].DISPFB = regs->DISP[1].DISPFB;
			DISP[1].DISPLAY = regs->DISP[1].DISPLAY;
			EXTBUF = regs->EXTBUF;
			EXTDATA = regs->EXTDATA;
			EXTWRITE = regs->EXTWRITE;
			BGCOLOR = regs->BGCOLOR;
			CSR = regs->CSR;
			IMR = regs->IMR;
			BUSDIR = regs->BUSDIR;
			SIGLBLID = regs->SIGLBLID;
		}
	};
	struct GSDumpItemGIFPacket
	{
		GIFTag giftag;
		std::vector<u8> data;

		GSDumpItemGIFPacket(const u8* packet_data, size_t packet_size)
		{
			if (packet_size < sizeof(GIFTag))
			{
				Console.Error("GSDumpItemGIFPacket: Packet size is smaller than GIFTag size.");
				return;
			}

			std::memcpy(&giftag, packet_data, sizeof(GIFTag));
			data.resize(packet_size - sizeof(GIFTag));
			std::memcpy(data.data(), packet_data + sizeof(GIFTag), data.size());
		}

		void GetReg(size_t index, GSReg& reg) 
	};
	class GSDumpItemVsync
	{
		std::vector<std : unique_ptr<GSDumpItemPacket>>
	};

};

class GSDumpEditorWindow final : public QDialog
{
	Q_OBJECT

public:


	explicit GSDumpEditorWindow(QWidget* parent = nullptr);
	~GSDumpEditorWindow();


	Ui::GSDumpEditorWindow m_ui;
	void PopulateDumpTree(const GSDumpFile& dump_file);
	void ClearDumpTree();
	QVBoxLayout* m_layout;
	QTreeView* m_dump_tree;
	QStandardItemModel* m_dump_model;
	//static void showHTMLDialog(QWidget* parent, const QString& title, const QString& url);
};

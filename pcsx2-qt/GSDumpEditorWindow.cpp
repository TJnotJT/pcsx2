// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "GS.h"
#include "GS/GSLzma.h"
#include "pcsx2/SupportURLs.h"
#include "common/StringUtil.h"

#include "GSDumpEditorWindow.h"
#include "QtHost.h"
#include "QtUtils.h"

#include "common/Error.h"
#include "common/FileSystem.h"
#include "common/Path.h"
#include "common/StringUtil.h"
#include "common/Threading.h"
#include "common/Timer.h"

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

GSDumpEditorWindow::GSDumpEditorWindow(QWidget* parent)
	: QDialog(parent)
{
	m_ui.setupUi(this);
	setWindowTitle("My Minimal Dialog");

	// Arrange widgets using a layout
	m_layout = new QVBoxLayout(this);

	// Create the tree view and set it as the central widget
	m_dump_tree = new QTreeView(this);
	m_layout->addWidget(m_dump_tree);

	// Create the model
	m_dump_model = new QStandardItemModel(m_dump_tree);
	m_dump_model->setHorizontalHeaderLabels(QStringList() << "Example Tree");

	// Populate the model with example data
	QStandardItem* rootItem = m_dump_model->invisibleRootItem();

	QStandardItem* parentItem = new QStandardItem("Parent");
	QStandardItem* childItem1 = new QStandardItem("Child 1");
	QStandardItem* childItem2 = new QStandardItem("Child 2");

	parentItem->appendRow(childItem1);
	parentItem->appendRow(childItem2);

	rootItem->appendRow(parentItem);

	// Set the model on the view
	m_dump_tree->setModel(m_dump_model);

	// Add a button box for clearing the tree
	QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Reset, this);
	connect(buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &GSDumpEditorWindow::accept);
	connect(buttonBox->button(QDialogButtonBox::Reset), &QPushButton::clicked, this, &GSDumpEditorWindow::ClearDumpTree);
	m_layout->addWidget(buttonBox);
	
	const char* filename = "E:\\ps2_gs_dumps\\megaman\\Mega Man X Collection_SLUS-21370_20250605121517.gs.zst";
	Console.WriteLn("(GSDumpReplayer) Reading file '%s'...", filename);

	Error error;
	auto dump_file = GSDumpFile::OpenGSDump(filename, &error);
	if (!dump_file || !dump_file->ReadFile(&error))
	{
		Host::ReportErrorAsync("GSDumpEditorWindow", fmt::format("Failed to open or read '{}': {}",
													 filename, error.GetDescription()));
		dump_file.reset();
	}
	else
	{
		PopulateDumpTree(*dump_file.get());
	}
}

void GSDumpEditorWindow::ClearDumpTree()
{
	m_dump_model->clear();
}

static const char* path_str[] = {
	"Path1Old",
	"Path1New",
	"Path2",
	"Path3",
};


static const char* flg_str[] = {
	"packed",
	"reglist",
	"image",
	"disable",
};


void GSDumpEditorWindow::PopulateDumpTree(const GSDumpFile& dump_file)
{
	FILE* f = fopen("E:\\debug.txt", "w");

	const u8* mem = 0;
	QStandardItem* path_item = 0;
	QStandardItem* giftag_item = 0;
	ClearDumpTree();


	std::string treeName = StringUtil::StdStringFromFormat("GS Dump (%zd packets)", dump_file.GetPackets().size());
	m_dump_tree->setObjectName(treeName.c_str());

	for (const GSDumpFile::GSData& packet : dump_file.GetPackets())
	{
		switch (packet.id)
		{
			case GSDumpTypes::GSType::Transfer:
			{
				switch (packet.path)
				{
					case GSDumpTypes::GSTransferPath::Path1Old:
						
						/*std::unique_ptr<u8[]> data(new u8[16384]);
						const s32 addr = 16384 - packet.length;
						std::memcpy(data.get(), packet.data + addr, packet.length);
						GSDumpReplayerSendPacketToMTGS(GIF_PATH_1, data.get(), packet.length);*/
					case GSDumpTypes::GSTransferPath::Path1New:
					case GSDumpTypes::GSTransferPath::Path2:
					case GSDumpTypes::GSTransferPath::Path3:
						// GSDumpReplayerSendPacketToMTGS(static_cast<GIF_PATH>(static_cast<u8>(packet.path) - 1),
						// 	packet.data, packet.length);
						path_item = new QStandardItem(path_str[(int)packet.path]);
						m_dump_model->appendRow(path_item);
						
						fprintf(f, "packet length: %zd\n", packet.length);
						for (mem = packet.data; mem < packet.data + packet.length;)
						{
							GIFTag* giftag = (GIFTag*)mem;
							mem += sizeof(GIFTag);
							
							std::string giftag_str = StringUtil::StdStringFromFormat(
								"GifTag NLOOP=%d EOP=%d PRE=%d PRIM=%d FLG=%s",
								giftag->NLOOP, giftag->EOP, giftag->PRE, giftag->PRIM, flg_str[giftag->FLG]);

							giftag_item = new QStandardItem(giftag_str.c_str());
							path_item->appendRow(giftag_item);

							std::string image_str;
							switch (giftag->FLG)
							{
								case 0: // packed
									for (size_t i = 0; i < giftag->NLOOP; i++)
									{
										for (size_t j = 0; j < giftag->NREG; j++)
										{
											switch ((giftag->REGS >> (4 * j)) & 0xf)
											{
												case 0x0:
													giftag_item->appendRow(new QStandardItem("PRIM"));
													break;
												case 0x1:
													giftag_item->appendRow(new QStandardItem("RGBAQ"));
													break;
												case 0x2:
													giftag_item->appendRow(new QStandardItem("ST"));
													break;
												case 0x3:
													giftag_item->appendRow(new QStandardItem("UV"));
													break;
												case 0x4:
													giftag_item->appendRow(new QStandardItem("XYZF2"));
													break;
												case 0x5:
													giftag_item->appendRow(new QStandardItem("XYZ2"));
													break;
												case 0x6:
													giftag_item->appendRow(new QStandardItem("TEX0_1"));
													break;
												case 0x7:
													giftag_item->appendRow(new QStandardItem("TEX0_2"));
													break;
												case 0x8:
													giftag_item->appendRow(new QStandardItem("CLAMP_1"));
													break;
												case 0x9:
													giftag_item->appendRow(new QStandardItem("CLAMP_2"));
													break;
												case 0xa:
													giftag_item->appendRow(new QStandardItem("FOG"));
													break;
												case 0xb:
													Console.WarningFmt("Reserved value in packed mode GIFTag");
													giftag_item->appendRow(new QStandardItem("Reserved"));
													break;
												case 0xc:
													giftag_item->appendRow(new QStandardItem("XYZF3"));
													break;
												case 0xd:
													giftag_item->appendRow(new QStandardItem("XYZ3"));
													break;
												case 0xe:
													giftag_item->appendRow(new QStandardItem("A+D"));
													break;
												case 0xf:
													giftag_item->appendRow(new QStandardItem("FOG"));
													break;
											}
											mem += 16;
										}
									}
									break;
								case 1: // reglist
									for (size_t i = 0; i < giftag->NLOOP; i++)
									{
										for (size_t j = 0; j < giftag->NREG; j++)
										{
											switch ((giftag->REGS >> (4 * j)) & 0xf)
											{
												case 0x0:
													giftag_item->appendRow(new QStandardItem("PRIM"));
													break;
												case 0x1:
													giftag_item->appendRow(new QStandardItem("RGBAQ"));
													break;
												case 0x2:
													giftag_item->appendRow(new QStandardItem("ST"));
													break;
												case 0x3:
													giftag_item->appendRow(new QStandardItem("UV"));
													break;
												case 0x4:
													giftag_item->appendRow(new QStandardItem("XYZF2"));
													break;
												case 0x5:
													giftag_item->appendRow(new QStandardItem("XYZ2"));
													break;
												case 0x6:
													giftag_item->appendRow(new QStandardItem("TEX0_1"));
													break;
												case 0x7:
													giftag_item->appendRow(new QStandardItem("TEX0_2"));
													break;
												case 0x8:
													giftag_item->appendRow(new QStandardItem("CLAMP_1"));
													break;
												case 0x9:
													giftag_item->appendRow(new QStandardItem("CLAMP_2"));
													break;
												case 0xa:
													giftag_item->appendRow(new QStandardItem("FOG"));
													break;
												case 0xb:
													Console.WarningFmt("Reserved value in packed mode GIFTag");
													giftag_item->appendRow(new QStandardItem("Reserved"));
													break;
												case 0xc:
													giftag_item->appendRow(new QStandardItem("XYZF3"));
													break;
												case 0xd:
													giftag_item->appendRow(new QStandardItem("XYZ3"));
													break;
												case 0xe:
													giftag_item->appendRow(new QStandardItem("A+D"));
													break;
												case 0xf:
													giftag_item->appendRow(new QStandardItem("FOG"));
													break;
											}
											mem += 8;
										}
									}
									break;
								case 2: // image
									mem += giftag->NLOOP * 16;
									image_str = StringUtil::StdStringFromFormat("IMAGE (%d bytes)", giftag->NLOOP * 16);
									giftag_item->appendRow(new QStandardItem(image_str.c_str()));
									break;
								case 3:
									Console.WarningFmt("GIF_FLG_DISABLE encountered in packet");
									break;
								default:
									Console.WarningFmt("bad FLG value encountered in packet");
									break;
							}
						}
						assert(mem == packet.data + packet.length);
					default:
						break;
				}
				break;
			}

			case GSDumpTypes::GSType::VSync:
			{
				m_dump_model->appendRow(new QStandardItem("VSync"));
				// s_dump_frame_number++;
				// GSDumpReplayerUpdateFrameLimit();
				// GSDumpReplayerFrameLimit();
				// MTGS::PostVsyncStart(false);
				// VMManager::Internal::VSyncOnCPUThread();
				// if (VMManager::Internal::IsExecutionInterrupted())
				// 	GSDumpReplayerExitExecution();
				// Host::PumpMessagesOnCPUThread();
			}
			break;

			case GSDumpTypes::GSType::ReadFIFO2:
			{
				m_dump_model->appendRow(new QStandardItem("ReadFIFO2"));
				// u32 size;
				// std::memcpy(&size, packet.data, sizeof(size));

				// // Allocate an extra quadword, some transfers write too much (e.g. Lego Racers 2 with Z24 downloads).
				// std::unique_ptr<u8[]> arr(new u8[(size + 1) * 16]);
				// MTGS::InitAndReadFIFO(arr.get(), size);
			}
			break;

			case GSDumpTypes::GSType::Registers:
			{
				m_dump_model->appendRow(new QStandardItem("Registers"));
				// std::memcpy(PS2MEM_GS, packet.data, std::min<s32>(packet.length, Ps2MemSize::GSregs));
			}
			break;
		}
	}
	fclose(f);
}

GSDumpEditorWindow::~GSDumpEditorWindow() = default;
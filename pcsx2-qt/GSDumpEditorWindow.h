// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#pragma once

#include "GS.h"
#include "GS/GSRegs.h"
#include "ui_GSDumpEditorWindow.h"
#include "GSRegParseFormat.h"
#include "GSDumpFileParsed.h"

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

#include "QLazyItemModel.h"


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
	//QStandardItemModel* m_dump_model;
	QStandardItemModel* m_dump_model;
	std::unique_ptr<GSDumpFileParsed> m_dump_file;
};

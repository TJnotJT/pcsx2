#pragma once

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

class QLazyItemModel : QStandardItemModel
{
private:
	bool hasChildren();
};
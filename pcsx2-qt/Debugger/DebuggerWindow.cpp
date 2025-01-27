// SPDX-FileCopyrightText: 2002-2025 PCSX2 Dev Team
// SPDX-License-Identifier: GPL-3.0+

#include "DebuggerWindow.h"

#include "DebugTools/DebugInterface.h"
#include "DebugTools/Breakpoints.h"
#include "DebugTools/SymbolImporter.h"
#include "VMManager.h"
#include "QtHost.h"
#include "MainWindow.h"
#include "AnalysisOptionsDialog.h"

DebuggerWindow::DebuggerWindow(QWidget* parent)
	: QMainWindow(parent)
{
	m_ui.setupUi(this);

// Easiest way to handle cross platform monospace fonts
// There are issues related to TabWidget -> Children font inheritance otherwise
#if defined(WIN32)
	m_ui.cpuTabs->setStyleSheet(QStringLiteral("font: 8pt 'Lucida Console'"));
#elif defined(__APPLE__)
	m_ui.cpuTabs->setStyleSheet(QStringLiteral("font: 10pt 'Monaco'"));
#else
	m_ui.cpuTabs->setStyleSheet(QStringLiteral("font: 8pt 'Monospace'"));
#endif

	connect(m_ui.actionRun, &QAction::triggered, this, &DebuggerWindow::onRunPause);
	connect(m_ui.actionStepInto, &QAction::triggered, this, &DebuggerWindow::onStepInto);
	connect(m_ui.actionStepOver, &QAction::triggered, this, &DebuggerWindow::onStepOver);
	connect(m_ui.actionStepOut, &QAction::triggered, this, &DebuggerWindow::onStepOut);
	connect(m_ui.actionAnalyse, &QAction::triggered, this, &DebuggerWindow::onAnalyse);
	connect(m_ui.actionOnTop, &QAction::triggered, [this] { this->setWindowFlags(this->windowFlags() ^ Qt::WindowStaysOnTopHint); this->show(); });

	connect(g_emu_thread, &EmuThread::onVMPaused, this, &DebuggerWindow::onVMStateChanged);
	connect(g_emu_thread, &EmuThread::onVMResumed, this, &DebuggerWindow::onVMStateChanged);

	onVMStateChanged(); // If we missed a state change while we weren't loaded

	// We can't do this in the designer, but we want to right align the actionOnTop action in the toolbar
	QWidget* spacer = new QWidget(this);
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_ui.toolBar->insertWidget(m_ui.actionAnalyse, spacer);

	m_cpuWidget_r5900 = new CpuWidget(this, r5900Debug);
	m_cpuWidget_r3000 = new CpuWidget(this, r3000Debug);

	m_ui.cpuTabs->addTab(m_cpuWidget_r5900, "R5900");
	m_ui.cpuTabs->addTab(m_cpuWidget_r3000, "R3000");
}

DebuggerWindow::~DebuggerWindow() = default;

// There is no straightforward way to set the tab text to bold in Qt
// Sorry colour blind people, but this is the best we can do for now
void DebuggerWindow::setTabActiveStyle(BreakPointCpu enabledCpu)
{
	m_ui.cpuTabs->tabBar()->setTabTextColor(m_ui.cpuTabs->indexOf(m_cpuWidget_r5900), (enabledCpu == BREAKPOINT_EE) ? Qt::red : this->palette().text().color());
	m_ui.cpuTabs->tabBar()->setTabTextColor(m_ui.cpuTabs->indexOf(m_cpuWidget_r3000), (enabledCpu == BREAKPOINT_IOP) ? Qt::red : this->palette().text().color());
}

void DebuggerWindow::onVMStateChanged()
{
	if (!QtHost::IsVMPaused())
	{
		m_ui.actionRun->setText(tr("Pause"));
		m_ui.actionRun->setIcon(QIcon::fromTheme(QStringLiteral("pause-line")));
		m_ui.actionStepInto->setEnabled(false);
		m_ui.actionStepOver->setEnabled(false);
		m_ui.actionStepOut->setEnabled(false);
		setTabActiveStyle(BREAKPOINT_IOP_AND_EE);
	}
	else
	{
		m_ui.actionRun->setText(tr("Run"));
		m_ui.actionRun->setIcon(QIcon::fromTheme(QStringLiteral("play-line")));
		m_ui.actionStepInto->setEnabled(true);
		m_ui.actionStepOver->setEnabled(true);
		m_ui.actionStepOut->setEnabled(true);
		// Switch to the CPU tab that triggered the breakpoint
		// Also bold the tab text to indicate that a breakpoint was triggered
		if (CBreakPoints::GetBreakpointTriggered())
		{
			const BreakPointCpu triggeredCpu = CBreakPoints::GetBreakpointTriggeredCpu();
			setTabActiveStyle(triggeredCpu);
			switch (triggeredCpu)
			{
				case BREAKPOINT_EE:
					m_ui.cpuTabs->setCurrentWidget(m_cpuWidget_r5900);
					break;
				case BREAKPOINT_IOP:
					m_ui.cpuTabs->setCurrentWidget(m_cpuWidget_r3000);
					break;
				default:
					break;
			}
			Host::RunOnCPUThread([] {
				CBreakPoints::ClearTemporaryBreakPoints();
				CBreakPoints::SetBreakpointTriggered(false, BREAKPOINT_IOP_AND_EE);
				// Our current PC is on a breakpoint.
				// When we run the core again, we want to skip this breakpoint and run
				CBreakPoints::SetSkipFirst(BREAKPOINT_EE, r5900Debug.getPC());
				CBreakPoints::SetSkipFirst(BREAKPOINT_IOP, r3000Debug.getPC());
			});
		}
	}
	return;
}

void DebuggerWindow::onRunPause()
{
	g_emu_thread->setVMPaused(!QtHost::IsVMPaused());
}

void DebuggerWindow::onStepInto()
{
	CpuWidget* currentCpu = static_cast<CpuWidget*>(m_ui.cpuTabs->currentWidget());
	currentCpu->onStepInto();
}

void DebuggerWindow::onStepOver()
{
	CpuWidget* currentCpu = static_cast<CpuWidget*>(m_ui.cpuTabs->currentWidget());
	currentCpu->onStepOver();
}

void DebuggerWindow::onStepOut()
{
	CpuWidget* currentCpu = static_cast<CpuWidget*>(m_ui.cpuTabs->currentWidget());
	currentCpu->onStepOut();
}

void DebuggerWindow::onAnalyse()
{
	AnalysisOptionsDialog* dialog = new AnalysisOptionsDialog(this);
	dialog->show();
}

void DebuggerWindow::showEvent(QShowEvent* event)
{
	Host::RunOnCPUThread([]() {
		R5900SymbolImporter.OnDebuggerOpened();
	});
	QMainWindow::showEvent(event);
}

void DebuggerWindow::hideEvent(QHideEvent* event)
{
	Host::RunOnCPUThread([]() {
		R5900SymbolImporter.OnDebuggerClosed();
	});
	QMainWindow::hideEvent(event);
}

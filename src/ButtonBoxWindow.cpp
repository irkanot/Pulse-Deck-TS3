#include "ButtonBoxWindow.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QSizePolicy>
#include <QVariant>
#include <algorithm>

#include "SoundButton.h"

ButtonBoxWindow::ButtonBoxWindow(QWidget* parent) :
	QWidget(parent),
	m_rowsMinus(new QPushButton("-", this)),
	m_rowsPlus(new QPushButton("+", this)),
	m_colsMinus(new QPushButton("-", this)),
	m_colsPlus(new QPushButton("+", this)),
	m_rowsValue(new QLabel(this)),
	m_colsValue(new QLabel(this)),
	m_grid(new QGridLayout()),
	m_gridHost(new QWidget(this)),
	m_rows(4),
	m_cols(5),
	m_minRows(1),
	m_minCols(1),
	m_maxRows(12),
	m_maxCols(12)
{
	setWindowTitle("ButtonBox");
	setAttribute(Qt::WA_DeleteOnClose, false);

	QVBoxLayout* root = new QVBoxLayout(this);
	root->setContentsMargins(8, 8, 8, 8);
	root->setSpacing(8);

	QHBoxLayout* header = new QHBoxLayout();
	header->setSpacing(6);
	header->addWidget(new QLabel("Rows", this));
	header->addWidget(m_rowsMinus);
	header->addWidget(m_rowsValue);
	header->addWidget(m_rowsPlus);
	header->addSpacing(12);
	header->addWidget(new QLabel("Cols", this));
	header->addWidget(m_colsMinus);
	header->addWidget(m_colsValue);
	header->addWidget(m_colsPlus);
	header->addStretch();
	root->addLayout(header);

	m_rowsValue->setFixedWidth(30);
	m_colsValue->setFixedWidth(30);
	m_rowsValue->setAlignment(Qt::AlignCenter);
	m_colsValue->setAlignment(Qt::AlignCenter);

	m_grid->setContentsMargins(0, 0, 0, 0);
	m_grid->setSpacing(6);
	m_gridHost->setLayout(m_grid);
	root->addWidget(m_gridHost, 1);

	connect(m_rowsMinus, SIGNAL(clicked()), this, SLOT(onRowsMinus()));
	connect(m_rowsPlus, SIGNAL(clicked()), this, SLOT(onRowsPlus()));
	connect(m_colsMinus, SIGNAL(clicked()), this, SLOT(onColsMinus()));
	connect(m_colsPlus, SIGNAL(clicked()), this, SLOT(onColsPlus()));

	rebuildGrid();
}

ButtonBoxWindow::~ButtonBoxWindow() = default;

void ButtonBoxWindow::setDimensions(int rows, int cols)
{
	rows = std::max(m_minRows, std::min(m_maxRows, rows));
	cols = std::max(m_minCols, std::min(m_maxCols, cols));
	if (rows == m_rows && cols == m_cols)
		return;
	m_rows = rows;
	m_cols = cols;
	rebuildGrid();
}

void ButtonBoxWindow::setButtonText(int index, const QString& text)
{
	if (index < 0 || index >= (int)m_buttons.size())
		return;
	m_buttons[index]->setText(text);
}

void ButtonBoxWindow::setButtonColor(int index, const QColor& color)
{
	if (index < 0 || index >= (int)m_buttons.size())
		return;
	m_buttons[index]->setBackgroundColor(color);
}

void ButtonBoxWindow::setButtonCountTexts()
{
	for (int i = 0; i < (int)m_buttons.size(); ++i)
	{
		if (m_buttons[i]->text().isEmpty())
			m_buttons[i]->setText(QString("B%1").arg(i + 1));
	}
}

void ButtonBoxWindow::onRowsMinus()
{
	if (m_rows <= m_minRows)
		return;
	m_rows--;
	rebuildGrid();
	emit rowsChanged(m_rows);
}

void ButtonBoxWindow::onRowsPlus()
{
	if (m_rows >= m_maxRows)
		return;
	m_rows++;
	rebuildGrid();
	emit rowsChanged(m_rows);
}

void ButtonBoxWindow::onColsMinus()
{
	if (m_cols <= m_minCols)
		return;
	m_cols--;
	rebuildGrid();
	emit colsChanged(m_cols);
}

void ButtonBoxWindow::onColsPlus()
{
	if (m_cols >= m_maxCols)
		return;
	m_cols++;
	rebuildGrid();
	emit colsChanged(m_cols);
}

void ButtonBoxWindow::onButtonClicked()
{
	SoundButton* btn = qobject_cast<SoundButton*>(sender());
	if (!btn)
		return;
	emit buttonPlayRequested(btn->property("buttonId").toInt());
}

void ButtonBoxWindow::onButtonContextMenu(const QPoint& point)
{
	SoundButton* btn = qobject_cast<SoundButton*>(sender());
	if (!btn)
		return;
	emit buttonContextRequested(btn->property("buttonId").toInt(), btn->mapToGlobal(point));
}

void ButtonBoxWindow::onButtonFileDropped(const QList<QUrl>& urls)
{
	SoundButton* btn = qobject_cast<SoundButton*>(sender());
	if (!btn)
		return;
	emit buttonFileDropped(btn->property("buttonId").toInt(), urls);
}

void ButtonBoxWindow::onButtonDroppedOnButton(SoundButton* button)
{
	SoundButton* src = qobject_cast<SoundButton*>(sender());
	if (!src || !button)
		return;
	emit buttonSwapRequested(src->property("buttonId").toInt(), button->property("buttonId").toInt());
}

void ButtonBoxWindow::rebuildGrid()
{
	for (SoundButton* b : m_buttons)
		delete b;
	m_buttons.clear();

	while (QLayoutItem* item = m_grid->takeAt(0))
	{
		delete item;
	}

	for (int r = 0; r < m_rows; ++r)
	{
		for (int c = 0; c < m_cols; ++c)
		{
			SoundButton* elem = new SoundButton(this);
			int id = (int)m_buttons.size();
			elem->setProperty("buttonId", id);
			elem->setText("(no file)");
			QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Expanding);
			elem->setSizePolicy(policy);
			elem->setMinimumSize(120, 72);

			connect(elem, SIGNAL(clicked()), this, SLOT(onButtonClicked()));
			elem->setContextMenuPolicy(Qt::CustomContextMenu);
			connect(elem, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(onButtonContextMenu(const QPoint&)));
			connect(elem, SIGNAL(fileDropped(QList<QUrl>)), this, SLOT(onButtonFileDropped(QList<QUrl>)));
			connect(elem, SIGNAL(buttonDropped(SoundButton*)), this, SLOT(onButtonDroppedOnButton(SoundButton*)));

			m_grid->addWidget(elem, r, c);
			m_buttons.push_back(elem);
		}
	}

	updateHeaderLabels();
	applyAutoResize();
}

void ButtonBoxWindow::updateHeaderLabels()
{
	m_rowsValue->setText(QString::number(m_rows));
	m_colsValue->setText(QString::number(m_cols));
}

void ButtonBoxWindow::applyAutoResize()
{
	const int minButtonW = 120;
	const int minButtonH = 72;
	const int spacing = m_grid->spacing();
	const int margins = 16;
	const int headerH = 46;

	int needW = margins + (m_cols * minButtonW) + ((m_cols - 1) * spacing) + margins;
	int needH = margins + headerH + (m_rows * minButtonH) + ((m_rows - 1) * spacing) + margins;

	setMinimumSize(needW, needH);
	resize(std::max(width(), needW), std::max(height(), needH));
}

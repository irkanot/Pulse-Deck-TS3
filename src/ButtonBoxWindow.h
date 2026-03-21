#pragma once

#include <QWidget>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>
#include <QColor>
#include <vector>

class SoundButton;

class ButtonBoxWindow : public QWidget
{
	Q_OBJECT

  public:
	explicit ButtonBoxWindow(QWidget* parent = nullptr);
	~ButtonBoxWindow() override;

	void setDimensions(int rows, int cols);
	void setButtonText(int index, const QString& text);
	void setButtonColor(int index, const QColor& color);
	void setButtonCountTexts();
	int rows() const { return m_rows; }
	int cols() const { return m_cols; }

  signals:
	void rowsChanged(int rows);
	void colsChanged(int cols);
	void buttonPlayRequested(int buttonId);
	void buttonContextRequested(int buttonId, const QPoint& globalPos);
	void buttonFileDropped(int buttonId, const QList<QUrl>& urls);
	void buttonSwapRequested(int sourceButtonId, int targetButtonId);

  private slots:
	void onRowsMinus();
	void onRowsPlus();
	void onColsMinus();
	void onColsPlus();
	void onButtonClicked();
	void onButtonContextMenu(const QPoint& point);
	void onButtonFileDropped(const QList<QUrl>& urls);
	void onButtonDroppedOnButton(SoundButton* button);

  private:
	void rebuildGrid();
	void updateHeaderLabels();
	void applyAutoResize();

	QPushButton* m_rowsMinus;
	QPushButton* m_rowsPlus;
	QPushButton* m_colsMinus;
	QPushButton* m_colsPlus;
	QLabel* m_rowsValue;
	QLabel* m_colsValue;
	QGridLayout* m_grid;
	QWidget* m_gridHost;

	std::vector<SoundButton*> m_buttons;
	int m_rows;
	int m_cols;
	int m_minRows;
	int m_minCols;
	int m_maxRows;
	int m_maxCols;
};

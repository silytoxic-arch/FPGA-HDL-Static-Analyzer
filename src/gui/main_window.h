#pragma once

#include "analysis/analyzer.h"

#include <QFutureWatcher>
#include <QMainWindow>
#include <QString>
#include <QStringList>

#include <optional>
#include <array>

class QAction;
class QPlainTextEdit;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QStackedWidget;
class QComboBox;

namespace fpga::gui {

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void openProjectPath(const QString& path);
    void openSourceFiles(const QStringList& paths);

private:
    void openProject();
    void runAnalysis();
    void analysisFinished();
    void populateResult();
    void showViolation(int row);
    void openTreeLocation(QTreeWidgetItem* item);
    void loadSourceLocation(const QString& path, int line);
    void exportReport(const QString& format);
    void setAnalysisRunning(bool running);

    QString projectPath_;
    QStringList sourcePaths_;
    QTreeWidget* projectTree_{nullptr};
    QPlainTextEdit* sourceEditor_{nullptr};
    QTableWidget* problemsTable_{nullptr};
    QTableWidget* statisticsTable_{nullptr};
    QPlainTextEdit* violationDetails_{nullptr};
    QStackedWidget* pages_{nullptr};
    QComboBox* standardSelector_{nullptr};
    QLabel* projectName_{nullptr};
    QLabel* sourceTitle_{nullptr};
    std::array<QLabel*, 8> metricValues_{};
    std::array<QLabel*, 8> metricNotes_{};
    QAction* openProjectAction_{nullptr};
    QAction* analyzeAction_{nullptr};
    QAction* exportHtmlAction_{nullptr};
    QAction* exportSarifAction_{nullptr};
    QAction* exportStatisticsAction_{nullptr};
    QFutureWatcher<analysis::AnalysisResult> analysisWatcher_;
    std::optional<analysis::AnalysisResult> lastResult_;
};

} // namespace fpga::gui

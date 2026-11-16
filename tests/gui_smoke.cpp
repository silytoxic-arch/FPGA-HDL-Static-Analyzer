#include "gui/main_window.h"
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QTimer>
#include <QToolBar>
#include <QTreeWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QStackedWidget>
#include <QPushButton>
#include <QElapsedTimer>
#include <QThread>
#include <QDir>
#include <QLabel>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QTemporaryDir>
#include <QFile>
#include <QIcon>
#include <QHeaderView>
#include <QMouseEvent>
#include <QScrollBar>
#include <array>
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    fpga::gui::MainWindow window;
    window.show();
    int failures = 0;
    QTimer::singleShot(100, &window, [&] {
        auto checkAtLine = [&](bool ok, int line) {
            if (!ok) { ++failures; std::cerr << "GUI check failed at line " << line << '\n'; }
        };
#define check(...) checkAtLine((__VA_ARGS__), __LINE__)
        auto dragColumnBoundary = [&](QTableWidget* table, int column, int delta, int inset = 6) {
            auto* header = table->horizontalHeader();
            auto* surface = header->viewport();
            const int widthBeforeDrag = header->sectionSize(column);
            const int x = header->sectionViewportPosition(column) + header->sectionSize(column) - inset;
            const int y = header->height() / 2;
            const QPoint start(x, y);
            const QPoint end(x + delta, y);
            QMouseEvent press(QEvent::MouseButtonPress, QPointF(start),
                              QPointF(surface->mapToGlobal(start)),
                              Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(surface, &press);
            QMouseEvent move(QEvent::MouseMove, QPointF(end),
                             QPointF(surface->mapToGlobal(end)),
                             Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
            QCoreApplication::sendEvent(surface, &move);
            app.processEvents();
            auto* guide = surface->findChild<QFrame*>("columnResizeGuide");
            check(guide && guide->isVisible());
            check(guide && guide->geometry().center().x()
                              == header->sectionViewportPosition(column) + widthBeforeDrag + delta);
            check(header->sectionSize(column) == widthBeforeDrag);
            QMouseEvent release(QEvent::MouseButtonRelease, QPointF(end),
                                QPointF(surface->mapToGlobal(end)),
                                Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QCoreApplication::sendEvent(surface, &release);
            app.processEvents();
            check(guide && !guide->isVisible());
        };
        check(window.isVisible());
        check(window.windowTitle() == QStringLiteral("FPGA HDL Static Analyzer"));
        const QIcon brandingIcon(QStringLiteral(":/branding/app-icon.png"));
        check(!brandingIcon.isNull());
        check(!brandingIcon.pixmap(32, 32).isNull());
        check(window.menuBar()->actions().size() == 4);
        check(window.findChildren<QDockWidget*>().isEmpty());
        auto* statistics = window.findChild<QTableWidget*>("statisticsTable");
        check(statistics && statistics->columnCount() == 6 && statistics->rowCount() == 0);
        std::array<int, 6> defaultStatisticsWidths{};
        std::array<int, 6> resizedStatisticsWidths{};
        if (statistics) {
            auto* header = statistics->horizontalHeader();
            check(!header->stretchLastSection());
            check(statistics->horizontalScrollBarPolicy() == Qt::ScrollBarAsNeeded);
            for (int column = 0; column < statistics->columnCount(); ++column) {
                check(header->sectionResizeMode(column) == QHeaderView::Interactive);
                defaultStatisticsWidths[column] = statistics->columnWidth(column);
                resizedStatisticsWidths[column] = statistics->columnWidth(column) + 45;
                header->resizeSection(column, resizedStatisticsWidths[column]);
                check(statistics->columnWidth(column) == resizedStatisticsWidths[column]);
            }
        }
        auto* project = window.findChild<QTreeWidget*>("projectTree");
        check(project && project->columnCount() == 3);
        check(project && project->headerItem()->text(0) == QStringLiteral("设计单元 / 实例"));
        auto* editor = window.findChild<QPlainTextEdit*>("sourceEditor");
        check(editor && editor->isReadOnly() && window.centralWidget()->objectName() == "workbench");
        auto* problems = window.findChild<QTableWidget*>("problemsTable");
        check(problems && problems->rowCount() == 0 && problems->columnCount() == 6);
        check(problems && problems->horizontalHeaderItem(0)->text() == QStringLiteral("级别"));
        if (problems) {
            check(problems->frameShape() == QFrame::Box && problems->showGrid());
            check(problems->horizontalScrollMode() == QAbstractItemView::ScrollPerPixel);
            auto* frame = window.findChild<QFrame*>("tablePanel");
            check(frame && frame->frameShape() == QFrame::Box && frame->lineWidth() == 2);
            check(problems->layoutDirection() == Qt::LeftToRight);
            check(problems->horizontalScrollBarPolicy() == Qt::ScrollBarAsNeeded);
            for (int column = 0; column < problems->columnCount(); ++column)
                check(problems->horizontalHeader()->sectionResizeMode(column) == QHeaderView::Interactive);
            const int before = problems->columnWidth(1);
            const int neighborBefore = problems->columnWidth(2);
            const int nextBefore = problems->horizontalHeader()->sectionViewportPosition(2);
            dragColumnBoundary(problems, 1, -32);
            check(problems->columnWidth(1) == before - 32);
            check(problems->columnWidth(2) == neighborBefore + 32);
            check(problems->horizontalHeader()->sectionViewportPosition(2) == nextBefore - 32);
            dragColumnBoundary(problems, 1, 32);
            check(problems->columnWidth(1) == before);
            check(problems->columnWidth(2) == neighborBefore);
            check(problems->horizontalHeader()->sectionViewportPosition(2) == nextBefore);
            const int firstBefore = problems->columnWidth(0);
            const int firstNeighborBefore = problems->columnWidth(1);
            dragColumnBoundary(problems, 0, 28);
            check(problems->columnWidth(0) == firstBefore + 28);
            check(problems->columnWidth(1) == firstNeighborBefore - 28);
            check(problems->horizontalHeader()->sectionViewportPosition(1) == firstBefore + 28);
            dragColumnBoundary(problems, 0, -28);
            check(problems->columnWidth(0) == firstBefore);
            check(problems->columnWidth(1) == firstNeighborBefore);
        }
        check(window.findChild<QPlainTextEdit*>("violationDetails") != nullptr);
        auto* pages = window.findChild<QStackedWidget*>("workspacePages");
        auto* statisticsNavigation = window.findChild<QPushButton*>("statisticsNavigation");
        check(pages && pages->count() == 2 && pages->currentIndex() == 0);
        check(statisticsNavigation != nullptr);
        if (statisticsNavigation) statisticsNavigation->click();
        check(pages && pages->currentIndex() == 1);
        app.processEvents();
        if (statistics) {
            check(statistics->frameShape() == QFrame::Box && statistics->showGrid());
            check(statistics->horizontalScrollMode() == QAbstractItemView::ScrollPerPixel);
            auto* frame = window.findChild<QFrame*>("statisticsPanel");
            check(frame && frame->frameShape() == QFrame::Box && frame->lineWidth() == 2);
            check(window.findChild<QWidget*>("statisticsTitleBar") != nullptr);
            check(statistics->layoutDirection() == Qt::LeftToRight);
            statistics->horizontalScrollBar()->setValue(0);
            const int before = statistics->columnWidth(0);
            const int neighborBefore = statistics->columnWidth(1);
            const int nextBefore = statistics->horizontalHeader()->sectionViewportPosition(1);
            dragColumnBoundary(statistics, 0, -32);
            check(statistics->columnWidth(0) == before - 32);
            check(statistics->columnWidth(1) == neighborBefore + 32);
            check(statistics->horizontalHeader()->sectionViewportPosition(1) == nextBefore - 32);
            dragColumnBoundary(statistics, 0, 32);
            check(statistics->columnWidth(0) == before);
            check(statistics->columnWidth(1) == neighborBefore);
            check(statistics->horizontalHeader()->sectionViewportPosition(1) == nextBefore);
            resizedStatisticsWidths[0] = statistics->columnWidth(0);
            statistics->horizontalScrollBar()->setValue(40);
            check(statistics->horizontalHeader()->sectionViewportPosition(0) < 0);
            const int scrolledBefore = statistics->columnWidth(1);
            const int scrolledNeighborBefore = statistics->columnWidth(2);
            const int scrolledOffset = statistics->horizontalScrollBar()->value();
            const int scrolledEdge = statistics->horizontalHeader()->sectionViewportPosition(1)
                                     + statistics->columnWidth(1);
            dragColumnBoundary(statistics, 1, -24);
            check(statistics->columnWidth(1) == scrolledBefore - 24);
            check(statistics->columnWidth(2) == scrolledNeighborBefore + 24);
            check(statistics->horizontalScrollBar()->value() == scrolledOffset);
            check(statistics->horizontalHeader()->sectionViewportPosition(1)
                  + statistics->columnWidth(1) == scrolledEdge - 24);
            dragColumnBoundary(statistics, 1, 24);
            check(statistics->columnWidth(1) == scrolledBefore);
            check(statistics->columnWidth(2) == scrolledNeighborBefore);
            statistics->horizontalScrollBar()->setValue(statistics->horizontalScrollBar()->maximum());
            const int farRightBefore = statistics->columnWidth(4);
            const int farRightNeighborBefore = statistics->columnWidth(5);
            const int farRightEdge = statistics->horizontalHeader()->sectionViewportPosition(4)
                                   + statistics->columnWidth(4);
            dragColumnBoundary(statistics, 4, -24);
            check(statistics->columnWidth(4) == farRightBefore - 24);
            check(statistics->columnWidth(5) == farRightNeighborBefore + 24);
            check(statistics->horizontalScrollBar()->value() == statistics->horizontalScrollBar()->maximum());
            check(statistics->horizontalHeader()->sectionViewportPosition(4)
                  + statistics->columnWidth(4) == farRightEdge - 24);
            dragColumnBoundary(statistics, 4, 24);
            check(statistics->columnWidth(4) == farRightBefore);
            check(statistics->columnWidth(5) == farRightNeighborBefore);
            statistics->horizontalScrollBar()->setValue(0);
        }
        check(statistics && statistics->horizontalScrollBar()->maximum() > 0);
        check(window.findChild<QAction*>("openProjectAction") != nullptr);
        auto* analyze = window.findChild<QAction*>("runAnalysisAction");
        check(analyze && !analyze->isEnabled());
        check(window.statusBar()->currentMessage().contains(QStringLiteral("就绪")));
        auto* standard = window.findChild<QComboBox*>("standardSelector");
        check(standard && standard->count() == 2);
        check(standard && standard->itemData(0).toString() == "GJB10157-2021");
        check(standard && standard->itemData(1).toString() == "GJB9765-2020");
        // Exercise both visible buttons, not just the direct path-loading helper.
        for (int page = 0; page < 2; ++page) {
            pages->setCurrentIndex(page);
            QPushButton* openButton = nullptr;
            for (auto* button : pages->currentWidget()->findChildren<QPushButton*>()) {
                if (button->text() == QStringLiteral("打开 RTL 源码")) openButton = button;
            }
            check(openButton && openButton->isEnabled());
            if (openButton) openButton->click();
            app.processEvents();
            auto* dialog = window.findChild<QFileDialog*>("rtlSourceDialog");
            check(dialog && dialog->isVisible());
            check(dialog && dialog->testOption(QFileDialog::DontUseNativeDialog));
            check(dialog && dialog->fileMode() == QFileDialog::ExistingFiles);
            if (dialog) dialog->reject();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            check(!analyze->isEnabled());
        }
        pages->setCurrentIndex(1);
        const auto fixturePath = qEnvironmentVariable("FPGA_GUI_TEST_PROJECT");
        if (!fixturePath.isEmpty()) {
            window.findChild<QAction*>("openProjectAction")->trigger();
            app.processEvents();
            auto* dialog = window.findChild<QFileDialog*>("rtlSourceDialog");
            check(dialog && dialog->isVisible());
            if (dialog) {
                dialog->setDirectory(fixturePath);
                auto* fileName = dialog->findChild<QLineEdit*>("fileNameEdit");
                check(fileName != nullptr);
                if (fileName) fileName->setText("\"gjb10157_defparam.sv\" \"gjb9765_case_buffer.vhd\"");
                check(QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection));
            }
            check(standard && !standard->isEnabled());
            QElapsedTimer timer;
            timer.start();
            while (!analyze->isEnabled() && timer.elapsed() < 15000) {
                app.processEvents();
                QThread::msleep(5);
            }
            check(analyze->isEnabled());
            check(window.statusBar()->currentMessage().contains(QStringLiteral("已分析 2 个文件")));
            check(window.windowTitle() == QStringLiteral("FPGA HDL Static Analyzer — 2 个 RTL 文件"));
            check(standard && standard->isEnabled());
            check(window.statusBar()->currentMessage().contains(QStringLiteral("规则评估 103 条")));
            check(statistics->rowCount() > 0 && problems->rowCount() > 0);
            for (int column = 0; column < statistics->columnCount(); ++column) {
                check(statistics->horizontalHeader()->sectionResizeMode(column) == QHeaderView::Interactive);
                check(statistics->columnWidth(column) == resizedStatisticsWidths[column]);
            }
            check(project->topLevelItemCount() == 1);
            const auto output = qEnvironmentVariable("FPGA_GUI_SCREENSHOT_DIR");
            if (!output.isEmpty()) {
                for (int column = 0; column < statistics->columnCount(); ++column)
                    statistics->horizontalHeader()->resizeSection(column, defaultStatisticsWidths[column]);
                statistics->horizontalScrollBar()->setValue(0);
                QDir().mkpath(output);
                app.processEvents();
                check(window.grab().save(output + "/statistics.png"));
                for (int column = 0; column < statistics->columnCount(); ++column)
                    statistics->horizontalHeader()->resizeSection(column, resizedStatisticsWidths[column]);
            }
            int fileRow = -1;
            for (int row = 0; row < statistics->rowCount(); ++row) {
                if (!statistics->item(row, 0)->data(Qt::UserRole + 1).toString().isEmpty()) { fileRow = row; break; }
            }
            check(fileRow >= 0);
            if (fileRow >= 0) statistics->cellDoubleClicked(fileRow, 0);
            check(pages->currentIndex() == 0 && !editor->toPlainText().isEmpty());
            problems->cellClicked(0, 0);
            check(!window.findChild<QPlainTextEdit*>("violationDetails")->toPlainText().isEmpty());
            if (!output.isEmpty()) {
                app.processEvents();
                check(window.grab().save(output + "/analysis.png"));
            }
            standard->setCurrentIndex(1);
            timer.restart();
            while (!analyze->isEnabled() && timer.elapsed() < 15000) {
                app.processEvents();
                QThread::msleep(5);
            }
            check(analyze->isEnabled() && standard->isEnabled());
            check(window.statusBar()->currentMessage().contains(QStringLiteral("规则评估 106 条")));
            // A single source with spaces in its name must not scan its directory.
            QTemporaryDir temporary;
            check(temporary.isValid());
            const auto singlePath = temporary.filePath("RTL source with spaces.vhd");
            check(QFile::copy(QDir(fixturePath).filePath("gjb9765_case_buffer.vhd"), singlePath));
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            window.findChild<QAction*>("openProjectAction")->trigger();
            app.processEvents();
            dialog = window.findChild<QFileDialog*>("rtlSourceDialog");
            check(dialog && dialog->isVisible());
            if (dialog) {
                dialog->setDirectory(temporary.path());
                auto* fileName = dialog->findChild<QLineEdit*>("fileNameEdit");
                check(fileName != nullptr);
                if (fileName) fileName->setText("\"RTL source with spaces.vhd\"");
                check(QMetaObject::invokeMethod(dialog, "accept", Qt::DirectConnection));
            }
            timer.restart();
            while (!analyze->isEnabled() && timer.elapsed() < 15000) {
                app.processEvents();
                QThread::msleep(5);
            }
            check(analyze->isEnabled());
            check(window.statusBar()->currentMessage().contains(QStringLiteral("已分析 1 个文件")));
            check(window.windowTitle() == QStringLiteral("FPGA HDL Static Analyzer — RTL source with spaces.vhd"));
            const auto previousTitle = window.windowTitle();
            const auto previousStatus = window.statusBar()->currentMessage();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            window.findChild<QAction*>("openProjectAction")->trigger();
            app.processEvents();
            dialog = window.findChild<QFileDialog*>("rtlSourceDialog");
            check(dialog && dialog->isVisible());
            if (dialog) dialog->reject();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
            check(window.windowTitle() == previousTitle && window.statusBar()->currentMessage() == previousStatus);
        }
        window.close();
        app.quit();
    });
    app.exec();
    if (failures) std::cerr << failures << " GUI checks failed\n";
    return failures == 0 ? 0 : 1;
}

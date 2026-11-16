#include "gui/main_window.h"

#include "analysis/baseline.h"
#include "analysis/standard_profiles.h"
#include "core/application_info.h"
#include "project/config_file.h"
#include "report/report_writer.h"

#include <QAction>
#include <QBrush>
#include <QColor>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFrame>
#include <QFileInfo>
#include <QFont>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocale>
#include <QPushButton>
#include <QStackedWidget>
#include <QSplitter>
#include <QButtonGroup>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QKeySequence>
#include <QPlainTextEdit>
#include <QSaveFile>
#include <QScrollBar>
#include <QStatusBar>
#include <QTableWidget>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QtConcurrentRun>

#include <filesystem>
#include <algorithm>
#include <array>
#include <cstdlib>
#include <limits>
#include <sstream>
#include <string_view>
#include <utility>

namespace fpga::gui {
namespace {

constexpr int FilePathRole = Qt::UserRole + 1;
constexpr int LineRole = Qt::UserRole + 2;
constexpr int ViolationIndexRole = Qt::UserRole + 3;

// Keep table geometry stable while a section boundary is being dragged.
// Resizing on every mouse move changes the horizontal scroll range and can make
// a divider appear to travel opposite to the pointer near a scroll boundary.
// A guide follows the pointer and the width is committed once on release.
class ColumnResizeHeader final : public QHeaderView {
public:
    explicit ColumnResizeHeader(QWidget* parent) : QHeaderView(Qt::Horizontal, parent) {
        setLayoutDirection(Qt::LeftToRight);
        setMouseTracking(true);
        setSectionResizeMode(QHeaderView::Interactive);
        setStretchLastSection(false);
        setCascadingSectionResizes(false);
        setSectionsMovable(false);
        resizeGuide_ = new QFrame(viewport());
        resizeGuide_->setObjectName(QStringLiteral("columnResizeGuide"));
        resizeGuide_->setFrameShape(QFrame::VLine);
        resizeGuide_->setLineWidth(2);
        resizeGuide_->setStyleSheet(QStringLiteral("color: #263d2d; background: #263d2d;"));
        resizeGuide_->hide();
    }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            const int column = boundaryAt(static_cast<int>(event->position().x()));
            if (column >= 0) {
                resizingColumn_ = column;
                adjacentColumn_ = nextVisibleSection(column);
                dragStartX_ = event->position().x();
                dragStartWidth_ = sectionSize(column);
                dragStartAdjacentWidth_ = adjacentColumn_ >= 0 ? sectionSize(adjacentColumn_) : 0;
                dragStartEdgeX_ = sectionViewportPosition(column) + sectionSize(column);
                resizeGuide_->setGeometry(dragStartEdgeX_, 0, 2, viewport()->height());
                resizeGuide_->show();
                resizeGuide_->raise();
                event->accept();
                return;
            }
        }
        QHeaderView::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        if (resizingColumn_ >= 0) {
            const int delta = boundedDelta(qRound(event->position().x() - dragStartX_));
            const int guideX = dragStartEdgeX_ + delta;
            resizeGuide_->setGeometry(guideX, 0, 2, viewport()->height());
            event->accept();
            return;
        }
        if (boundaryAt(static_cast<int>(event->position().x())) >= 0) {
            viewport()->setCursor(Qt::SplitHCursor);
            event->accept();
            return;
        }
        viewport()->unsetCursor();
        QHeaderView::mouseMoveEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent* event) override {
        if (resizingColumn_ >= 0 && event->button() == Qt::LeftButton) {
            const int delta = boundedDelta(qRound(event->position().x() - dragStartX_));
            // Keep the complete table width stable for an internal divider.
            // Growing the destination side first prevents a transient scrollbar
            // clamp, so the divider always moves in the pointer's direction.
            if (adjacentColumn_ >= 0) {
                if (delta < 0) {
                    resizeSection(adjacentColumn_, dragStartAdjacentWidth_ - delta);
                    resizeSection(resizingColumn_, dragStartWidth_ + delta);
                } else {
                    resizeSection(resizingColumn_, dragStartWidth_ + delta);
                    resizeSection(adjacentColumn_, dragStartAdjacentWidth_ - delta);
                }
            } else {
                resizeSection(resizingColumn_, dragStartWidth_ + delta);
            }
            resizingColumn_ = -1;
            adjacentColumn_ = -1;
            resizeGuide_->hide();
            event->accept();
            return;
        }
        QHeaderView::mouseReleaseEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        if (resizingColumn_ < 0) viewport()->unsetCursor();
        QHeaderView::leaveEvent(event);
    }

private:
    int nextVisibleSection(int column) const {
        const int visual = visualIndex(column);
        for (int next = visual + 1; next < count(); ++next) {
            const int candidate = logicalIndex(next);
            if (!isSectionHidden(candidate)) return candidate;
        }
        return -1;
    }

    int boundedDelta(int requested) const {
        int lower = minimumSectionSize() - dragStartWidth_;
        int upper = std::numeric_limits<int>::max();
        if (adjacentColumn_ >= 0)
            upper = dragStartAdjacentWidth_ - minimumSectionSize();
        return std::clamp(requested, lower, upper);
    }

    int boundaryAt(int x) const {
        constexpr int hitRadius = 8;
        for (int visual = 0; visual < count(); ++visual) {
            const int column = logicalIndex(visual);
            if (isSectionHidden(column)) continue;
            const int right = sectionViewportPosition(column) + sectionSize(column);
            if (right >= 0 && right <= viewport()->width() && std::abs(x - right) <= hitRadius)
                return column;
        }
        return -1;
    }

    int resizingColumn_{-1};
    int adjacentColumn_{-1};
    qreal dragStartX_{0};
    int dragStartWidth_{0};
    int dragStartAdjacentWidth_{0};
    int dragStartEdgeX_{0};
    QFrame* resizeGuide_{nullptr};
};

// QTableWidget stops painting cell dividers after the last data row. Keep the
// column boundaries visible across the entire result frame, including when a
// scan returns no rows, so the draggable header edges have a clear target.
class ResultTable final : public QTableWidget {
public:
    ResultTable(int rows, int columns, QWidget* parent) : QTableWidget(rows, columns, parent) {}

protected:
    void paintEvent(QPaintEvent* event) override {
        QTableWidget::paintEvent(event);
        if (!showGrid()) return;
        const int filledBottom = rowCount() > 0
            ? rowViewportPosition(rowCount() - 1) + rowHeight(rowCount() - 1)
            : 0;
        if (filledBottom >= viewport()->height()) return;
        QPainter painter(viewport());
        painter.setClipRect(event->rect());
        painter.setPen(QColor(QStringLiteral("#829783")));
        for (int column = 0; column < columnCount(); ++column) {
            if (isColumnHidden(column)) continue;
            const int edge = columnViewportPosition(column) + columnWidth(column) - 1;
            if (edge >= 0 && edge < viewport()->width())
                painter.drawLine(edge, std::max(0, filledBottom), edge, viewport()->height() - 1);
        }
        const int rowHeight = verticalHeader()->defaultSectionSize();
        if (rowHeight > 0) {
            for (int y = std::max(0, filledBottom) + rowHeight - 1;
                 y < viewport()->height(); y += rowHeight)
                painter.drawLine(0, y, viewport()->width() - 1, y);
        }
    }
};

struct RuleText {
    std::string_view id;
    const char* summary;
};

constexpr std::array ChineseRuleTexts{
    RuleText{"STYLE-001", "行尾存在多余空白"},
    RuleText{"STYLE-002", "代码行宽超过 120 列"},
    RuleText{"SV-ALWAYS-COMB-EVENT-CONTROL", "always_comb 中出现显式事件控制，破坏自动敏感语义"},
    RuleText{"SV-ALWAYS-FF-EVENT-CONTROL", "always_ff 没有事件控制或包含多个事件控制"},
    RuleText{"SV-ALWAYS-FF-NONEDGE-EVENT", "always_ff 的事件控制不包含 posedge 或 negedge"},
    RuleText{"SV-ALWAYS-LATCH-EVENT-CONTROL", "always_latch 中出现显式事件控制"},
    RuleText{"SV-ALWAYS-LATCH-NO-LATCH", "always_latch 没有推断出锁存器"},
    RuleText{"CDC-DIRECT-CROSSING", "寄存器信号在同一设计单元内直接跨越时钟域"},
    RuleText{"RDC-MULTI-CLOCK-ASYNC-RESET", "同一异步复位直接控制多个时钟域"},
    RuleText{"VL-SEQ-BLOCKING", "边沿触发过程中使用阻塞赋值"},
    RuleText{"VL-COMB-NONBLOCKING", "组合过程中使用非阻塞赋值"},
    RuleText{"VL-ALWAYS-NO-SUSPENSION", "普通 always 不含事件、延时或 wait 挂起点"},
    RuleText{"VL-EMPTY-CASE-ITEM", "case 选择项为空，不执行任何 RTL 行为"},
    RuleText{"VL-EMPTY-CONDITIONAL-BRANCH", "if/else 分支为空，可能存在多余分号或遗漏逻辑"},
    RuleText{"VL-IMPLICIT-NET", "使用了未声明标识符，可能创建隐式网络"},
    RuleText{"VL-MIXED-ASSIGNMENT-STYLE", "同一过程中对同一信号混用阻塞与非阻塞赋值"},
    RuleText{"VL-SIDE-EFFECT-CONDITION", "条件表达式包含会修改状态的赋值或自增/自减操作"},
    RuleText{"VL-CASEX", "使用 casex 可能隐藏 X/Z 状态问题"},
    RuleText{"VL-OVERLAPPING-WILDCARD-CASE", "casez/casex 通配模式存在交集，结果依赖源码顺序"},
    RuleText{"VL-CASE-DEFAULT", "case 语句缺少 default 分支"},
    RuleText{"VL-CASE-ITEM-VALUE-OUT-OF-RANGE", "case 选择值超出选择器可表示范围"},
    RuleText{"VHDL-NONSTD-NUMERIC", "使用了非标准 VHDL 数值运算包"},
    RuleText{"VHDL-PROCESS-NO-SUSPENSION", "VHDL process 没有敏感列表或 wait 挂起点"},
    RuleText{"VHDL-SHARED-VARIABLE", "使用了需要同步保护的 shared variable"},
    RuleText{"VHDL-WAIT", "RTL 中使用了综合支持依赖工具的 wait 语句"},
    RuleText{"VHDL-WAIT-WITH-SENSITIVITY", "带敏感列表的 process 中出现显式 wait"},
    RuleText{"STRUCT-DUPLICATE-UNIT", "项目中存在重名 module 或 entity 定义"},
    RuleText{"STRUCT-DUPLICATE-NAMED-PARAMETER", "同一实例重复覆盖同一命名参数"},
    RuleText{"STRUCT-DUPLICATE-NAMED-PORT", "同一实例重复连接同一命名端口"},
    RuleText{"STRUCT-DUPLICATE-PARAMETER-DECLARATION", "接口中重复声明 parameter 或 generic"},
    RuleText{"STRUCT-DUPLICATE-PORT-DECLARATION", "接口中重复声明同名端口"},
    RuleText{"STRUCT-DUPLICATE-SIGNAL-DECLARATION", "同一作用域内重复声明信号"},
    RuleText{"STRUCT-MISSING-REQUIRED-PARAMETER", "实例没有覆盖无默认值的必需参数"},
    RuleText{"STRUCT-MIXED-PARAMETER-OVERRIDE-STYLE", "参数覆盖列表混用位置和命名方式"},
    RuleText{"STRUCT-UNRESOLVED-INSTANCE", "实例目标不在当前分析输入或 black box 声明中"},
    RuleText{"STRUCT-UNKNOWN-NAMED-PORT", "命名连接端口不存在于目标接口"},
    RuleText{"STRUCT-UNKNOWN-NAMED-PARAMETER", "命名参数不存在于目标接口"},
    RuleText{"STRUCT-UNCONNECTED-INPUT-PORT", "实例遗漏或悬空必需输入端口"},
    RuleText{"STRUCT-UNCONNECTED-OUTPUT-PORT", "实例遗漏或悬空输出/双向端口"},
    RuleText{"STRUCT-VHDL-ASSOCIATION-ORDER", "VHDL association 顺序无效"},
    RuleText{"STRUCT-MIXED-PORT-CONNECTION-STYLE", "同一实例混用位置和命名端口连接"},
    RuleText{"STRUCT-POSITIONAL-PORT-COUNT", "位置端口连接数量超过目标接口端口数量"},
    RuleText{"STRUCT-POSITIONAL-PARAMETER-COUNT", "位置参数覆盖数量超过目标声明数量"},
    RuleText{"STRUCT-RECURSIVE-INSTANCE", "module/entity 实例关系形成递归环路"},
    RuleText{"SYNTAX-UNTERMINATED-COMMENT", "块注释到文件结尾仍未闭合"},
    RuleText{"SYNTAX-UNTERMINATED-STRING", "字符串到行尾或文件结尾仍未闭合"},
    RuleText{"SYNTAX-UNMATCHED-DELIMITER", "右括号没有匹配的左括号"},
    RuleText{"SYNTAX-UNCLOSED-DELIMITER", "左括号没有匹配的右括号"},
    RuleText{"SYNTAX-UNEXPECTED-CLOSER", "结束关键字没有对应的起始结构"},
    RuleText{"SYNTAX-UNCLOSED-CONSTRUCT", "HDL 语言结构没有正确闭合"},
    RuleText{"PP-INCLUDE-NOT-FOUND", "找不到 include 指令引用的文件"},
    RuleText{"PP-UNEXPECTED-DIRECTIVE", "条件编译结束/分支指令没有对应起点"},
    RuleText{"PP-INVALID-DIRECTIVE", "define 或 include 指令参数无效"},
    RuleText{"PP-UNCLOSED-CONDITIONAL", "条件编译块缺少对应的 endif"},
    RuleText{"RTL-ASYNC-RESET-POLARITY-MISMATCH", "异步复位事件边沿与复位分支有效电平不一致"},
    RuleText{"RTL-ASYNC-RESET-NOT-FIRST", "异步复位没有作为过程中的首个独立条件"},
    RuleText{"RTL-ASYNC-RESET-UNGUARDED-LOGIC", "异步复位条件链之后仍有未受保护的独立赋值"},
    RuleText{"RTL-CASE-ITEM-WIDTH-MISMATCH", "case 选择项宽度与选择器宽度不一致"},
    RuleText{"RTL-BITWISE-WIDTH-MISMATCH", "位运算两侧的静态位宽不一致"},
    RuleText{"RTL-MIXED-SIGNEDNESS-ARITHMETIC", "算术表达式混合使用有符号和无符号信号"},
    RuleText{"RTL-MULTIPLE-DRIVERS", "信号由多个独立来源驱动"},
    RuleText{"RTL-NEGATIVE-SHIFT-AMOUNT", "移位操作使用负的常量移位量"},
    RuleText{"RTL-MULTIPLE-CASE-DEFAULT", "同一 case 包含多个兜底分支"},
    RuleText{"RTL-COMBINATIONAL-LOOP", "组合逻辑信号依赖形成反馈环路"},
    RuleText{"RTL-COMPARISON-WIDTH-MISMATCH", "比较运算两侧的静态位宽不一致"},
    RuleText{"RTL-COMPLEX-EDGE-EVENT", "边沿事件使用组合表达式、位选或索引"},
    RuleText{"RTL-CONSTANT-CONDITION", "运行时条件可以被静态证明为常量"},
    RuleText{"RTL-DERIVED-CLOCK", "本地 RTL 生成的信号被用作时钟"},
    RuleText{"RTL-DIVIDE-BY-ZERO", "除法、取模或余数运算使用常量零分母"},
    RuleText{"RTL-DUPLICATE-CASE-ITEM", "同一 case 中存在完全重复的选择项"},
    RuleText{"RTL-DUPLICATE-CONDITION", "同一条件分支链中存在重复条件"},
    RuleText{"RTL-DUPLICATE-SENSITIVITY", "显式敏感列表包含重复项"},
    RuleText{"RTL-EMPTY-PROCESS", "过程没有可执行行为"},
    RuleText{"RTL-NONSYNTHESIZABLE-DELAY", "RTL 中使用了不可移植综合的仿真延时"},
    RuleText{"RTL-OUT-OF-RANGE-SELECT", "常量位索引或切片边界超出信号声明范围"},
    RuleText{"RTL-INPUT-DRIVEN", "输入端口在设计单元内部被驱动"},
    RuleText{"RTL-UNDRIVEN-OUTPUT", "输出端口没有驱动源"},
    RuleText{"RTL-UNDRIVEN-SIGNAL", "被读取的内部信号没有驱动源"},
    RuleText{"RTL-UNUSED-ASYNC-RESET", "异步复位位于敏感列表中但过程体没有读取该信号"},
    RuleText{"RTL-UNUSED-INPUT-PORT", "输入端口未被本地逻辑使用或转发"},
    RuleText{"RTL-UNUSED-PARAMETER", "parameter、localparam 或 generic 从未被引用"},
    RuleText{"RTL-UNUSED-SIGNAL", "内部信号从未被读取"},
    RuleText{"RTL-WRITE-ONLY-SIGNAL", "内部信号有驱动但没有负载或使用"},
    RuleText{"RTL-WIDTH-TRUNCATION", "赋值源宽度大于目标宽度，可能发生截断"},
    RuleText{"RTL-INCOMPLETE-SENSITIVITY", "组合过程敏感列表遗漏了读取信号"},
    RuleText{"RTL-REDUNDANT-SENSITIVITY", "组合过程敏感列表包含未读取信号"},
    RuleText{"RTL-LATCH-INFERRED", "组合过程部分控制路径未赋值，可能推断锁存器"},
    RuleText{"RTL-MIXED-CLOCK-EDGE", "同一时钟同时用于上升沿和下降沿过程"},
    RuleText{"RTL-MIXED-RESET-MODE", "同一复位信号同时采用同步和异步方式"},
    RuleText{"RTL-RESET-POLARITY-CONFLICT", "同一复位信号的有效极性不一致"},
    RuleText{"RTL-SELF-ASSIGNMENT", "过程赋值将信号直接写回自身"},
    RuleText{"RTL-SHIFT-AMOUNT-OUT-OF-RANGE", "常量移位量大于或等于操作数位宽"},
    RuleText{"RTL-MULTIPLE-CLOCKS-PER-PROCESS", "同一时序过程包含多个独立时钟"},
};
static_assert(ChineseRuleTexts.size() == 93);

QString chineseRuleSummary(std::string_view ruleId) {
    if (const auto detail = analysis::findStandardRuleDetail(ruleId)) {
        return QString::fromUtf8(detail->summary.data(), static_cast<int>(detail->summary.size()));
    }
    const auto found = std::find_if(ChineseRuleTexts.begin(), ChineseRuleTexts.end(),
        [ruleId](const RuleText& entry) { return entry.id == ruleId; });
    if (found == ChineseRuleTexts.end()) return QStringLiteral("检测到 HDL 代码问题");
    return QString::fromUtf8(found->summary);
}

std::filesystem::path filesystemPath(const QString& path) {
#ifdef _WIN32
    return std::filesystem::path(path.toStdWString());
#else
    return std::filesystem::path(path.toUtf8().constData());
#endif
}

QString displayPath(const std::filesystem::path& path) {
#ifdef _WIN32
    return QString::fromStdWString(path.wstring());
#else
    const auto value = path.string();
    return QString::fromUtf8(value.data(), static_cast<int>(value.size()));
#endif
}

QString severityText(analysis::Severity severity) {
    switch (severity) {
    case analysis::Severity::Critical: return QStringLiteral("严重");
    case analysis::Severity::Error: return QStringLiteral("错误");
    case analysis::Severity::Warning: return QStringLiteral("警告");
    case analysis::Severity::Info: return QStringLiteral("提示");
    }
    return {};
}

QString designUnitKindText(parser::DesignUnitKind kind) {
    switch (kind) {
    case parser::DesignUnitKind::VerilogModule: return QStringLiteral("Verilog 模块");
    case parser::DesignUnitKind::VhdlEntity: return QStringLiteral("VHDL 实体");
    }
    return QStringLiteral("未知");
}

QString portDirectionText(parser::PortDirection direction) {
    switch (direction) {
    case parser::PortDirection::Input: return QStringLiteral("输入");
    case parser::PortDirection::Output: return QStringLiteral("输出");
    case parser::PortDirection::Inout: return QStringLiteral("双向");
    case parser::PortDirection::Unknown: return QStringLiteral("未知");
    }
    return QStringLiteral("未知");
}

QColor severityColor(analysis::Severity severity) {
    switch (severity) {
    case analysis::Severity::Critical:
    case analysis::Severity::Error: return QColor("#AD4D47");
    case analysis::Severity::Warning: return QColor("#986528");
    case analysis::Severity::Info: return QColor("#49645B");
    }
    return {};
}

analysis::AnalysisResult analyzeProject(const QString& projectPath, const QStringList& sourcePaths,
                                       const QString& standardId) {
    const auto root = filesystemPath(projectPath);
    analysis::AnalysisOptions options;
    options.standardId = standardId.toStdString();
    std::optional<std::filesystem::path> baselinePath;
    const auto configPath = root / ".fpga-lint.conf";
    std::error_code error;
    if (std::filesystem::is_regular_file(configPath, error)) {
        auto loaded = project::loadConfigFile(configPath);
        if (!loaded.errors.empty()) {
            analysis::AnalysisResult result;
            result.errors = std::move(loaded.errors);
            return result;
        }
        options = std::move(loaded.config.analysis);
        options.standardId = standardId.toStdString();
        baselinePath = std::move(loaded.config.baseline);
    }
    std::vector<std::filesystem::path> inputs;
    for (const auto& path : sourcePaths) inputs.push_back(filesystemPath(path));
    if (inputs.empty()) inputs.push_back(root); // Legacy directory/CLI startup compatibility.
    auto result = analysis::Analyzer(std::move(options)).analyzePaths(inputs);
    if (baselinePath) {
        auto baseline = analysis::loadBaseline(*baselinePath);
        if (!baseline.errors.empty()) {
            result.errors.insert(result.errors.end(), baseline.errors.begin(), baseline.errors.end());
        } else {
            analysis::applyBaseline(result, baseline.entries);
        }
    }
    return result;
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    const auto info = application_info();
    setWindowTitle(QString::fromUtf8(info.name.data(), static_cast<int>(info.name.size())));
    resize(1440, 940);
    setMinimumSize(1050, 720);

    sourceEditor_ = new QPlainTextEdit(this);
    sourceEditor_->setObjectName("sourceEditor");
    sourceEditor_->setReadOnly(true);
    sourceEditor_->setLineWrapMode(QPlainTextEdit::NoWrap);
    sourceEditor_->setPlaceholderText(tr("选择 RTL 源码文件后即可分析；支持多选 Verilog、SystemVerilog 和 VHDL。"));
    sourceEditor_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    projectTree_ = new QTreeWidget(this);
    projectTree_->setObjectName("projectTree");
    projectTree_->setColumnCount(3);
    projectTree_->setHeaderLabels({tr("设计单元 / 实例"), tr("类型"), tr("源文件")});
    projectTree_->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    projectTree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    projectTree_->header()->setSectionResizeMode(2, QHeaderView::Stretch);
    auto* emptyRoot = new QTreeWidgetItem(projectTree_, {tr("尚未加载工程")});
    emptyRoot->setFlags(Qt::NoItemFlags);
    projectTree_->setHeaderHidden(true);
    projectTree_->setColumnHidden(1, true);
    projectTree_->setColumnHidden(2, true);

    problemsTable_ = new ResultTable(0, 6, this);
    problemsTable_->setObjectName("problemsTable");
    problemsTable_->setLayoutDirection(Qt::LeftToRight);
    problemsTable_->setHorizontalHeader(new ColumnResizeHeader(problemsTable_));
    problemsTable_->setHorizontalHeaderLabels(
        {tr("级别"), tr("规则"), tr("问题描述"), tr("单元 / 信号"), tr("文件"), tr("行号")});
    problemsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    problemsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    problemsTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    problemsTable_->setSortingEnabled(true);
    problemsTable_->horizontalHeader()->setMinimumSectionSize(48);
    problemsTable_->setColumnWidth(0, 65);
    problemsTable_->setColumnWidth(1, 235);
    problemsTable_->setColumnWidth(2, 480);
    problemsTable_->setColumnWidth(3, 120);
    problemsTable_->setColumnWidth(4, 165);
    problemsTable_->setColumnWidth(5, 55);
    problemsTable_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    statisticsTable_ = new ResultTable(0, 6, this);
    statisticsTable_->setObjectName("statisticsTable");
    statisticsTable_->setLayoutDirection(Qt::LeftToRight);
    statisticsTable_->setHorizontalHeader(new ColumnResizeHeader(statisticsTable_));
    statisticsTable_->setHorizontalHeaderLabels(
        {tr("文件 / 语言汇总"), tr("语言 / 文件数"), tr("总行数"), tr("代码行"), tr("注释行"), tr("空白行")});
    statisticsTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    statisticsTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    statisticsTable_->setSortingEnabled(true);
    statisticsTable_->horizontalHeader()->setMinimumSectionSize(60);
    statisticsTable_->setColumnWidth(0, 420);
    statisticsTable_->setColumnWidth(1, 190);
    for (int column = 2; column < 6; ++column) statisticsTable_->setColumnWidth(column, 125);
    statisticsTable_->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    statisticsTable_->setToolTip(tr("统计原始源码，包含条件编译分支。混合代码/注释行计为代码行；总行数 = 代码行 + 注释行 + 空白行。双击文件查看源码。"));

    violationDetails_ = new QPlainTextEdit(this);
    violationDetails_->setObjectName("violationDetails");
    violationDetails_->setReadOnly(true);
    violationDetails_->setPlaceholderText(tr("选择问题后可查看详细说明和修改建议。"));

    auto* fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    openProjectAction_ = fileMenu->addAction(tr("打开 RTL 源码(&O)…"));
    openProjectAction_->setObjectName("openProjectAction");
    exportHtmlAction_ = fileMenu->addAction(tr("导出 HTML 报告(&H)…"));
    exportSarifAction_ = fileMenu->addAction(tr("导出 SARIF 报告(&S)…"));
    exportStatisticsAction_ = fileMenu->addAction(tr("导出行数统计 CSV(&C)…"));
    exportStatisticsAction_->setEnabled(false);
    exportHtmlAction_->setEnabled(false);
    exportSarifAction_->setEnabled(false);
    fileMenu->addSeparator();
    auto* exitAction = fileMenu->addAction(tr("退出(&X)"));

    auto* analysisMenu = menuBar()->addMenu(tr("分析(&A)"));
    analyzeAction_ = analysisMenu->addAction(tr("运行分析(&R)"));
    analyzeAction_->setObjectName("runAnalysisAction");
    analyzeAction_->setEnabled(false);
    analyzeAction_->setShortcut(QKeySequence(QStringLiteral("Ctrl+R")));

    auto* viewMenu = menuBar()->addMenu(tr("视图(&V)"));
    auto* helpMenu = menuBar()->addMenu(tr("帮助(&H)"));
    auto* aboutAction = helpMenu->addAction(tr("关于(&B)"));

    // Figma tokens are shared by both pages; all contents remain native, interactive Qt widgets.
    setStyleSheet(QStringLiteral(R"(
        QMainWindow, QWidget#workbench { background: white; }
        QWidget { color: #252724; font-size: 13px; }
        QMenuBar { background: white; color: #747770; padding: 8px; }
        QStatusBar { background: white; color: #747770; padding: 8px; font-size: 11px; border-top: 1px solid #f2f3ef; }
        QWidget#navigation { background: #f6f6f3; }
        QWidget#navigation QLabel { color: #444b45; }
        QPushButton { background: #f6f6f3; border: 1px solid #e6e7e2; border-radius: 6px; padding: 10px 16px; }
        QPushButton:hover { background: #eceee8; border-color: #cdd2c8; }
        QPushButton:focus { border-color: #49645b; }
        QPushButton:disabled { color: #989c94; background: #fafaf8; border-color: #eceee8; }
        QPushButton#primary { background: #2c2c2c; color: white; border-color: #2c2c2c; }
        QPushButton#primary:hover { background: #444b45; border-color: #444b45; }
        QPushButton#primary:disabled { background: #c9cdc5; border-color: #c9cdc5; }
        QPushButton[nav="true"] { text-align: left; background: transparent; color: #444b45; border: none; height: 22px; }
        QPushButton[nav="true"]:checked { background: #eeefea; color: #252724; font-weight: 500; }
        QPushButton[nav="true"]:hover { background: #eceee8; }
        QPushButton[nav="true"]:focus { border: 1px solid #cdd2c8; }
        QWidget#metricCard, QWidget#panel { background: white; border: none; }
        QFrame#tablePanel, QFrame#statisticsPanel { background: white; border: 2px solid #263d2d; border-radius: 3px; }
        QLabel#tablePanelTitle, QWidget#statisticsTitleBar { background: #e8eee7; border-bottom: 2px solid #263d2d; }
        QLabel#tablePanelTitle, QLabel#statisticsPanelTitle { color: #252724; font-weight: 600; }
        QWidget#codePanel { background: #fafaf8; border: none; border-radius: 6px; }
        QLabel#metricValue { font-size: 28px; font-weight: 500; }
        QLabel#caption { color: #747770; font-size: 11px; }
        QLabel#heading { font-size: 20px; font-weight: 500; }
        QLabel#notice { background: transparent; color: #747770; padding: 4px 16px; font-size: 11px; }
        QPlainTextEdit { background: white; border: none; padding: 12px; }
        QPlainTextEdit#sourceEditor { background: #fafaf8; color: #444b45; selection-background-color: #e1e8df; selection-color: #252724; }
        QTreeWidget { background: transparent; color: #444b45; border: none; }
        QTreeWidget::item { padding: 6px 0; }
        QTreeWidget::item:selected { background: #e8ebe3; color: #252724; }
        QTableWidget#problemsTable, QTableWidget#statisticsTable { background: white; alternate-background-color: #fafaf8; border: 1px solid #526956; border-radius: 0; gridline-color: #6f846f; selection-background-color: #e8efe7; selection-color: #252724; }
        QHeaderView::section { background: #edf2ec; color: #303b32; border: 0; border-right: 1px solid #526956; border-bottom: 2px solid #263d2d; padding: 8px; font-size: 11px; font-weight: 600; }
        QSplitter::handle { background: #e0e6dc; }
        QSplitter::handle:hover { background: #bfc9bd; }
        QScrollBar:vertical { background: transparent; width: 8px; margin: 0; }
        QScrollBar:horizontal { background: transparent; height: 8px; margin: 0; }
        QScrollBar::handle { background: #d4d8ce; border-radius: 4px; min-height: 24px; min-width: 24px; }
        QScrollBar::handle:hover { background: #acb5a5; }
        QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
        QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }
    )"));
    auto* workbench = new QWidget(this);
    workbench->setObjectName("workbench");
    auto* shell = new QHBoxLayout(workbench);
    shell->setContentsMargins(0, 0, 0, 0);
    shell->setSpacing(0);
    auto* navigation = new QWidget(workbench);
    navigation->setObjectName("navigation");
    navigation->setFixedWidth(240);
    auto* nav = new QVBoxLayout(navigation);
    nav->setContentsMargins(16, 24, 16, 16);
    nav->setSpacing(12);
    auto* logo = new QLabel(QStringLiteral("HDL"), navigation);
    logo->setStyleSheet("color: #252724; font-size: 20px; font-weight: 500;");
    nav->addWidget(logo);
    auto* workspaceCaption = new QLabel(tr("工作空间"), navigation);
    workspaceCaption->setObjectName("caption");
    nav->addWidget(workspaceCaption);
    projectName_ = new QLabel(tr("尚未加载工程"), navigation);
    projectName_->setWordWrap(true);
    nav->addWidget(projectName_);
    nav->addSpacing(16);
    pages_ = new QStackedWidget(workbench);
    pages_->setObjectName("workspacePages");
    auto* group = new QButtonGroup(this);
    for (int page = 0; page < 2; ++page) {
        const auto title = page == 0 ? tr("分析工作台") : tr("代码行数统计");
        auto* button = new QPushButton(title, navigation);
        button->setObjectName(page == 0 ? "analysisNavigation" : "statisticsNavigation");
        button->setProperty("nav", true);
        button->setCheckable(true);
        group->addButton(button, page);
        nav->addWidget(button);
        auto* action = viewMenu->addAction(title);
        connect(button, &QPushButton::clicked, this, [this, page] { pages_->setCurrentIndex(page); });
        connect(action, &QAction::triggered, button, &QPushButton::click);
        if (page == 0) button->setChecked(true);
    }
    connect(pages_, &QStackedWidget::currentChanged, this, [group](int page) { group->button(page)->setChecked(true); });
    nav->addSpacing(24);
    auto* hierarchyCaption = new QLabel(tr("设计层次"), navigation);
    hierarchyCaption->setObjectName("caption");
    projectTree_->setToolTip(tr("双击设计节点打开源文件。"));
    nav->addWidget(hierarchyCaption);
    nav->addWidget(projectTree_, 1);
    auto* languageNote = new QLabel(QStringLiteral("Verilog / SystemVerilog / VHDL"), navigation);
    languageNote->setStyleSheet("font-size: 11px;");
    languageNote->hide();
    shell->addWidget(navigation);
    shell->addWidget(pages_, 1);
    const std::array titles{tr("源文件"), tr("实例"), tr("警告 / 错误"), tr("提示"),
                            tr("总行数"), tr("代码行"), tr("注释行"), tr("空白行")};
    auto panel = [](const QString& title, QWidget* content) {
        auto* widget = new QWidget;
        widget->setObjectName("panel");
        auto* layout = new QVBoxLayout(widget);
        layout->setContentsMargins(0, 0, 0, 0);
        auto* label = new QLabel(title);
        label->setMargin(12);
        layout->addWidget(label);
        layout->addWidget(content, 1);
        return widget;
    };
    for (int page = 0; page < 2; ++page) {
        auto* widget = new QWidget(pages_);
        auto* layout = new QVBoxLayout(widget);
        layout->setContentsMargins(24, 32, 24, 24);
        layout->setSpacing(page == 0 ? 16 : 20);
        auto* heading = new QHBoxLayout;
        auto* label = new QLabel(page == 0 ? tr("工程分析") : tr("代码行数统计"));
        label->setObjectName("heading");
        heading->addWidget(label, 1);
        if (page == 0) {
            heading->addWidget(new QLabel(tr("规则集")));
            standardSelector_ = new QComboBox(widget);
            standardSelector_->setObjectName("standardSelector");
            standardSelector_->addItem(QStringLiteral("GJB10157-2021 · Verilog"),
                                       QStringLiteral("GJB10157-2021"));
            standardSelector_->addItem(QStringLiteral("GJB9765-2020 · VHDL"),
                                       QStringLiteral("GJB9765-2020"));
            standardSelector_->setToolTip(tr("FPGA Analyzer 仅提供这两个 GJB 规则集。"));
            heading->addWidget(standardSelector_);
        }
        for (auto* action : {openProjectAction_, analyzeAction_}) {
            auto* button = new QPushButton(action == openProjectAction_ ? tr("打开 RTL 源码") : tr("运行分析"));
            if (action == analyzeAction_) button->setObjectName("primary");
            button->setEnabled(action->isEnabled());
            connect(action, &QAction::changed, button, [action, button] { button->setEnabled(action->isEnabled()); });
            connect(button, &QPushButton::clicked, action, &QAction::trigger);
            heading->addWidget(button);
        }
        layout->addLayout(heading);
        auto* cards = new QHBoxLayout;
        cards->setSpacing(16);
        for (int i = page * 4; i < page * 4 + 4; ++i) {
            auto* card = new QWidget;
            card->setObjectName("metricCard");
            card->setFixedHeight(106);
            auto* cardLayout = new QVBoxLayout(card);
            cardLayout->setContentsMargins(16, 10, 16, 10);
            cardLayout->setSpacing(4);
            auto* title = new QLabel(titles[i]);
            title->setObjectName("caption");
            metricValues_[i] = new QLabel(QStringLiteral("—"));
            metricValues_[i]->setObjectName("metricValue");
            metricNotes_[i] = new QLabel(tr("等待工程分析"));
            metricNotes_[i]->setObjectName("caption");
            cardLayout->addWidget(title);
            cardLayout->addWidget(metricValues_[i]);
            cardLayout->addWidget(metricNotes_[i]);
            cards->addWidget(card, 1);
        }
        layout->addLayout(cards);
        if (page == 0) {
            auto* editorPanel = panel(tr("源码预览"), sourceEditor_);
            editorPanel->setObjectName("codePanel");
            sourceTitle_ = editorPanel->findChild<QLabel*>();
            auto* editorSplitter = new QSplitter(Qt::Horizontal);
            editorSplitter->setHandleWidth(6);
            editorSplitter->addWidget(editorPanel);
            editorSplitter->addWidget(panel(tr("问题详情"), violationDetails_));
            editorSplitter->setStretchFactor(0, 3);
            editorSplitter->setStretchFactor(1, 1);
            editorSplitter->setSizes({824, 312});
            auto* body = new QSplitter(Qt::Vertical);
            body->setHandleWidth(6);
            body->addWidget(editorSplitter);
            auto* problemsPanel = new QFrame;
            auto* problemsLayout = new QVBoxLayout(problemsPanel);
            problemsLayout->setContentsMargins(0, 0, 0, 0);
            problemsLayout->setSpacing(0);
            auto* problemsTitle = new QLabel(tr("问题列表"), problemsPanel);
            problemsTitle->setMargin(12);
            problemsLayout->addWidget(problemsTitle);
            problemsLayout->addWidget(problemsTable_, 1);
            problemsPanel->setObjectName("tablePanel");
            problemsPanel->setAttribute(Qt::WA_StyledBackground, true);
            problemsPanel->setFrameShape(QFrame::Box);
            problemsPanel->setFrameShadow(QFrame::Plain);
            problemsPanel->setLineWidth(2);
            problemsTitle->setObjectName("tablePanelTitle");
            body->addWidget(problemsPanel);
            problemsTable_->setToolTip(tr("单击问题查看源码与修改建议；点击表头排序。"));
            body->setSizes({344, 234});
            layout->addWidget(body, 1);
        } else {
            auto* notice = new QLabel(tr("统计口径   总行数 = 代码行 + 注释行 + 空白行；代码与注释混合的行计为代码行。"));
            notice->setWordWrap(true);
            notice->setObjectName("notice");
            layout->addWidget(notice);
            auto* statisticsPanel = new QFrame(widget);
            statisticsPanel->setObjectName("statisticsPanel");
            statisticsPanel->setAttribute(Qt::WA_StyledBackground, true);
            statisticsPanel->setFrameShape(QFrame::Box);
            statisticsPanel->setFrameShadow(QFrame::Plain);
            statisticsPanel->setLineWidth(2);
            auto* statisticsLayout = new QVBoxLayout(statisticsPanel);
            statisticsLayout->setContentsMargins(0, 0, 0, 0);
            statisticsLayout->setSpacing(0);
            auto* titleBar = new QWidget(statisticsPanel);
            titleBar->setObjectName("statisticsTitleBar");
            auto* exports = new QHBoxLayout(titleBar);
            exports->setContentsMargins(12, 6, 12, 6);
            auto* statisticsTitle = new QLabel(tr("代码规模明细"), titleBar);
            statisticsTitle->setObjectName("statisticsPanelTitle");
            exports->addWidget(statisticsTitle, 1);
            for (auto* action : {exportStatisticsAction_, exportHtmlAction_}) {
                auto* button = new QPushButton(action == exportStatisticsAction_ ? tr("导出 CSV") : tr("导出 HTML"));
                button->setEnabled(false);
                connect(action, &QAction::changed, button, [action, button] { button->setEnabled(action->isEnabled()); });
                connect(button, &QPushButton::clicked, action, &QAction::trigger);
                exports->addWidget(button);
            }
            statisticsLayout->addWidget(titleBar);
            statisticsLayout->addWidget(statisticsTable_);
            layout->addWidget(statisticsPanel, 1);
            auto* hint = new QLabel(tr("双击文件查看源码 · 结果可离线导出"));
            hint->setObjectName("caption");
            layout->addWidget(hint);
        }
        pages_->addWidget(widget);
    }
    for (auto* table : {problemsTable_, statisticsTable_}) {
        table->setAlternatingRowColors(false);
        table->setShowGrid(true);
        table->setGridStyle(Qt::SolidLine);
        table->setFrameShape(QFrame::Box);
        table->setFrameShadow(QFrame::Plain);
        table->setLineWidth(1);
        table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        table->verticalHeader()->hide();
        table->verticalHeader()->setDefaultSectionSize(40);
        table->horizontalHeader()->setToolTip(tr("拖动列标题右侧的分隔线：向左收窄，向右加宽；超出框体时使用底部滚动条。"));
    }
    setCentralWidget(workbench);

    connect(openProjectAction_, &QAction::triggered, this, &MainWindow::openProject);
    connect(analyzeAction_, &QAction::triggered, this, &MainWindow::runAnalysis);
    connect(standardSelector_, &QComboBox::currentIndexChanged, this, [this] {
        if (!projectPath_.isEmpty() && !analysisWatcher_.isRunning()) runAnalysis();
    });
    connect(exportHtmlAction_, &QAction::triggered, this, [this] { exportReport("html"); });
    connect(exportSarifAction_, &QAction::triggered, this, [this] { exportReport("sarif"); });
    connect(exportStatisticsAction_, &QAction::triggered, this, [this] { exportReport("csv"); });
    connect(statisticsTable_, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        const auto path = statisticsTable_->item(row, 0)->data(FilePathRole).toString();
        if (!path.isEmpty()) loadSourceLocation(path, 1);
    });
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    connect(aboutAction, &QAction::triggered, this, [this] {
        const auto application = application_info();
        const auto version = QString::fromUtf8(application.version.data(),
                                               static_cast<int>(application.version.size()));
        QMessageBox::about(this, tr("关于 FPGA HDL Static Analyzer"),
            tr("FPGA HDL Static Analyzer %1\n支持多语言 HDL 代码检查、RTL 风险检查、设计层次分析和报告导出。")
                .arg(version));
    });
    connect(&analysisWatcher_, &QFutureWatcher<analysis::AnalysisResult>::finished,
            this, &MainWindow::analysisFinished);
    connect(problemsTable_, &QTableWidget::cellActivated,
            this, [this](int row, int) { showViolation(row); });
    connect(problemsTable_, &QTableWidget::cellClicked,
            this, [this](int row, int) { showViolation(row); });
    connect(projectTree_, &QTreeWidget::itemActivated,
            this, [this](QTreeWidgetItem* item, int) { openTreeLocation(item); });

    statusBar()->showMessage(tr("就绪 — 请选择 RTL 源码文件开始分析（支持多选）"));
}

void MainWindow::openProject() {
    // An explicit non-blocking Qt picker avoids native panel display problems.
    if (auto* existing = findChild<QFileDialog*>("rtlSourceDialog")) {
        existing->raise();
        existing->activateWindow();
        return;
    }
    auto* dialog = new QFileDialog(this, tr("选择 RTL 源码文件（支持多选）"),
        projectPath_.isEmpty() ? QDir::homePath() : projectPath_);
    dialog->setObjectName("rtlSourceDialog");
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setOption(QFileDialog::DontUseNativeDialog);
    dialog->setFileMode(QFileDialog::ExistingFiles);
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    dialog->setNameFilters({tr("RTL 源码 (*.v *.sv *.vhd *.vhdl)"),
        tr("Verilog / SystemVerilog (*.v *.sv)"), tr("VHDL (*.vhd *.vhdl)")});
    dialog->setLabelText(QFileDialog::LookIn, tr("位置："));
    dialog->setLabelText(QFileDialog::FileName, tr("源码文件："));
    dialog->setLabelText(QFileDialog::FileType, tr("文件类型："));
    dialog->setLabelText(QFileDialog::Accept, tr("打开源码"));
    dialog->setLabelText(QFileDialog::Reject, tr("取消"));
    dialog->resize(860, 560);
    connect(dialog, &QFileDialog::accepted, this, [this, dialog] {
        const auto selected = dialog->selectedFiles();
        if (!selected.isEmpty()) openSourceFiles(selected);
    });
    dialog->open();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::openProjectPath(const QString& selected) {
    if (QFileInfo(selected).isFile()) {
        openSourceFiles({selected});
        return;
    }
    if (analysisWatcher_.isRunning() || !QFileInfo(selected).isDir()) return;
    sourcePaths_.clear();
    projectPath_ = QDir(selected).absolutePath();
    projectName_->setText(QFileInfo(projectPath_).fileName());
    projectName_->setToolTip(projectPath_);
    setWindowTitle(QStringLiteral("FPGA HDL Static Analyzer — %1").arg(QFileInfo(projectPath_).fileName()));
    analyzeAction_->setEnabled(true);
    runAnalysis();
}

void MainWindow::openSourceFiles(const QStringList& paths) {
    if (analysisWatcher_.isRunning() || paths.isEmpty()) return;
    QStringList selected;
    for (const auto& path : paths) {
        const QFileInfo file(path);
        const auto suffix = file.suffix().toLower();
        if (!file.isFile() || !QStringList{"v", "sv", "vhd", "vhdl"}.contains(suffix)) {
            QMessageBox::warning(this, tr("无法打开源码"), tr("请选择有效的 RTL 源码文件：\n%1").arg(path));
            return;
        }
        selected.push_back(file.absoluteFilePath());
    }
    selected.removeDuplicates();
    sourcePaths_ = selected;
    projectPath_ = QFileInfo(selected.first()).absolutePath();
    const auto name = selected.size() == 1 ? QFileInfo(selected.first()).fileName()
        : tr("%1 个 RTL 文件").arg(selected.size());
    projectName_->setText(name);
    projectName_->setToolTip(selected.join('\n'));
    setWindowTitle(QStringLiteral("FPGA HDL Static Analyzer — %1").arg(name));
    analyzeAction_->setEnabled(true);
    runAnalysis();
}

void MainWindow::setAnalysisRunning(bool running) {
    standardSelector_->setEnabled(!running);
    openProjectAction_->setEnabled(!running);
    analyzeAction_->setEnabled(!running && !projectPath_.isEmpty());
    exportHtmlAction_->setEnabled(!running && lastResult_.has_value());
    exportSarifAction_->setEnabled(!running && lastResult_.has_value());
    exportStatisticsAction_->setEnabled(!running && lastResult_.has_value());
    if (running) statusBar()->showMessage(tr("正在分析 %1…").arg(projectPath_));
}

void MainWindow::runAnalysis() {
    if (projectPath_.isEmpty() || analysisWatcher_.isRunning()) return;
    lastResult_.reset();
    problemsTable_->setRowCount(0);
    statisticsTable_->setRowCount(0);
    violationDetails_->clear();
    sourceEditor_->clear();
    sourceTitle_->setText(tr("源码预览 · 正在分析"));
    for (int i = 0; i < 8; ++i) {
        metricValues_[i]->setText(QStringLiteral("—"));
        metricNotes_[i]->setText(tr("正在分析"));
    }
    setAnalysisRunning(true);
    const auto standardId = standardSelector_->currentData().toString();
    analysisWatcher_.setFuture(QtConcurrent::run([path = projectPath_, files = sourcePaths_, standardId] {
        return analyzeProject(path, files, standardId);
    }));
}

void MainWindow::analysisFinished() {
    lastResult_ = analysisWatcher_.result();
    populateResult();
    setAnalysisRunning(false);
    if (!lastResult_->errors.empty()) {
        QMessageBox::warning(this, tr("分析完成，但存在错误"),
                             QString::fromStdString(lastResult_->errors.front()));
    }
}

void MainWindow::populateResult() {
    statisticsTable_->setSortingEnabled(false);
    statisticsTable_->setRowCount(0);
    const auto appendStatistics = [&](const QString& label, const QString& language,
                                     const analysis::LineCounts& lines, const QString& path = {}) {
        const int row = statisticsTable_->rowCount();
        statisticsTable_->insertRow(row);
        auto* name = new QTableWidgetItem(label);
        name->setData(FilePathRole, path);
        name->setToolTip(path.isEmpty() ? label : path);
        statisticsTable_->setItem(row, 0, name);
        statisticsTable_->setItem(row, 1, new QTableWidgetItem(language));
        const std::array counts{lines.total, lines.code, lines.comment, lines.blank};
        for (int i = 0; i < 4; ++i) {
            auto* value = new QTableWidgetItem;
            value->setData(Qt::DisplayRole, static_cast<qulonglong>(counts[i]));
            statisticsTable_->setItem(row, i + 2, value);
        }
    };
    const auto& statistics = lastResult_->lineStatistics;
    std::size_t instances = 0, warnings = 0, infos = 0;
    for (const auto& unit : lastResult_->structure) instances += unit.instances.size();
    for (const auto& issue : lastResult_->violations) {
        if (issue.severity == analysis::Severity::Info) ++infos;
        else ++warnings;
    }
    const std::array counts{statistics.files.size(), instances, warnings, infos,
        statistics.total.total, statistics.total.code, statistics.total.comment, statistics.total.blank};
    for (int i = 0; i < 8; ++i) metricValues_[i]->setText(QLocale().toString(static_cast<qulonglong>(counts[i])));
    metricNotes_[0]->setText(tr("%1 个设计单元").arg(lastResult_->structure.size()));
    metricNotes_[1]->setText(tr("设计单元间实例关系"));
    metricNotes_[2]->setText(tr("需要优先检查"));
    metricNotes_[3]->setText(tr("编码规范与设计建议"));
    metricNotes_[4]->setText(tr("%1 个 HDL 文件").arg(statistics.files.size()));
    for (int i = 5; i < 8; ++i) {
        const double percent = statistics.total.total ? 100.0 * counts[i] / statistics.total.total : 0.0;
        metricNotes_[i]->setText(tr("占总行数 %1%").arg(percent, 0, 'f', 1));
    }
    sourceTitle_->setText(tr("源码预览"));
    appendStatistics(tr("全部 HDL"), tr("%1 个文件").arg(statistics.files.size()), statistics.total);
    for (const auto& language : statistics.languages) {
        const auto label = analysis::lineStatisticsLanguageName(language.language);
        appendStatistics(QString::fromUtf8(label.data(), static_cast<int>(label.size())),
                         tr("%1 个文件").arg(language.files), language.lines);
    }
    for (const auto& file : statistics.files) {
        const auto language = analysis::lineStatisticsLanguageName(file.language);
        appendStatistics(QDir(projectPath_).relativeFilePath(displayPath(file.file)),
                         QString::fromUtf8(language.data(), static_cast<int>(language.size())),
                         file.lines, displayPath(file.file));
    }
    statisticsTable_->setSortingEnabled(true);
    projectTree_->clear();
    auto* root = new QTreeWidgetItem(projectTree_, {QFileInfo(projectPath_).fileName(), tr("工程"), projectPath_});
    root->setExpanded(true);
    for (const auto& unit : lastResult_->structure) {
        const auto file = displayPath(unit.location.file);
        auto* item = new QTreeWidgetItem(root, {
            QString::fromStdString(unit.name),
            designUnitKindText(unit.kind),
            tr("%1:%2").arg(file).arg(unit.location.line)});
        item->setData(0, FilePathRole, file);
        item->setData(0, LineRole, static_cast<qulonglong>(unit.location.line));
        if (unit.topLevel) {
            auto font = item->font(0);
            font.setBold(true);
            item->setFont(0, font);
            item->setToolTip(0, tr("顶层设计单元"));
        }
        for (const auto& parameter : unit.parameters) {
            const auto parameterFile = displayPath(parameter.location.file);
            auto* child = new QTreeWidgetItem(item, {
                tr("参数：%1").arg(QString::fromStdString(parameter.name)),
                parameter.hasDefault ? tr("有默认值") : tr("必需参数"),
                tr("%1:%2").arg(parameterFile).arg(parameter.location.line)});
            child->setData(0, FilePathRole, parameterFile);
            child->setData(0, LineRole, static_cast<qulonglong>(parameter.location.line));
        }
        for (const auto& port : unit.ports) {
            const auto portFile = displayPath(port.location.file);
            auto portDetails = portDirectionText(port.direction);
            if (port.hasDefault) portDetails += tr("；有默认值");
            auto* child = new QTreeWidgetItem(item, {
                tr("端口：%1").arg(QString::fromStdString(port.name)),
                portDetails,
                tr("%1:%2").arg(portFile).arg(port.location.line)});
            child->setData(0, FilePathRole, portFile);
            child->setData(0, LineRole, static_cast<qulonglong>(port.location.line));
        }
        for (const auto& clock : unit.clocks) {
            const auto clockFile = displayPath(clock.location.file);
            auto* child = new QTreeWidgetItem(item, {
                tr("时钟：%1").arg(QString::fromStdString(clock.signal)),
                clock.risingEdge ? tr("上升沿") : tr("下降沿"),
                tr("%1:%2").arg(clockFile).arg(clock.location.line)});
            child->setData(0, FilePathRole, clockFile);
            child->setData(0, LineRole, static_cast<qulonglong>(clock.location.line));
        }
        for (const auto& reset : unit.resets) {
            const auto resetFile = displayPath(reset.location.file);
            auto* child = new QTreeWidgetItem(item, {
                tr("复位：%1").arg(QString::fromStdString(reset.signal)),
                tr("%1, %2").arg(
                    reset.synchronousObserved && reset.asynchronousObserved ? tr("同步/异步混用") :
                        reset.asynchronous ? tr("异步") : tr("同步"),
                    reset.activeHighObserved && reset.activeLowObserved ? tr("有效极性混用") :
                        reset.activeHigh ? tr("高有效") : tr("低有效")),
                tr("%1:%2").arg(resetFile).arg(reset.location.line)});
            child->setData(0, FilePathRole, resetFile);
            child->setData(0, LineRole, static_cast<qulonglong>(reset.location.line));
        }
        for (const auto& process : unit.sequentialProcesses) {
            const auto processFile = displayPath(process.location.file);
            auto* child = new QTreeWidgetItem(item, {
                tr("时序过程：%1").arg(QString::fromStdString(process.clock)),
                tr("%1；读取 %2，写入 %3，复位 %4，时钟事件 %5")
                    .arg(process.risingEdge ? tr("上升沿") : tr("下降沿"))
                    .arg(static_cast<qulonglong>(process.reads.size()))
                    .arg(static_cast<qulonglong>(process.writes.size()))
                    .arg(static_cast<qulonglong>(process.resets.size()))
                    .arg(static_cast<qulonglong>(process.clockEvents.size())),
                tr("%1:%2").arg(processFile).arg(process.location.line)});
            child->setData(0, FilePathRole, processFile);
            child->setData(0, LineRole, static_cast<qulonglong>(process.location.line));
        }
        for (const auto& synchronizer : unit.cdcSynchronizers) {
            const auto synchronizerFile = displayPath(synchronizer.location.file);
            auto* child = new QTreeWidgetItem(item, {
                tr("CDC 同步器：%1").arg(
                    QString::fromStdString(synchronizer.sourceSignal)),
                tr("%1 → %2; %3 %4")
                    .arg(QString::fromStdString(synchronizer.firstStage),
                         QString::fromStdString(synchronizer.secondStage),
                         QString::fromStdString(synchronizer.destinationClock),
                         synchronizer.risingEdge ? tr("上升沿") : tr("下降沿")),
                tr("%1:%2").arg(synchronizerFile).arg(synchronizer.location.line)});
            child->setData(0, FilePathRole, synchronizerFile);
            child->setData(0, LineRole,
                           static_cast<qulonglong>(synchronizer.location.line));
        }
        for (const auto& instance : unit.instances) {
            const auto instanceFile = displayPath(instance.location.file);
            const auto state = instance.external ? tr("外部单元") :
                               instance.resolved ? tr("已解析") : tr("未解析");
            const auto style = instance.hasNamedConnections && instance.hasPositionalConnections ?
                                   tr("混合") : instance.hasNamedConnections ? tr("命名连接") :
                                   instance.hasPositionalConnections ? tr("位置连接") : tr("无连接");
            auto connectionDetails = instance.hasWildcardConnection ?
                tr("%1；通配连接").arg(style) : style;
            if (instance.invalidPortAssociationOrder) connectionDetails += tr("；顺序无效");
            auto parameterStyle =
                instance.hasNamedParameterOverrides && instance.hasPositionalParameterOverrides ?
                    tr("混合") : instance.hasNamedParameterOverrides ? tr("命名覆盖") :
                    instance.hasPositionalParameterOverrides ? tr("位置覆盖") : tr("无覆盖");
            if (instance.invalidParameterAssociationOrder) parameterStyle += tr("；顺序无效");
            auto* child = new QTreeWidgetItem(item, {
                tr("%1 → %2").arg(QString::fromStdString(instance.name),
                                  QString::fromStdString(instance.target)),
                tr("%1；参数覆盖 %2（%3）；端口连接 %4（%5）")
                    .arg(state)
                    .arg(static_cast<qulonglong>(instance.parameterOverrideCount))
                    .arg(parameterStyle)
                    .arg(static_cast<qulonglong>(instance.portConnectionCount))
                    .arg(connectionDetails),
                tr("%1:%2").arg(instanceFile).arg(instance.location.line)});
            child->setData(0, FilePathRole, instanceFile);
            child->setData(0, LineRole, static_cast<qulonglong>(instance.location.line));
        }
    }

    problemsTable_->setSortingEnabled(false);
    problemsTable_->setRowCount(static_cast<int>(lastResult_->violations.size()));
    std::size_t criticalCount = 0;
    std::size_t errorCount = 0;
    std::size_t warningCount = 0;
    std::size_t infoCount = 0;
    for (std::size_t index = 0; index < lastResult_->violations.size(); ++index) {
        const auto& violation = lastResult_->violations[index];
        switch (violation.severity) {
        case analysis::Severity::Critical: ++criticalCount; break;
        case analysis::Severity::Error: ++errorCount; break;
        case analysis::Severity::Warning: ++warningCount; break;
        case analysis::Severity::Info: ++infoCount; break;
        }
        const int row = static_cast<int>(index);
        auto* severity = new QTableWidgetItem(severityText(violation.severity));
        severity->setForeground(QBrush(severityColor(violation.severity)));
        severity->setData(ViolationIndexRole, static_cast<qulonglong>(index));
        problemsTable_->setItem(row, 0, severity);
        problemsTable_->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(
            analysis::standardRuleDisplayId(violation.ruleId))));
        problemsTable_->setItem(row, 2, new QTableWidgetItem(chineseRuleSummary(violation.ruleId)));
        QString context;
        if (violation.module) context = QString::fromStdString(*violation.module);
        if (violation.signal) {
            if (!context.isEmpty()) context += " / ";
            context += QString::fromStdString(*violation.signal);
        }
        problemsTable_->setItem(row, 3, new QTableWidgetItem(context));
        const auto path = displayPath(violation.file);
        problemsTable_->setItem(row, 4, new QTableWidgetItem(QDir(projectPath_).relativeFilePath(path)));
        auto* line = new QTableWidgetItem;
        line->setData(Qt::DisplayRole, static_cast<qulonglong>(violation.line));
        problemsTable_->setItem(row, 5, line);
        for (int column = 0; column < 6; ++column) {
            auto* cell = problemsTable_->item(row, column);
            cell->setToolTip(column == 4 ? path : cell->text());
        }
    }
    problemsTable_->setSortingEnabled(true);
    QString standardStatus;
    if (!lastResult_->standardId.empty()) {
        std::size_t passed = 0, violated = 0, project = 0, manual = 0;
        for (const auto& assessment : lastResult_->standardAssessments) {
            switch (assessment.state) {
            case analysis::StandardRuleState::Passed: ++passed; break;
            case analysis::StandardRuleState::Violated: ++violated; break;
            case analysis::StandardRuleState::ProjectContextRequired: ++project; break;
            case analysis::StandardRuleState::ManualReviewRequired: ++manual; break;
            }
        }
        standardStatus = tr("；规则评估 %1 条（未检出 %2、违反 %3、工程复核 %4、人工复核 %5；未检出不代表合规）")
            .arg(lastResult_->standardAssessments.size()).arg(passed).arg(violated)
            .arg(project).arg(manual);
    }
    statusBar()->showMessage(tr("已分析 %1 个文件 — %2 个设计单元，%3 个问题（严重：%4 错误：%5 警告：%6 提示：%7），已豁免 %8，基线命中 %9%10")
        .arg(lastResult_->filesAnalyzed).arg(lastResult_->designUnits)
        .arg(lastResult_->violations.size()).arg(criticalCount).arg(errorCount)
        .arg(warningCount).arg(infoCount).arg(lastResult_->suppressedViolations)
        .arg(lastResult_->baselinedViolations).arg(standardStatus));
}

void MainWindow::showViolation(int row) {
    if (!lastResult_ || row < 0) return;
    const auto* item = problemsTable_->item(row, 0);
    if (!item) return;
    const auto index = item->data(ViolationIndexRole).toULongLong();
    if (index >= lastResult_->violations.size()) return;
    const auto& violation = lastResult_->violations[index];
    violationDetails_->setPlainText(
        tr("%1［%2］\n\n问题说明\n%3\n\n修改建议\n%4\n\n位置\n%5:%6:%7")
            .arg(QString::fromStdString(analysis::standardRuleDisplayId(violation.ruleId)))
            .arg(severityText(violation.severity))
            .arg(chineseRuleSummary(violation.ruleId))
            .arg(tr("请根据规则要求检查并修正相关 HDL 代码；如果该结构属于明确的设计意图，可在评审后添加局部规则豁免。"))
            .arg(displayPath(violation.file))
            .arg(violation.line).arg(violation.column));
    loadSourceLocation(displayPath(violation.file), static_cast<int>(violation.line));
}

void MainWindow::openTreeLocation(QTreeWidgetItem* item) {
    if (!item) return;
    const auto path = item->data(0, FilePathRole).toString();
    if (!path.isEmpty()) loadSourceLocation(path, item->data(0, LineRole).toInt());
}

void MainWindow::loadSourceLocation(const QString& path, int line) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        statusBar()->showMessage(tr("无法打开源文件：%1").arg(path), 5000);
        return;
    }
    sourceEditor_->setPlainText(QString::fromUtf8(file.readAll()));
    pages_->setCurrentIndex(0);
    sourceTitle_->setText(tr("%1 · 第 %2 行").arg(QFileInfo(path).fileName()).arg(line));
    sourceTitle_->setToolTip(path);
    const auto block = sourceEditor_->document()->findBlockByLineNumber(std::max(0, line - 1));
    if (block.isValid()) {
        QTextCursor cursor(block);
        cursor.select(QTextCursor::LineUnderCursor);
        sourceEditor_->setTextCursor(cursor);
        sourceEditor_->centerCursor();
        sourceEditor_->setFocus();
    }
}

void MainWindow::exportReport(const QString& format) {
    if (!lastResult_) return;
    const auto extension = format == "html" ? "html" : format == "csv" ? "csv" : "sarif";
    const auto filter = format == "html" ? tr("HTML 报告 (*.html)") :
                        format == "csv" ? tr("行数统计 CSV (*.csv)") : tr("SARIF 报告 (*.sarif)");
    const auto suggested = QDir(projectPath_).filePath(QString("fpga-lint-report.") + extension);
    const auto target = QFileDialog::getSaveFileName(this, tr("导出分析报告"), suggested, filter);
    if (target.isEmpty()) return;
    std::ostringstream content;
    if (format == "html") report::writeHtml(*lastResult_, content);
    else if (format == "csv") report::writeLineStatisticsCsv(*lastResult_, content);
    else report::writeSarif(*lastResult_, content);
    const auto bytes = content.str();
    QSaveFile file(target);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(bytes.data(), static_cast<qint64>(bytes.size())) != static_cast<qint64>(bytes.size()) ||
        !file.commit()) {
        QMessageBox::critical(this, tr("导出失败"), tr("无法写入报告：%1").arg(target));
        return;
    }
    statusBar()->showMessage(tr("报告已导出：%1").arg(target), 5000);
}

} // namespace fpga::gui

#include "ui/main_window.hpp"

#include "ui/code_highlighter.hpp"

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>
#include <QStringConverter>
#include <QTextStream>
#include <QToolBar>
#include <QTreeView>

namespace {

QString read_text_file(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    return stream.readAll();
}

bool write_text_file(const QString& path, const QString& content) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }

    QTextStream stream(&file);
    stream.setEncoding(QStringConverter::Utf8);
    stream << content;
    return true;
}

QFont editor_font() {
    QFont font("Cascadia Mono");
    if (!font.exactMatch()) {
        font = QFont("Consolas");
    }
    font.setPointSize(11);
    return font;
}

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setup_ui();
    apply_theme();

    open_root_path_ = QDir::currentPath();
    file_model_->setRootPath(open_root_path_);
    file_tree_->setRootIndex(file_model_->index(open_root_path_));
    path_label_->setText(open_root_path_);

    create_new_tab("untitled.cpp", default_snippet("cpp"), QString(), "cpp", true);
    resize(1480, 920);
    update_window_title();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    event->accept();
}

void MainWindow::setup_ui() {
    setup_toolbar();
    setup_layout();
    setup_terminal();
    setup_connections();
}

void MainWindow::setup_toolbar() {
    auto* toolbar = addToolBar("Main");
    toolbar->setMovable(false);

    auto* openFolderButton = new QPushButton("Open Folder", this);
    add_tab_button_ = new QPushButton("New Tab", this);
    close_tab_button_ = new QPushButton("Close Tab", this);
    auto* saveButton = new QPushButton("Save", this);
    auto* runButton = new QPushButton("Run", this);

    language_combo_ = new QComboBox(this);
    for (const auto& language : core_.languages()) {
        language_combo_->addItem(QString::fromStdString(language.display_name), QString::fromStdString(language.id));
    }
    language_combo_->setCurrentIndex(language_combo_->findData("cpp"));

    path_label_ = new QLineEdit(this);
    path_label_->setReadOnly(true);
    path_label_->setPlaceholderText("Workspace folder");

    toolbar->addWidget(openFolderButton);
    toolbar->addWidget(add_tab_button_);
    toolbar->addWidget(close_tab_button_);
    toolbar->addWidget(saveButton);
    toolbar->addSeparator();
    toolbar->addWidget(new QLabel("Language", this));
    toolbar->addWidget(language_combo_);
    toolbar->addWidget(runButton);
    toolbar->addSeparator();
    toolbar->addWidget(new QLabel("Workspace", this));
    toolbar->addWidget(path_label_);

    connect(openFolderButton, &QPushButton::clicked, this, [this] { open_folder(); });
    connect(add_tab_button_, &QPushButton::clicked, this, [this] { add_tab(); });
    connect(close_tab_button_, &QPushButton::clicked, this, [this] { remove_current_tab(); });
    connect(saveButton, &QPushButton::clicked, this, [this] { save_current_tab(); });
    connect(runButton, &QPushButton::clicked, this, [this] { run_current_tab(); });
}

void MainWindow::setup_layout() {
    auto* splitter = new QSplitter(this);
    splitter->setChildrenCollapsible(false);

    file_model_ = new QFileSystemModel(this);
    file_model_->setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Files);

    file_tree_ = new QTreeView(splitter);
    file_tree_->setModel(file_model_);
    file_tree_->setMinimumWidth(300);
    file_tree_->setAlternatingRowColors(true);
    file_tree_->header()->setStretchLastSection(true);
    file_tree_->setAnimated(true);

    editor_tabs_ = new QTabWidget(splitter);
    editor_tabs_->setTabsClosable(true);
    editor_tabs_->setDocumentMode(true);
    editor_tabs_->setMovable(true);
    editor_tabs_->setUsesScrollButtons(true);

    splitter->addWidget(file_tree_);
    splitter->addWidget(editor_tabs_);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);

    status_label_ = new QLabel("Ready.", this);
    syntax_label_ = new QLabel("Syntax: basic check OK", this);
    statusBar()->addWidget(status_label_, 1);
    statusBar()->addPermanentWidget(syntax_label_);
}

void MainWindow::setup_terminal() {
    auto* dock = new QDockWidget("Terminal", this);
    dock->setAllowedAreas(Qt::BottomDockWidgetArea);

    terminal_output_ = new QPlainTextEdit(dock);
    terminal_output_->setReadOnly(true);
    terminal_output_->setFont(editor_font());
    dock->setWidget(terminal_output_);

    addDockWidget(Qt::BottomDockWidgetArea, dock);
}

void MainWindow::setup_connections() {
    connect(file_tree_, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) { open_file_from_index(index); });
    connect(editor_tabs_, &QTabWidget::tabCloseRequested, this, [this](int index) { close_tab(index); });
    connect(editor_tabs_, &QTabWidget::currentChanged, this, [this](int index) {
        sync_language_for_tab(index);
        update_window_title();
        update_syntax_status();
    });
    connect(language_combo_, &QComboBox::currentIndexChanged, this, [this](int) {
        if (auto* state = current_state(); state != nullptr) {
            state->language = language_combo_->currentData().toString();
            if (state->highlighter != nullptr) {
                state->highlighter->setLanguage(state->language);
            }
            update_syntax_status();
        }
    });
}
void MainWindow::apply_theme() {
    qApp->setStyleSheet(
        "QMainWindow { background: #0b1220; color: #e5e7eb; }"
        "QToolBar { spacing: 8px; padding: 8px; border: 0; background: #111827; color: #d1d5db; }"
        "QStatusBar { background: #0f172a; color: #94a3b8; border-top: 1px solid #1f2937; }"
        "QTreeView, QPlainTextEdit, QTabWidget::pane, QLineEdit, QComboBox {"
        "  background: #111827; color: #e5e7eb; border: 1px solid #243244; border-radius: 10px; selection-background-color: #2563eb; selection-color: #f8fafc; }"
        "QTreeView { alternate-background-color: #0f172a; }"
        "QHeaderView::section { background: #0f172a; color: #94a3b8; border: 0; padding: 6px 8px; }"
        "QPushButton { background: #2563eb; color: #eff6ff; border: 1px solid #3b82f6; padding: 8px 14px; border-radius: 10px; font-weight: 600; }"
        "QPushButton:hover { background: #1d4ed8; border-color: #60a5fa; }"
        "QPushButton:pressed { background: #1e40af; }"
        "QLabel { color: #cbd5e1; }"
        "QLineEdit[readOnly=\"true\"] { background: #0f172a; color: #93c5fd; }"
        "QComboBox::drop-down { border: 0; width: 24px; }"
        "QTabBar::tab { background: #0f172a; color: #94a3b8; padding: 9px 14px; margin-right: 4px; border: 1px solid #1f2937; border-bottom: 0; border-top-left-radius: 8px; border-top-right-radius: 8px; }"
        "QTabBar::tab:selected { background: #111827; color: #f8fafc; border-color: #334155; }"
        "QTabBar::tab:hover:!selected { background: #172033; color: #dbeafe; }"
        "QDockWidget { color: #d1d5db; }"
        "QDockWidget::title { padding: 8px 10px; background: #0f172a; color: #cbd5e1; border-bottom: 1px solid #1f2937; }"
        "QSplitter::handle { background: #0f172a; }"
        "QScrollBar:vertical, QScrollBar:horizontal { background: #0f172a; border: 0; margin: 0; }"
        "QScrollBar::handle:vertical, QScrollBar::handle:horizontal { background: #334155; border-radius: 6px; min-height: 24px; min-width: 24px; }"
        "QScrollBar::add-line, QScrollBar::sub-line, QScrollBar::add-page, QScrollBar::sub-page { background: none; border: 0; }"
    );
}

void MainWindow::open_folder() {
    const auto folder = QFileDialog::getExistingDirectory(this, "Open Folder", open_root_path_.isEmpty() ? QDir::currentPath() : open_root_path_);
    if (folder.isEmpty()) {
        return;
    }

    open_root_path_ = folder;
    file_model_->setRootPath(folder);
    file_tree_->setRootIndex(file_model_->index(folder));
    path_label_->setText(folder);
    status_label_->setText("Workspace opened.");
}

void MainWindow::open_file(const QString& file_path) {
    if (file_path.isEmpty()) {
        return;
    }

    for (int index = 0; index < editor_tabs_->count(); ++index) {
        if (auto* state = state_for_index(index); state != nullptr && state->file_path == file_path) {
            editor_tabs_->setCurrentIndex(index);
            return;
        }
    }

    const QString content = read_text_file(file_path);
    create_new_tab(QFileInfo(file_path).fileName(), content, file_path, infer_language(file_path), false);
    status_label_->setText("File opened.");
}

void MainWindow::open_file_from_index(const QModelIndex& index) {
    if (!index.isValid()) {
        return;
    }
    if (file_model_->isDir(index)) {
        file_tree_->setExpanded(index, !file_tree_->isExpanded(index));
        return;
    }
    open_file(file_model_->filePath(index));
}

void MainWindow::add_tab() {
    const auto language = language_combo_->currentData().toString();
    const auto suffix = language.isEmpty() ? QStringLiteral("txt") : language;
    create_new_tab("untitled." + suffix, default_snippet(language), QString(), language, true);
    status_label_->setText("New tab created.");
}

void MainWindow::create_new_tab(const QString& title, const QString& content, const QString& file_path, const QString& language, bool untitled) {
    auto* editor = new QPlainTextEdit(this);
    editor->setPlainText(content);
    editor->setTabStopDistance(4 * fontMetrics().horizontalAdvance(' '));
    editor->setFont(editor_font());

    auto* highlighter = new CodeHighlighter(editor->document());
    highlighter->setLanguage(language);

    const int index = editor_tabs_->addTab(editor, title);
    editor_tabs_->setCurrentIndex(index);
    editors_[editor] = EditorState{editor, highlighter, file_path, language, untitled};

    connect(editor, &QPlainTextEdit::textChanged, this, [this, editor] {
        if (editor == current_editor()) {
            update_syntax_status();
        }
    });

    sync_language_for_tab(index);
    update_window_title();
    update_syntax_status();
}

void MainWindow::close_tab(int index) {
    if (index < 0) {
        return;
    }

    auto* editor = qobject_cast<QPlainTextEdit*>(editor_tabs_->widget(index));
    if (editor != nullptr) {
        editors_.erase(editor);
    }

    auto* widget = editor_tabs_->widget(index);
    editor_tabs_->removeTab(index);
    delete widget;

    if (editor_tabs_->count() == 0) {
        add_tab();
    } else {
        update_syntax_status();
    }
}

void MainWindow::remove_current_tab() {
    close_tab(editor_tabs_->currentIndex());
}

void MainWindow::save_current_tab() {
    save_tab(editor_tabs_->currentIndex(), false);
}

void MainWindow::save_tab(int index, bool save_as) {
    auto* state = state_for_index(index);
    if (state == nullptr) {
        return;
    }

    QString target_path = state->file_path;
    if (save_as || target_path.isEmpty()) {
        const auto suggested_name = state->untitled ? editor_tabs_->tabText(index) : QFileInfo(target_path).fileName();
        target_path = QFileDialog::getSaveFileName(this, "Save File", open_root_path_ + QDir::separator() + suggested_name);
        if (target_path.isEmpty()) {
            return;
        }
    }

    const auto* editor = qobject_cast<QPlainTextEdit*>(editor_tabs_->widget(index));
    if (editor == nullptr) {
        return;
    }

    if (!write_text_file(target_path, editor->toPlainText())) {
        QMessageBox::warning(this, "Save Failed", "Unable to save the file.");
        return;
    }

    state->file_path = target_path;
    state->language = infer_language(target_path);
    state->untitled = false;
    if (state->highlighter != nullptr) {
        state->highlighter->setLanguage(state->language);
    }
    editor_tabs_->setTabText(index, QFileInfo(target_path).fileName());
    sync_language_for_tab(index);
    update_window_title();
    update_syntax_status();
    status_label_->setText("Saved.");
}

void MainWindow::run_current_tab() {
    auto* state = current_state();
    auto* editor = current_editor();
    if (state == nullptr || editor == nullptr) {
        return;
    }

    if (state->language.isEmpty()) {
        state->language = language_combo_->currentData().toString();
    }

    terminal_output_->clear();
    status_label_->setText("Running...");

    noc::core::ExecutionRequest request;
    request.language = state->language.toStdString();
    request.code = editor->toPlainText().toStdString();
    request.session_id = "desktop_run";

    const auto result = core_.run(std::move(request));
    const QString output = QString::fromStdString(result.output.empty() ? result.error : result.output);
    append_terminal(output.isEmpty() ? QStringLiteral("(no output)") : output);
    status_label_->setText(result.success ? "Completed." : "Failed.");
}

void MainWindow::sync_language_for_tab(int index) {
    const auto* state = state_for_index(index);
    if (state == nullptr) {
        return;
    }

    const int combo_index = language_combo_->findData(state->language);
    if (combo_index >= 0) {
        language_combo_->setCurrentIndex(combo_index);
    }
}

void MainWindow::update_window_title() {
    QString title = QStringLiteral("NOC");
    if (editor_tabs_->currentIndex() >= 0) {
        title += QStringLiteral(" - ") + editor_tabs_->tabText(editor_tabs_->currentIndex());
    }
    setWindowTitle(title);
}

void MainWindow::update_syntax_status() {
    if (syntax_label_ == nullptr) {
        return;
    }

    const auto* editor = current_editor();
    if (editor == nullptr) {
        syntax_label_->setText("Syntax: n/a");
        return;
    }

    const QString text = editor->toPlainText();
    int round = 0;
    int curly = 0;
    int square = 0;
    bool inSingle = false;
    bool inDouble = false;
    bool escaped = false;

    for (const QChar ch : text) {
        if (escaped) {
            escaped = false;
            continue;
        }
        if ((inSingle || inDouble) && ch == QChar('\\')) {
            escaped = true;
            continue;
        }
        if (!inDouble && ch == QChar('\'')) {
            inSingle = !inSingle;
            continue;
        }
        if (!inSingle && ch == QChar('"')) {
            inDouble = !inDouble;
            continue;
        }
        if (inSingle || inDouble) {
            continue;
        }

        if (ch == QChar('(')) ++round;
        else if (ch == QChar(')')) --round;
        else if (ch == QChar('{')) ++curly;
        else if (ch == QChar('}')) --curly;
        else if (ch == QChar('[')) ++square;
        else if (ch == QChar(']')) --square;

        if (round < 0 || curly < 0 || square < 0) {
            syntax_label_->setText("Syntax: unmatched closing token");
            return;
        }
    }

    if (inSingle || inDouble) {
        syntax_label_->setText("Syntax: unclosed string");
        return;
    }
    if (round != 0 || curly != 0 || square != 0) {
        syntax_label_->setText("Syntax: unbalanced delimiters");
        return;
    }

    syntax_label_->setText("Syntax: basic check OK");
}

void MainWindow::append_terminal(const QString& text) {
    terminal_output_->setPlainText(text);
}

MainWindow::EditorState* MainWindow::state_for_index(int index) {
    if (index < 0) {
        return nullptr;
    }
    auto* editor = qobject_cast<QPlainTextEdit*>(editor_tabs_->widget(index));
    if (editor == nullptr) {
        return nullptr;
    }
    const auto it = editors_.find(editor);
    if (it == editors_.end()) {
        return nullptr;
    }
    return &it->second;
}

MainWindow::EditorState* MainWindow::current_state() {
    return state_for_index(editor_tabs_->currentIndex());
}

const MainWindow::EditorState* MainWindow::current_state() const {
    if (editor_tabs_ == nullptr) {
        return nullptr;
    }
    const int index = editor_tabs_->currentIndex();
    if (index < 0) {
        return nullptr;
    }
    auto* editor = qobject_cast<QPlainTextEdit*>(editor_tabs_->widget(index));
    if (editor == nullptr) {
        return nullptr;
    }
    const auto it = editors_.find(editor);
    if (it == editors_.end()) {
        return nullptr;
    }
    return &it->second;
}

QPlainTextEdit* MainWindow::current_editor() const {
    return editor_tabs_ == nullptr ? nullptr : qobject_cast<QPlainTextEdit*>(editor_tabs_->currentWidget());
}

QString MainWindow::infer_language(const QString& file_path) const {
    const auto suffix = QFileInfo(file_path).suffix().toLower();
    if (suffix == "py") return "python";
    if (suffix == "c") return "c";
    if (suffix == "cpp" || suffix == "cc" || suffix == "cxx" || suffix == "hpp" || suffix == "hxx") return "cpp";
    if (suffix == "cs") return "csharp";
    if (suffix == "java") return "java";
    if (suffix == "js") return "javascript";
    if (suffix == "html" || suffix == "htm") return "html";
    if (suffix == "rs") return "rust";
    if (suffix == "lua") return "lua";
    if (suffix == "zig") return "zig";
    if (suffix == "scala") return "scala";
    if (suffix == "rb") return "ruby";
    if (suffix == "go") return "go";
    if (suffix == "php") return "php";
    return language_combo_ != nullptr ? language_combo_->currentData().toString() : QStringLiteral("cpp");
}

QString MainWindow::default_snippet(const QString& language) const {
    if (language == "python") return "print('Hello from NOC')\n";
    if (language == "c") return "#include <stdio.h>\n\nint main(void) {\n    puts(\"Hello from C\");\n    return 0;\n}\n";
    if (language == "cpp") return "#include <iostream>\n\nint main() {\n    std::cout << \"Hello from C++\\n\";\n    return 0;\n}\n";
    if (language == "csharp") return "using System;\n\nclass Program {\n    static void Main() {\n        Console.WriteLine(\"Hello from C#\");\n    }\n}\n";
    if (language == "java") return "public class Main {\n    public static void main(String[] args) {\n        System.out.println(\"Hello from Java\");\n    }\n}\n";
    if (language == "javascript") return "console.log('Hello from NOC');\n";
    if (language == "html") return "<!DOCTYPE html>\n<html>\n<body>\n  <h1>Hello from NOC</h1>\n</body>\n</html>\n";
    if (language == "rust") return "fn main() {\n    println!(\"Hello from Rust\");\n}\n";
    if (language == "lua") return "print('Hello from Lua')\n";
    if (language == "zig") return "const std = @import(\"std\");\n\npub fn main() !void {\n    try std.io.getStdOut().writer().print(\"Hello from Zig\\n\", .{});\n}\n";
    if (language == "scala") return "object Main extends App {\n    println(\"Hello from Scala\")\n}\n";
    if (language == "ruby") return "puts 'Hello from Ruby'\n";
    if (language == "go") return "package main\n\nimport \"fmt\"\n\nfunc main() {\n    fmt.Println(\"Hello from Go\")\n}\n";
    if (language == "php") return "<?php\necho 'Hello from PHP';\n";
    return {};
}

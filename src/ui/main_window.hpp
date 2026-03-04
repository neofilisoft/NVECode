#pragma once

#include <unordered_map>

#include <QCloseEvent>
#include <QMainWindow>
#include <QString>

#include "core/ide_core.hpp"

class CodeHighlighter;
class QFileSystemModel;
class QModelIndex;
class QPlainTextEdit;
class QPushButton;
class QTabWidget;
class QTreeView;
class QComboBox;
class QLabel;
class QLineEdit;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    struct EditorState {
        QPlainTextEdit* editor = nullptr;
        CodeHighlighter* highlighter = nullptr;
        QString file_path;
        QString language;
        bool untitled = false;
    };

    void setup_ui();
    void setup_toolbar();
    void setup_layout();
    void setup_terminal();
    void setup_connections();
    void apply_theme();

    void open_folder();
    void open_file(const QString& file_path);
    void open_file_from_index(const QModelIndex& index);
    void add_tab();
    void create_new_tab(const QString& title, const QString& content, const QString& file_path, const QString& language, bool untitled);
    void close_tab(int index);
    void remove_current_tab();
    void save_current_tab();
    void save_tab(int index, bool save_as);
    void run_current_tab();
    void sync_language_for_tab(int index);
    void update_window_title();
    void update_syntax_status();
    void append_terminal(const QString& text);

    EditorState* state_for_index(int index);
    EditorState* current_state();
    const EditorState* current_state() const;
    QPlainTextEdit* current_editor() const;
    QString infer_language(const QString& file_path) const;
    QString default_snippet(const QString& language) const;

    noc::core::IdeCore core_;

    QTreeView* file_tree_ = nullptr;
    QFileSystemModel* file_model_ = nullptr;
    QTabWidget* editor_tabs_ = nullptr;
    QPlainTextEdit* terminal_output_ = nullptr;
    QComboBox* language_combo_ = nullptr;
    QLabel* status_label_ = nullptr;
    QLabel* syntax_label_ = nullptr;
    QLineEdit* path_label_ = nullptr;
    QPushButton* add_tab_button_ = nullptr;
    QPushButton* close_tab_button_ = nullptr;

    QString open_root_path_;
    std::unordered_map<QPlainTextEdit*, EditorState> editors_;
};

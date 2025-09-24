#ifndef MSTEDITOR_H
#define MSTEDITOR_H

#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDesktopServices>
#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QFontDatabase>
#include <QTreeWidgetItem>
#include <QMap>
#include <QMainWindow>
#include <QMessageBox>
#include <QMimeData>
#include <QScrollBar>
#include <QSettings>
#include <QShortcut>
#include <QSpinBox>
#include <QTextCodec>
#include <QTextBrowser>
#include <QValidator>
#include <QPainter>

#include "mst.h"

using namespace std;

namespace Ui {
class mstEditor;
}

// Button enum
enum Button : int {
    A = 0,
    B,
    X,
    Y,
    LB,
    LT,
    RB,
    RT,
    START,
    BACK,
    DPAD,
    LSTICK,
    RSTICK,
    COUNT
};

// Tag Type enum
enum Tag : int {
    Sound,
    Picture,
    RGBA,
    Color
};

// Drag box enum
enum DragBox : int {
    Alpha,
    Start,
    End,
    None
};

class mstEditor : public QMainWindow
{
    Q_OBJECT

public:
    explicit mstEditor(QWidget *parent = nullptr);
    ~mstEditor();

    bool eventFilter(QObject *object, QEvent *event);
    void closeEvent (QCloseEvent *event);

    // Easy file reading
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    void passArgument(std::string const& _file);

private slots:
    // Menu bar
    void on_actionOpen_triggered();
    void on_actionSave_triggered();
    void on_actionSave_as_triggered();
    void on_actionClose_triggered();
    void on_actionExport_triggered();
    void on_actionAbout_Qt_triggered();
    void on_actionAbout_mstEditor_triggered();

    // Tree view
    void on_TW_TreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_TW_TreeWidget_itemMoved(int from, int to);
    void on_PB_SubtitleAdd_clicked();
    void on_PB_SubtitleDelete_clicked();
    void on_PB_Find_clicked();
    void on_LE_Find_returnPressed();
    void on_LE_Find_textEdited(const QString &arg1);

    // Subtitle Editor
    void on_PB_TagWhat_clicked();
    void on_PB_PagePrev_clicked();
    void on_PB_PageNext_clicked();
    void on_PB_PageAdd_clicked();
    void on_PB_PageDelete_clicked();
    void on_PB_Save_clicked();
    void on_PB_Reset_clicked();
    void on_PB_ColorAdd_clicked();
    void on_TE_TextEditor_textChanged();
    void on_CB_ComboBox_currentIndexChanged(int index);
    void on_CB_Sound_stateChanged(int state);
    void on_CB_AutoApply_clicked(bool checked);
    void on_LE_Sound_textEdited(const QString &str);
    void on_LE_SubtitleName_textEdited(const QString &str);

    // Shortcuts
    void on_Shortcut_ApplyChanges();
    void on_Shortcut_ResetSubtitle();
    void on_Shortcut_Find();

    // Russian Mode
    void on_CB_Russian_clicked(bool checked);

    // Color block signals
    void ColorSpinBoxChanged(int _value);
    void ColorDeletePressed();

    // Preview
    void on_PB_SavePreview_clicked();

private:
    struct ColorBlock
    {
        ColorBlock():m_color(255,153,0),m_start(0),m_end(0){}

        // Get: start -> end
        // Insert: end -> start
        QColor m_color;
        int m_start;
        int m_end;
    };

    void ResetProgram();
    void ResetEditor();
    void OpenFile(QString const& mstFile, bool showSuccess = true);
    bool DiscardSaveMessage(QString _title, QString _message, bool _checkFileEdited);

    // Tree view
    void TW_Refresh();
    void TW_FocusItem(int _id);
    void TW_AddOrReplaceEntry(mst::TextEntry entry, int _id = -1);
    void TW_Find();

    // Subtitle Editor
    void LoadSubtitle(int _id, int _page = 0);
    void LoadPage(int _page);
    void AddButtonComboBox(int _insertAt, Button _button = Button::COUNT);
    void RemoveButtonComboBox(int _comboID);
    void RemoveAllButtonComboBox();
    void SetCurrentPageLabel();
    void UpdateSubtitlePreview();
    void SetSubtitleEdited(bool _edited);

    void GetColorTagsFromCurrentPage(bool _insertUI);
    void InsertColorTagsToCurrentPage(bool _removeUI);
    void AddColorBlock(int insertAt, ColorBlock _colorBlock = ColorBlock());
    void RemoveColorBlock(int _colorID);
    void RemoveAllColorBlocks();
    void UpdateStartEndLimits(int _colorID);

    // Russian Mode
    QString ToRussian(QString const& str);
    QString ToUnicode(QString const& str);

private:
    Ui::mstEditor *ui;
    QSettings *m_settings;

    // File
    mst m_mst;
    QString m_path;
    QString m_fileName;
    bool m_fileEdited;
    QString m_pngPath;

    // Editor
    QLabel* m_previewLabel;
    int m_id;
    int m_page;
    QString m_name;
    QVector<QString> m_subtitles;
    typedef QPair<QString, Tag> TagPair;
    QVector<TagPair> m_tags;
    QVector<ColorBlock> m_colorBlocks;
    bool m_subtitleEdited;
    bool m_subtitleHardcoded;

    // Dragging spin box
    int m_initValue;
    int m_mouseY;
    int m_dragScale;
    QSpinBox* m_dragSpinBox;

    // Button mapping
    QMap<QString, Button> m_stringToButton;
    QMap<Button, QString> m_buttonToCombo;
    QMap<Button, QString> m_buttonToString;

    // Russian mode
    QMap<QChar, QChar> m_unicodeToRussian;
    QMap<QChar, QChar> m_russianToUnicode;
};

#endif // MSTEDITOR_H

#include "msteditor.h"
#include "ui_msteditor.h"

//---------------------------------------------------------------------------
// Constructor
//---------------------------------------------------------------------------
mstEditor::mstEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::mstEditor)
{
    ui->setupUi(this);

    // Initialize mapping
    m_stringToButton["button_a"] = Button::A;
    m_stringToButton["button_b"] = Button::B;
    m_stringToButton["button_x"] = Button::X;
    m_stringToButton["button_y"] = Button::Y;
    m_stringToButton["button_lb"] = Button::LB;
    m_stringToButton["button_lt"] = Button::LT;
    m_stringToButton["button_rb"] = Button::RB;
    m_stringToButton["button_rt"] = Button::RT;
    m_stringToButton["button_start"] = Button::START;
    m_stringToButton["button_back"] = Button::BACK;

    m_buttonToCombo[Button::A] = "A Button";
    m_buttonToCombo[Button::B] = "B Button";
    m_buttonToCombo[Button::X] = "X Button";
    m_buttonToCombo[Button::Y] = "Y Button";
    m_buttonToCombo[Button::LB] = "Left Bumper";
    m_buttonToCombo[Button::LT] = "Left Trigger";
    m_buttonToCombo[Button::RB] = "Right Bumper";
    m_buttonToCombo[Button::RT] = "Right Trigger";
    m_buttonToCombo[Button::START] = "Start Button";
    m_buttonToCombo[Button::BACK] = "Back Button";

    m_buttonToString[Button::A] = "button_a";
    m_buttonToString[Button::B] = "button_b";
    m_buttonToString[Button::X] = "button_x";
    m_buttonToString[Button::Y] = "button_y";
    m_buttonToString[Button::LB] = "button_lb";
    m_buttonToString[Button::LT] = "button_lt";
    m_buttonToString[Button::RB] = "button_rb";
    m_buttonToString[Button::RT] = "button_rt";
    m_buttonToString[Button::START] = "button_start";
    m_buttonToString[Button::BACK] = "button_back";

    // Unicode to russian encoding
    QString unicode = "¨ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþÿ¸";
    QString russian = "ЁАБВГДЕЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯабвгдежзийклмнопрстуфхцчшщъыьэюяё";
    for(int i = 0; i < unicode.size(); i++)
    {
        m_unicodeToRussian[unicode[i]] = russian[i];
        m_russianToUnicode[russian[i]] = unicode[i];
    }

    // Validator for line edits
    QRegExp rx("[A-Za-z0-9_]+");
    QRegExpValidator* v = new QRegExpValidator(rx, this);
    ui->LE_SubtitleName->setValidator(v);
    ui->LE_Sound->setValidator(v);

    // Tree view
    ui->TW_TreeWidget->setColumnWidth(0, 150);
    ui->TW_TreeWidget->setColumnWidth(1, 320);

    // Create a label layout on top of the subtitle background
    QFontDatabase::addApplicationFont(":/resources/FOT-RodinCattleyaPro-DB.otf");
    QFontDatabase::addApplicationFont(":/resources/Nintendo_NTLG-DB_002.ttf");
    m_previewLabel = new QLabel(ui->L_Preview);
    m_previewLabel->move(76,31);
    m_previewLabel->setStyleSheet("font: 27px \"FOT-RodinCattleya Pro DB\"; color: white;");
    m_previewLabel->setTextFormat(Qt::TextFormat::RichText);
    QSize previewSize = QSize(792,108);
    m_previewLabel->setMinimumSize(previewSize);
    m_previewLabel->setMaximumSize(previewSize);
    m_previewLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // Load previous path and window size
    m_settings = new QSettings("brianuuu", "mstEditor", this);
    m_path = m_settings->value("DefaultDirectory", QString()).toString();
    this->resize(m_settings->value("DefaultSize", QSize(1145, 720)).toSize());

    // Bind extra shortcuts
    QShortcut *applyChanges = new QShortcut(QKeySequence("Alt+S"), this);
    connect(applyChanges, SIGNAL(activated()), this, SLOT(on_Shortcut_ApplyChanges()));
    QShortcut *resetSubtitle = new QShortcut(QKeySequence("Alt+R"), this);
    connect(resetSubtitle, SIGNAL(activated()), this, SLOT(on_Shortcut_ResetSubtitle()));
    QShortcut *find = new QShortcut(QKeySequence("Ctrl+F"), this);
    connect(find, SIGNAL(activated()), this, SLOT(on_Shortcut_Find()));

    // Restart
    ResetProgram();

    // Enable drag and drop to window
    setAcceptDrops(true);
}

//---------------------------------------------------------------------------
// Destructor
//---------------------------------------------------------------------------
mstEditor::~mstEditor()
{
    m_settings->setValue("DefaultDirectory", m_path);
    m_settings->setValue("DefaultSize", this->size());
    delete ui;
}

//---------------------------------------------------------------------------
// Handling mouse events
//---------------------------------------------------------------------------
bool mstEditor::eventFilter(QObject *object, QEvent *event)
{
    // Changing a color of a color block
    if (event->type() == QEvent::FocusIn)
    {
        QWidget* widget = qobject_cast<QWidget*>(object);
        for (int i = 0; i < ui->VL_Colors->layout()->count(); i++)
        {
            QLayoutItem* colorLayoutItem = ui->VL_Colors->layout()->itemAt(i);
            if (colorLayoutItem->layout()->itemAt(1)->widget() == widget)
            {
                widget->clearFocus();
                QPalette pal = widget->palette();
                QColor color = pal.color(QPalette::Base);
                QColor newColor = QColorDialog::getColor(color, this, "Pick a Color");
                if (newColor.isValid())
                {
                    // Update color
                    pal.setColor(QPalette::Base, newColor);
                    m_colorBlocks[i].m_color.setRgb(newColor.rgb());
                    widget->setPalette(pal);

                    // Save & Reset buttons
                    UpdateSubtitlePreview();
                    SetSubtitleEdited(true);
                }
                return true;
            }
        }

        for (int i = 0; i < ui->VL_Colors->layout()->count(); i++)
        {
            // Look for start or end
            QLayoutItem* colorLayoutItem = ui->VL_Colors->layout()->itemAt(i);
            if (colorLayoutItem->layout()->itemAt(4)->widget() == widget)
            {
                UpdateStartEndLimits(i);
                break;
            }
            else if (colorLayoutItem->layout()->itemAt(6)->widget() == widget)
            {
                UpdateStartEndLimits(i);
                break;
            }
        }
    }

    // Dragging spin boxes
    if (!m_dragSpinBox && event->type() == QEvent::MouseButtonPress)
    {
        QWidget* widget = qobject_cast<QWidget*>(object);
        m_dragScale = 0;
        int index = -1;
        for (int i = 0; i < ui->VL_Colors->layout()->count(); i++)
        {
            // Look for alpha
            QLayoutItem* colorLayoutItem = ui->VL_Colors->layout()->itemAt(i);
            if (colorLayoutItem->layout()->itemAt(2)->widget() == widget)
            {
                index = i;
                break;
            }
        }

        if (index != -1)
        {
            m_dragScale = 1;
        }
        else
        {
            index = -1;
            for (int i = 0; i < ui->VL_Colors->layout()->count(); i++)
            {
                // Look for start or end
                QLayoutItem* colorLayoutItem = ui->VL_Colors->layout()->itemAt(i);
                if (colorLayoutItem->layout()->itemAt(4)->widget() == widget)
                {
                    index = i;
                    break;
                }
                else if (colorLayoutItem->layout()->itemAt(6)->widget() == widget)
                {
                    index = i;
                    break;
                }
            }

            if (index != -1)
            {
                m_dragScale = 6;
            }
        }

        if (m_dragScale != 0)
        {
            QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
            m_mouseY = e->y();
            m_dragSpinBox = reinterpret_cast<QSpinBox*>(widget);
            m_initValue = m_dragSpinBox->value();

            UpdateStartEndLimits(index);
        }
    }
    else if (m_dragSpinBox && event->type() == QEvent::MouseButtonRelease)
    {
        m_dragSpinBox = Q_NULLPTR;
    }
    else if (m_dragSpinBox && event->type() == QEvent::MouseMove)
    {
        QMouseEvent *e = reinterpret_cast<QMouseEvent*>(event);
        int newVar = m_initValue + (m_mouseY - e->y()) / m_dragScale;

        if (newVar < m_dragSpinBox->minimum())
        {
            m_initValue = m_dragSpinBox->minimum();
            m_mouseY = e->y();
            newVar = m_initValue;
        }
        else if (newVar > m_dragSpinBox->maximum())
        {
            m_initValue = m_dragSpinBox->maximum();
            m_mouseY = e->y();
            newVar = m_initValue;
        }

        m_dragSpinBox->setValue(newVar);
    }

    return false;
}

//---------------------------------------------------------------------------
// Drag event
//---------------------------------------------------------------------------
void mstEditor::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls())
    {
        QString fileName = e->mimeData()->urls()[0].toLocalFile();
        if (fileName.endsWith(".mst"))
        {
            e->acceptProposedAction();
        }
    }
}

//---------------------------------------------------------------------------
// Drop event
//---------------------------------------------------------------------------
void mstEditor::dropEvent(QDropEvent *e)
{
    // Push window to front
    activateWindow();

    if (!DiscardSaveMessage("Open", "You have unsaved changes, continue without saving?", true))
    {
        return;
    }

    // Pass the first argument
    OpenFile(e->mimeData()->urls()[0].toLocalFile());
}

//---------------------------------------------------------------------------
// Drag drop file to .exe
//---------------------------------------------------------------------------
void mstEditor::passArgument(const std::string &_file)
{
    OpenFile(QString::fromStdString(_file), false);
}

//---------------------------------------------------------------------------
// Opening .mst file
//---------------------------------------------------------------------------
void mstEditor::on_actionOpen_triggered()
{
    if (!DiscardSaveMessage("Open", "You have unsaved changes, continue without saving?", true))
    {
        return;
    }

    QString path = "";
    if (!m_path.isEmpty())
    {
        path = m_path;
    }

    QString mstFile = QFileDialog::getOpenFileName(this, tr("Open"), path, "MST File (*.mst)");
    if (mstFile == Q_NULLPTR) return;

    OpenFile(mstFile);
}

//---------------------------------------------------------------------------
// Opening .mst file
//---------------------------------------------------------------------------
void mstEditor::OpenFile(QString const& mstFile, bool showSuccess)
{
    if (mstFile.endsWith(".mst"))
    {
        // Save directory
        QFileInfo info(mstFile);
        m_path = info.dir().absolutePath();

        // Load fco file
        string errorMsg;
        if (!m_mst.Load(mstFile.toStdString(), errorMsg))
        {
            ResetProgram();
            QMessageBox::critical(this, "Error", QString::fromStdString(errorMsg), QMessageBox::Ok);
        }
        else
        {
            ResetProgram();

            // Update local entries and tree view
            TW_Refresh();

            m_fileName = mstFile;
            int index = m_fileName.lastIndexOf('\\');
            if (index == -1) index = m_fileName.lastIndexOf('/');
            ui->L_FileName->setText("File Name: " + m_fileName.mid(index + 1));

            // Enable search
            ui->LE_Find->setEnabled(true);
            ui->RB_Top->setEnabled(true);
            ui->RB_Current->setEnabled(true);
            ui->PB_Find->setEnabled(true);

            if (showSuccess)
            {
                QMessageBox::information(this, "Open", "File load successful!", QMessageBox::Ok);
            }
        }
    }
    else
    {
        QMessageBox::critical(this, "Error", "Unsupported format!", QMessageBox::Ok);
    }
}

//---------------------------------------------------------------------------
// Saving .mst file
//---------------------------------------------------------------------------
void mstEditor::on_actionSave_triggered()
{
    if (!m_mst.IsLoaded())
    {
        return;
    }

    if (!DiscardSaveMessage("Save", "You have not applied changes yet, continue without applying?", false))
    {
        return;
    }

    // Save fco file
    string errorMsg;
    if (!m_mst.Save(m_fileName.toStdString(), errorMsg))
    {
        QMessageBox::critical(this, "Error", QString::fromStdString(errorMsg), QMessageBox::Ok);
    }
    else
    {
        m_fileEdited = false;
        QMessageBox::information(this, "Save", "File save successful!", QMessageBox::Ok);
    }
}

//---------------------------------------------------------------------------
// Saving as a new .mst file
//---------------------------------------------------------------------------
void mstEditor::on_actionSave_as_triggered()
{
    if (!m_mst.IsLoaded())
    {
        return;
    }

    if (!DiscardSaveMessage("Save As", "You have not applied changes yet, continue without applying?", false))
    {
        return;
    }

    QString path = "";
    if (!m_path.isEmpty())
    {
        path = m_path;
    }

    QString mstFile = QFileDialog::getSaveFileName(this, tr("Save As .mst File"), path, "MST File (*.mst)");
    if (mstFile == Q_NULLPTR) return;

    // Save directory
    QFileInfo info(mstFile);
    m_path = info.dir().absolutePath();

    // Save fco file
    string errorMsg;
    if (!m_mst.Save(mstFile.toStdString(), errorMsg))
    {
        QMessageBox::critical(this, "Error", QString::fromStdString(errorMsg), QMessageBox::Ok);
    }
    else
    {
        m_fileEdited = false;
        QMessageBox::information(this, "Save As", "File save successful!", QMessageBox::Ok);
    }
}

//---------------------------------------------------------------------------
// Export .txt file
//---------------------------------------------------------------------------
void mstEditor::on_actionExport_triggered()
{
    if (!m_mst.IsLoaded())
    {
        return;
    }

    if (!DiscardSaveMessage("Export", "You have not applied changes yet, continue without applying?", false))
    {
        return;
    }

    int index = m_fileName.indexOf(".mst");
    QString fullTextFileName = m_fileName.mid(0, index) + ".txt";

    m_mst.Export(fullTextFileName.toStdString(), ui->CB_Russian->isChecked());
    QString fileName = fullTextFileName.mid(m_fileName.lastIndexOf('/') + 1);
    QMessageBox::information(this, "Export", fileName + " has been exported at the same directory!", QMessageBox::Ok);

    // Open file explorer
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_path));
}

//---------------------------------------------------------------------------
// Close application
//---------------------------------------------------------------------------
void mstEditor::on_actionClose_triggered()
{
    if (m_mst.IsLoaded())
    {
        if (!DiscardSaveMessage("Close", "Quit without saving?", true))
        {
            return;
        }
    }

    QApplication::quit();
}

//---------------------------------------------------------------------------
// About mstEditor
//---------------------------------------------------------------------------
void mstEditor::on_actionAbout_mstEditor_triggered()
{
    QString message = "mstEditor - Sonic the Hedgehog (2006) Subtitle Editor v2.1";
    message += "\nCreated by brianuuu 2019";
    message += "\nYoutube: brianuuuSonic Reborn";
    message += "\nSpecial Thanks: N69vid - Beta testing, Russian mapping";
    message += "\n\nv1.0 - Initial Release";
    message += "\nv2.0 - Added Color Support";
    message += "\nv2.1 - Bug Fix on able to add color to sound before";
    QMessageBox::information(this, "About mstEditor", message, QMessageBox::Ok);
}

//---------------------------------------------------------------------------
// About Qt
//---------------------------------------------------------------------------
void mstEditor::on_actionAbout_Qt_triggered()
{
    QMessageBox::aboutQt(this, "About Qt");
}

//---------------------------------------------------------------------------
// Closing application
//---------------------------------------------------------------------------
void mstEditor::closeEvent (QCloseEvent *event)
{
    if (m_mst.IsLoaded())
    {
        if (!DiscardSaveMessage("Close", "Quit without saving?", true))
        {
            event->ignore();
            return;
        }
    }

    event->accept();
}

//---------------------------------------------------------------------------
// Alt+S
//---------------------------------------------------------------------------
void mstEditor::on_Shortcut_ApplyChanges()
{
    if (ui->PB_Save->isEnabled())
    {
        on_PB_Save_clicked();
    }
}

//---------------------------------------------------------------------------
// Alt+R
//---------------------------------------------------------------------------
void mstEditor::on_Shortcut_ResetSubtitle()
{
    if (ui->PB_Reset->isEnabled())
    {
        on_PB_Reset_clicked();
    }
}

//---------------------------------------------------------------------------
// Ctrl+F
//---------------------------------------------------------------------------
void mstEditor::on_Shortcut_Find()
{
    if (ui->LE_Find->isEnabled())
    {
        ui->LE_Find->setFocus();
        ui->LE_Find->selectAll();
    }
}

//---------------------------------------------------------------------------
// Resetting all buttons and data
//---------------------------------------------------------------------------
void mstEditor::ResetProgram()
{
    m_fileEdited = false;

    ui->L_FileName->setText("File Name: ---");

    ResetEditor();

    ui->PB_SubtitleAdd->setEnabled(false);
    ui->PB_SubtitleDelete->setEnabled(false);
    ui->TW_TreeWidget->scrollToTop();

    ui->LE_Find->setEnabled(false);
    ui->RB_Top->setEnabled(false);
    ui->RB_Current->setEnabled(false);
    ui->PB_Find->setEnabled(false);
}

//---------------------------------------------------------------------------
// Reset subtitle editor
//---------------------------------------------------------------------------
void mstEditor::ResetEditor()
{
    ui->LE_SubtitleName->setEnabled(false);
    ui->LE_SubtitleName->setText("");

    m_id = -1;
    m_page = -1;
    m_name.clear();
    m_subtitles.clear();
    m_tags.clear();

    m_initValue = 0;
    m_mouseY = 0;
    m_dragScale = 0;
    m_dragSpinBox = Q_NULLPTR;

    ui->TE_TextEditor->setEnabled(false);
    ui->TE_TextEditor->setText("");
    m_previewLabel->setText("");
    m_subtitleEdited = false;
    m_subtitleHardcoded = false;

    SetCurrentPageLabel();
    RemoveAllButtonComboBox();

    RemoveAllColorBlocks();
    m_colorBlocks.clear();

    ui->CB_Sound->setEnabled(false);
    ui->CB_Sound->setChecked(false);
    ui->LE_Sound->setEnabled(false);
    ui->LE_Sound->setText("");

    ui->PB_PagePrev->setEnabled(false);
    ui->PB_PageNext->setEnabled(false);
    ui->PB_PageAdd->setEnabled(false);
    ui->PB_PageDelete->setEnabled(false);
    ui->PB_Save->setEnabled(false);
    ui->PB_Reset->setEnabled(false);
    ui->PB_SubtitleDelete->setEnabled(false);
    ui->PB_ColorAdd->setEnabled(false);
}

//---------------------------------------------------------------------------
// Message box when user have unsaved changes
//---------------------------------------------------------------------------
bool mstEditor::DiscardSaveMessage(QString _title, QString _message, bool _checkFileEdited)
{
    if ((_checkFileEdited && m_fileEdited) || m_subtitleEdited)
    {
        QMessageBox::StandardButton resBtn = QMessageBox::Yes;
        resBtn = QMessageBox::warning(this, _title, _message, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (resBtn == QMessageBox::No)
        {
            return false;
        }
    }

    return true;
}

//---------------------------------------------------------------------------
// Refresh entire tree view with current entries
//---------------------------------------------------------------------------
void mstEditor::TW_Refresh()
{
    ui->TW_TreeWidget->clear();

    vector<mst::TextEntry> entries;
    m_mst.GetAllEntries(entries);

    for(mst::TextEntry const& entry : entries)
    {
        TW_AddOrReplaceEntry(entry);
    }

    bool empty = entries.empty();
    ui->PB_SubtitleAdd->setEnabled(!empty);
    // Delete button is handled after loading subtitle
}

//---------------------------------------------------------------------------
// Add a new entry to tree view
//---------------------------------------------------------------------------
void mstEditor::TW_AddOrReplaceEntry(mst::TextEntry entry, int _id)
{
    QTreeWidgetItem *item;
    if (_id == -1)
    {
        // New entry
        item = new QTreeWidgetItem(ui->TW_TreeWidget);
    }
    else
    {
        // Replace existing entry
        item = ui->TW_TreeWidget->topLevelItem(_id);
    }

    item->setText(0, QString::fromStdString(entry.m_name));

    QString subtitle;
    for(unsigned int i = 0; i < entry.m_subtitles.size(); i++)
    {
        subtitle += QString::fromStdWString(entry.m_subtitles[i]);
        if (i != entry.m_subtitles.size() - 1)
        {
            subtitle += "\n\n";
        }
    }
    if (ui->CB_Russian->isChecked())
    {
        subtitle = ToRussian(subtitle);
    }
    item->setText(1, subtitle);

    QString tags;
    for(unsigned int i = 0; i < entry.m_tags.size(); i++)
    {
        tags += QString::fromStdString(entry.m_tags[i]);
        if (i != entry.m_tags.size() - 1)
        {
            tags += "\n";
        }
    }
    item->setText(2, tags);
}

//---------------------------------------------------------------------------
// Item in tree widget clicked
//---------------------------------------------------------------------------
void mstEditor::on_TW_TreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (!DiscardSaveMessage("Discard", "Discard unsaved changes?", false))
    {
       return;
    }

    LoadSubtitle(ui->TW_TreeWidget->indexOfTopLevelItem(item));
}

//---------------------------------------------------------------------------
// Add a new subititle
//---------------------------------------------------------------------------
void mstEditor::on_PB_SubtitleAdd_clicked()
{
    // Add new entry and update tree entry
    int id = m_mst.AddNewEntry();
    mst::TextEntry const entry = m_mst.GetEntry(static_cast<unsigned int>(id));

    // Focus at entry
    TW_FocusItem(id);
    TW_AddOrReplaceEntry(entry);

    m_fileEdited = true;
}

//---------------------------------------------------------------------------
// Delete current subititle
//---------------------------------------------------------------------------
void mstEditor::on_PB_SubtitleDelete_clicked()
{
    if (m_id < 0 || m_id >= ui->TW_TreeWidget->topLevelItemCount()) return;

    m_mst.RemoveEntry(static_cast<unsigned int>(m_id));
    delete ui->TW_TreeWidget->takeTopLevelItem(m_id);
    ResetEditor();

    m_fileEdited = true;
}

//---------------------------------------------------------------------------
// Find button pressed
//---------------------------------------------------------------------------
void mstEditor::on_PB_Find_clicked()
{
    TW_Find();
}

//---------------------------------------------------------------------------
// Start searching
//---------------------------------------------------------------------------
void mstEditor::on_LE_Find_returnPressed()
{
    TW_Find();
}

//---------------------------------------------------------------------------
// Search text changed
//---------------------------------------------------------------------------
void mstEditor::on_LE_Find_textEdited(const QString &arg1)
{
    // Assume user want to search from the beginning
    ui->RB_Top->setChecked(true);
}

//---------------------------------------------------------------------------
// Find string in tree view
//---------------------------------------------------------------------------
void mstEditor::TW_Find()
{
    QString const str = ui->LE_Find->text();
    if (str.isEmpty()) return;

    if (!DiscardSaveMessage("Find", "You have not \"Apply Changes\" yet, continue without applying?", false))
    {
        return;
    }

    bool found = false;
    int page = 0;
    int findID = ui->RB_Top->isChecked() ? 0 : (m_id + 1);
    for (; findID < ui->TW_TreeWidget->topLevelItemCount() ; findID++)
    {
        // Go through tree view
        QTreeWidgetItem const* item = ui->TW_TreeWidget->topLevelItem(findID);
        for (int i = 0; i < 3; i++)
        {
            // Find within name, subtitle and tags
            QString const columnString = item->text(i);
            int index = columnString.indexOf(str, 0, Qt::CaseInsensitive);
            if (index != -1)
            {
                if (i == 1)
                {
                    // This is a subtitle, get the page number
                    page = columnString.mid(0, index).count("\n\n");
                }
                else if (i == 2)
                {
                    // This is a tag, find the page that uses it
                    int tagID = columnString.mid(0, index).count('\n');
                    int tagCount = -1;

                    // Find the page that uses the tag
                    QString const subtitle = item->text(1);
                    for (int c = 0; c < subtitle.size(); c++)
                    {
                        if ((c + 1) < subtitle.size() && subtitle[c] == '\n' && subtitle[++c] == '\n') page++;
                        if (subtitle[c] == '$') tagCount++;
                        if (tagCount == tagID) break;
                    }
                }

                found = true;
                break;
            }
        }

        if (found) break;
    }

    if (found)
    {
        // Focus on found item
        TW_FocusItem(findID);

        // Load the subtitle and set focus to find again
        LoadSubtitle(findID, page);
        ui->RB_Current->setChecked(true);
    }
    else
    {
        QMessageBox::information(this, "Find", "Subtitle not found or reached the end of search.", QMessageBox::Ok);
        ui->RB_Top->setChecked(true);
    }

    // Select all and focus on find line edit
    ui->LE_Find->setFocus();
    ui->LE_Find->selectAll();
}

//---------------------------------------------------------------------------
// Load a subtitle for editor
//---------------------------------------------------------------------------
void mstEditor::TW_FocusItem(int _id)
{
    if (_id < 0 || _id >= ui->TW_TreeWidget->topLevelItemCount()) return;

    QTreeWidgetItem* item = ui->TW_TreeWidget->topLevelItem(_id);
    ui->TW_TreeWidget->setCurrentItem(item);
    ui->TW_TreeWidget->scrollTo(ui->TW_TreeWidget->currentIndex(), QAbstractItemView::ScrollHint::PositionAtCenter);
    ui->TW_TreeWidget->setFocus();
}

//---------------------------------------------------------------------------
// Load a subtitle for editor
//---------------------------------------------------------------------------
void mstEditor::LoadSubtitle(int _id, int _page)
{
    // Un-highlight previous selection
    if (m_id >= 0)
    {
        QTreeWidgetItem* item = ui->TW_TreeWidget->topLevelItem(m_id);
        item->setForeground(0, QColor(0,0,0));
        item->setForeground(1, QColor(0,0,0));
        item->setForeground(2, QColor(0,0,0));
    }

    // Loading a new subtitle, put back all the color tags
    InsertColorTagsToCurrentPage(true);

    if (_id < 0 || _page < 0) return;

    // Highlight selected
    m_id = _id;
    QTreeWidgetItem* item = ui->TW_TreeWidget->topLevelItem(m_id);
    item->setForeground(0, QColor(255,0,0));
    item->setForeground(1, QColor(255,0,0));
    item->setForeground(2, QColor(255,0,0));

    mst::TextEntry const entry = m_mst.GetEntry(static_cast<unsigned int>(m_id));
    m_name = QString::fromStdString(entry.m_name);

    m_tags.clear();
    for (string const& tag : entry.m_tags)
    {
        // Remove "sound(" or "picture(" and ")"
        QString str = QString::fromStdString(tag);
        int start = str.indexOf("("); // Color will be -1

        // Record tag type, default as button
        Tag tagType = Tag::Picture;
        QString type = str.mid(0, start);
        if (type == "sound")
        {
            tagType = Tag::Sound;
        }
        else if (type == "rgba")
        {
            tagType = Tag::RGBA;
        }

        if (type == "color")
        {
            // Color has no brackets
            tagType = Tag::Color;
        }
        else
        {
            str.remove(0, start + 1);
            str.remove(str.size() - 1, 1);
        }

        m_tags.push_back(TagPair(str, tagType));
    }

    // Loop through all the tags again, check for invalid color
    for (int i = 0; i < m_tags.size(); i++)
    {
        TagPair& tagPair = m_tags[i];
        if (tagPair.second == Tag::RGBA)
        {
            bool valid = true;

            QList<int> rgbaNum;
            QStringList rgbaStr = tagPair.first.split(",");
            if (rgbaStr.size() == 3 || rgbaStr.size() == 4)
            {
                // Validate individual number
                for (QString const& numStr : rgbaStr)
                {
                    bool ok = false;
                    int num = numStr.toInt(&ok);
                    if (ok) rgbaNum.push_back(num);
                    valid &= ok;
                }
            }

            if (valid)
            {
                // Search if a "Color" tag exist after this
                valid = false;
                for (int j = i + 1; j < m_tags.size(); j++)
                {
                    if (m_tags[j].second == Tag::Color)
                    {
                        valid = true;
                        break;
                    }
                }
            }

            if (!valid)
            {
                // Invalid rgba tag, set to button
                tagPair.first = m_buttonToString[Button::A];
                tagPair.second = Tag::Picture;
            }
        }
        else if (tagPair.second == Tag::Color)
        {
            // Search if a "RGBA" tag exist before this
            bool valid = false;
            for (int j = i - 1; j >= 0; j--)
            {
                if (m_tags[j].second == Tag::RGBA)
                {
                    valid = true;
                    break;
                }
            }

            if (!valid)
            {
                // Invalid color tag, set to button
                tagPair.first = m_buttonToString[Button::A];
                tagPair.second = Tag::Picture;
            }
        }
    }

    m_subtitles.clear();
    for (wstring const& subtitle : entry.m_subtitles)
    {
        m_subtitles.push_back(QString::fromStdWString(subtitle));
    }

    // Check if number of $ match the number of tags
    int tagCount = 0;
    for (QString const& subtitle : m_subtitles)
    {
        tagCount += subtitle.count("$");
    }

    bool forceEdited = false;
    m_subtitleHardcoded = false;
    if (m_tags.isEmpty() && tagCount > 0)
    {
        // Hard-coded subtitle
        QMessageBox::StandardButton resBtn = QMessageBox::Yes;
        QString message = "Subtitle contains $ but has no sounds or buttons.";
        message += "\nThis is most likely a hard-coded subtitle and not recommended to be edited.";
        message += "\nYou should keep the same number of $ and edit it at your own risk.";
        message += "\nEditing will be restricted, do you wish to continue?";
        resBtn = QMessageBox::warning(this, "Hardcoded Subtitle", message, QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (resBtn == QMessageBox::No)
        {
            ResetEditor();
            item->setForeground(0, QColor(0,0,0));
            item->setForeground(1, QColor(0,0,0));
            item->setForeground(2, QColor(0,0,0));
            return;
        }
        else
        {
            m_subtitleHardcoded = true;
        }
    }
    else if (m_tags.size() != tagCount)
    {
        // Missing tags
        QMessageBox::warning(this, "Warning", "Number of $ does not match the number of sounds and buttons!\nAll the tags will be removed!", QMessageBox::Ok);
        m_tags.clear();
        for (QString& subtitle : m_subtitles)
        {
            subtitle.remove('$');
        }
        forceEdited = true;
    }

    // Load initial or specific page
    if (_page < m_subtitles.size())
    {
        LoadPage(_page);
    }
    else
    {
        LoadPage(0);
    }

    // Subtitle name
    ui->LE_SubtitleName->setEnabled(!m_subtitleHardcoded);
    ui->LE_SubtitleName->setText(QString::fromStdString(entry.m_name));

    // Enable add page if not hard-coded
    ui->PB_PageAdd->setEnabled(!m_subtitleHardcoded);
    ui->PB_SubtitleDelete->setEnabled(!m_subtitleHardcoded);
    ui->PB_ColorAdd->setEnabled(!m_subtitleHardcoded);

    SetSubtitleEdited(forceEdited);
}

//---------------------------------------------------------------------------
// Load one subtitle page
//---------------------------------------------------------------------------
void mstEditor::LoadPage(int _page)
{
    if (_page < 0 || _page >= m_subtitles.size())
    {
        ui->TE_TextEditor->setText("");
        return;
    }

    // Put back all the color tags for the old page
    InsertColorTagsToCurrentPage(true);

    m_page = _page;
    SetCurrentPageLabel();

    // Loading a new page, grab all color tags
    GetColorTagsFromCurrentPage(true);
    QString subtitle = m_subtitles[m_page];

    // Count the number of tags it has before this page
    int tagStart = 0;
    for (int i = 0; i < m_page; i++)
    {
        tagStart += m_subtitles[i].count("$");
    }

    // Convert to russian
    if (ui->CB_Russian->isChecked())
    {
        subtitle = ToRussian(subtitle);
    }

    int buttonStart = tagStart;
    int buttonCount = subtitle.count("$");

    // Disable sound check box and line edit so their events doesn't get triggered
    ui->TE_TextEditor->setEnabled(false);
    ui->CB_Sound->setEnabled(false);

    // Check if the first tag is a sound
    bool hasFirstTag = !subtitle.isEmpty() && subtitle[0] == '$';
    bool isSoundTag = tagStart < m_tags.size() && m_tags[tagStart].second == Tag::Sound;
    if (hasFirstTag && isSoundTag)
    {
        ui->CB_Sound->setChecked(true);
        ui->LE_Sound->setText(m_tags[tagStart].first);
        ui->LE_Sound->setEnabled(true);
        subtitle = subtitle.mid(1);

        buttonStart++;
        buttonCount--;
    }
    else
    {
        ui->CB_Sound->setChecked(false);
        ui->LE_Sound->setEnabled(false);
        ui->LE_Sound->setText("");
    }

    // Enable text edit
    ui->TE_TextEditor->setText(subtitle);
    ui->TE_TextEditor->setEnabled(true);

    // Disable sound check box if hard-coded
    ui->CB_Sound->setEnabled(!m_subtitleHardcoded);

    // Delete existing button combo boxes
    RemoveAllButtonComboBox();

    // Non hard-coded mode
    if (!m_subtitleHardcoded)
    {
        // Add button combo boxes
        for(int i = 0; i < buttonCount; i++)
        {
            // If we are here everything should be button
            TagPair& tagPair = m_tags[buttonStart + i];
            tagPair.second = Tag::Picture;
            if (!m_stringToButton.contains(tagPair.first))
            {
                tagPair.first = m_buttonToString[Button::A];
            }
            AddButtonComboBox(i, m_stringToButton[tagPair.first]);
        }
    }

    // Enable page scroll
    ui->PB_PagePrev->setEnabled(m_page > 0);
    ui->PB_PageNext->setEnabled(m_page < m_subtitles.size() - 1);

    // Enable delete if not hard-coded
    ui->PB_PageDelete->setEnabled(m_subtitles.size() > 1 && !m_subtitleHardcoded);

    // Highlight text edit text
    ui->TE_TextEditor->selectAll();
    ui->TE_TextEditor->setFocus();

    UpdateSubtitlePreview();
}

//---------------------------------------------------------------------------
// Add a combo box for selection
//---------------------------------------------------------------------------
void mstEditor::AddButtonComboBox(int _insertAt, Button _button)
{
    if (_insertAt < 0 || _insertAt > ui->VL_Tags->layout()->count()) return;

    QHBoxLayout* pictureLayout = new QHBoxLayout();
    pictureLayout->addStretch();

    // Label
    QLabel* label = new QLabel();
    label->setMinimumSize(QSize(80,22));
    pictureLayout->insertWidget(0, label);

    // Combo box
    QComboBox* comboBox = new QComboBox();
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    pictureLayout->insertWidget(1, comboBox);
    for (int i = 0; i < Button::COUNT; i++)
    {
        // Add all items
        comboBox->addItem(m_buttonToCombo[static_cast<Button>(i)]);
        comboBox->setItemIcon(i, QIcon(":/resources/" + m_buttonToString[static_cast<Button>(i)] + ".png"));
    }
    if (_button < Button::COUNT)
    {
        // Set current index
        comboBox->setCurrentIndex(_button);
    }
    connect(comboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(on_CB_ComboBox_currentIndexChanged(int)));

    // Create layout
    ui->VL_Tags->insertLayout(_insertAt + 1, pictureLayout);

    // Rename id
    for (int i = 0; i < ui->VL_Tags->layout()->count(); i++)
    {
        QLayoutItem* pictureLayoutItem = ui->VL_Tags->layout()->itemAt(i);
        if (pictureLayoutItem->layout()->count() > 1)
        {
            QLabel* label = reinterpret_cast<QLabel*>(pictureLayoutItem->layout()->itemAt(0)->widget());
            label->setText("Picture" + QString::number(i) + ":");
        }
    }
}

//---------------------------------------------------------------------------
// Remove a combo box of given comboID
//---------------------------------------------------------------------------
void mstEditor::RemoveButtonComboBox(int _comboID)
{
    if (_comboID < 0 || _comboID >= ui->VL_Tags->layout()->count()) return;

    QLayoutItem* pictureLayoutItem = ui->VL_Tags->layout()->takeAt(_comboID);
    while (pictureLayoutItem->layout()->count() > 0)
    {
        delete pictureLayoutItem->layout()->takeAt(0)->widget(); // Remove all widgets inside
    }
    delete pictureLayoutItem->widget();                         // Remove layout

    // Rename id
    for (int i = 0; i < ui->VL_Tags->layout()->count(); i++)
    {
        QLayoutItem* pictureLayoutItem = ui->VL_Tags->layout()->itemAt(i);
        if (pictureLayoutItem->layout()->count() > 1)
        {
            QLabel* label = reinterpret_cast<QLabel*>(pictureLayoutItem->layout()->itemAt(0)->widget());
            label->setText("Picture" + QString::number(i) + ":");
        }
    }
}

//---------------------------------------------------------------------------
// Remove all combo boxes
//---------------------------------------------------------------------------
void mstEditor::RemoveAllButtonComboBox()
{
    int comboCount = ui->VL_Tags->layout()->count();
    for (int i = 0; i < comboCount; i++)
    {
        RemoveButtonComboBox(0);
    }
}

//---------------------------------------------------------------------------
// Set the page number label from m_page
//---------------------------------------------------------------------------
void mstEditor::SetCurrentPageLabel()
{
    if (m_page == -1)
    {
        ui->L_Page->setText("Page: --- of ---");
    }
    else
    {
        ui->L_Page->setText("Page: " + QString::number(m_page + 1) + " of " + QString::number(m_subtitles.size()));
    }
}

//---------------------------------------------------------------------------
// Update subtitle preview
//---------------------------------------------------------------------------
void mstEditor::UpdateSubtitlePreview()
{
    if (m_page == -1 || m_page >= m_subtitles.size())
    {
        m_previewLabel->setText("");
        return;
    }

    QString subtitle = m_subtitles[m_page];
    if (ui->CB_Russian->isChecked())
    {
        subtitle = ToRussian(subtitle);
    }

    int tagID = 0;
    if (!m_subtitleHardcoded)
    {
        // Count the number of tags it has before this page
        for (int i = 0; i < m_page; i++)
        {
            tagID += m_subtitles[i].count("$");
        }

        // Skip the first $ if sound exist
        if (ui->CB_Sound->isChecked())
        {
            subtitle = subtitle.mid(1);
            tagID++;
        }
    }

    QString htmlString;
    for(int i = 0; i < subtitle.size(); i++)
    {
        QString str(subtitle[i]);

        // Line break in html
        if (str == "\n") str = "<br>";

        // Space in html
        if (str == " ") str = "<font color=\"#00000000\" size = \"2\">..</font>";

        // Japanese Space in html
        if (str == "　") str = "<font color=\"#00000000\">あ</font>";

        // Button images
        if (str == "$" && !m_subtitleHardcoded)
        {
            TagPair tagPair = m_tags[tagID];
            Q_ASSERT(tagPair.second != Tag::RGBA && tagPair.second != Tag::Color);
            if (tagPair.second == Tag::Picture)
            {
                str = "<img src=\":/resources/" + tagPair.first + ".png\">";
            }
            tagID++;
        }

        for(ColorBlock const& colorBlock : m_colorBlocks)
        {
            if (colorBlock.m_start == i)
            {
                QColor const& color = colorBlock.m_color;
                QString r,g,b,a;
                r.setNum(color.red(),   16); if (color.red()    < 0x10) r = "0" + r;
                g.setNum(color.green(), 16); if (color.green()  < 0x10) g = "0" + g;
                b.setNum(color.blue(),  16); if (color.blue()   < 0x10) b = "0" + b;
                a.setNum(color.alpha(), 16); if (color.alpha()  < 0x10) a = "0" + a;
                str = "<font color=\"#" + a + r + g + b + "\">" + str;
            }

            if (colorBlock.m_end == i)
            {
                str = str + "</font>";
            }
        }

        htmlString += str;
    }

    m_previewLabel->setText(htmlString);
}

//---------------------------------------------------------------------------
// Remove all color tags from subtitle and save them separately
//---------------------------------------------------------------------------
void mstEditor::GetColorTagsFromCurrentPage(bool _insertUI)
{
    if (m_id < 0 || m_page < 0) return;

    QString subtitle = m_subtitles[m_page];
    m_colorBlocks.clear();
    int colorIndex = -1;

    // Count the number of tags it has before this page
    int tagStart = 0;
    for (int i = 0; i < m_page; i++)
    {
        tagStart += m_subtitles[i].count("$");
    }

    // Temporary remove color $ in m_subtitle[m_page]
    int tagIndex = subtitle.indexOf('$', 0);
    int tagID = (tagIndex != -1) ? 0 : -1;
    while (tagID != -1)
    {
        TagPair& tagPair = m_tags[tagStart + tagID];

        if (tagPair.second == Tag::RGBA)
        {
            ColorBlock colorBlock;
            colorBlock.m_start = tagIndex;

            QStringList rgbaStr = tagPair.first.split(",");
            colorBlock.m_color = QColor(rgbaStr[0].toInt(), rgbaStr[1].toInt(), rgbaStr[2].toInt(), rgbaStr.size() == 4 ? rgbaStr[3].toInt() : 255);

            m_colorBlocks.push_front(colorBlock);
            colorIndex++;
        }
        else if (tagPair.second == Tag::Color)
        {
            ColorBlock& colorBlock = m_colorBlocks[colorIndex];
            colorBlock.m_end = tagIndex - 1;
            colorIndex--;
        }

        if (tagPair.second == Tag::RGBA || tagPair.second == Tag::Color)
        {
            subtitle.remove(tagIndex, 1);
            m_tags.remove(tagStart + tagID);

            tagIndex--;
            tagID--;
        }

        tagIndex = subtitle.indexOf('$', tagIndex + 1);
        tagID = (tagIndex != -1) ? tagID + 1 : -1;
    }

    m_subtitles[m_page] = subtitle;

    // Reverse color block order
    QVector<ColorBlock> temp;
    for (ColorBlock const& colorBlock : m_colorBlocks)
    {
        temp.push_front(colorBlock);
    }
    m_colorBlocks.clear();
    m_colorBlocks = temp;

    // If sound tag exist, push front all indices by 1
    if (!m_tags.empty() && m_tags[tagStart].second == Tag::Sound)
    {
        for (ColorBlock& colorBlock : m_colorBlocks)
        {
            colorBlock.m_start--;
            colorBlock.m_end--;
            Q_ASSERT(colorBlock.m_start >= 0 && colorBlock.m_end >= 0);
        }
    }

    if (_insertUI)
    {
        for (ColorBlock const& colorBlock : m_colorBlocks)
        {
            AddColorBlock(ui->VL_Colors->layout()->count(), colorBlock);
        }
    }
}

//---------------------------------------------------------------------------
// Re-insert removed color tags back to the current page
//---------------------------------------------------------------------------
void mstEditor::InsertColorTagsToCurrentPage(bool _removeUI)
{
    if (m_id < 0 || m_page < 0 || m_colorBlocks.empty()) return;

    QString const subtitle = m_subtitles[m_page];
    int tagStart = 0;

    // Count the number of tags it has before this page
    for (int i = 0; i < m_page; i++)
    {
        tagStart += m_subtitles[i].count("$");
    }

    // Skip the first $ if sound exist
    if (ui->CB_Sound->isChecked())
    {
        tagStart++;
    }

    // Add back color to m_tags
    for(int i = 0; i < m_colorBlocks.size(); i++)
    {
        ColorBlock& colorBlock = m_colorBlocks[i];

        QVector<int> positions;
        positions.push_back(colorBlock.m_start);
        positions.push_back(colorBlock.m_end);

        while(!positions.empty())
        {
            int position = positions[0];
            positions.pop_front();

            // Find how many tag exist before this position
            QString subStr = subtitle.mid(ui->CB_Sound->isChecked() ? 1 : 0, position);
            int tagID = subStr.count('$');

            // Also add the number color tags exist before this position
            for (int j = 0; j < i; j++)
            {
                ColorBlock& checkColorBlock = m_colorBlocks[j];
                if (checkColorBlock.m_start < position) tagID++;
                if (checkColorBlock.m_end < position) tagID++;
            }

            if (!positions.empty())
            {
                // Front
                QColor const& color = colorBlock.m_color;
                QString tagStr = QString::number(color.red()) + "," + QString::number(color.green()) + "," + QString::number(color.blue());
                if (color.alpha() != 255)
                {
                    tagStr += "," + QString::number(color.alpha());
                }
                m_tags.insert(tagStart + tagID, TagPair(tagStr, Tag::RGBA));
            }
            else
            {
                // Need to add 1 to ID, start must be before end
                tagID++;

                // Back
                m_tags.insert(tagStart + tagID, TagPair("color", Tag::Color));
            }
        }
    }

    // Finally add back all the $ to the subtitle
    QString fixedSubtitle;
    for(int i = 0; i < subtitle.size(); i++)
    {
        QString str(subtitle[i]);
        for(ColorBlock const& colorBlock : m_colorBlocks)
        {
            if (colorBlock.m_start + (ui->CB_Sound->isChecked() ? 1 : 0) == i)
            {
                str = "$" + str;
            }

            if (colorBlock.m_end + (ui->CB_Sound->isChecked() ? 1 : 0) == i)
            {
                str = str + "$";
            }
        }
        fixedSubtitle += str;
    }

    m_subtitles[m_page] = fixedSubtitle;

    if (_removeUI)
    {
        RemoveAllColorBlocks();
    }
}

//---------------------------------------------------------------------------
// Add color blocks to the editor
//---------------------------------------------------------------------------
void mstEditor::AddColorBlock(int _insertAt, ColorBlock _colorBlock)
{
    QHBoxLayout* colorLayout = new QHBoxLayout();

    // Label (0)
    QLabel* label = new QLabel();
    label->setMinimumSize(80,22);
    label->setText("Color" + QString::number(ui->VL_Colors->layout()->count()) + ":");
    colorLayout->addWidget(label);

    // Color (1)
    QTextBrowser *colorBrowser = new QTextBrowser();
    colorBrowser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    colorBrowser->setMaximumSize(27,27);
    QPalette pal = palette();
    pal.setColor(QPalette::Base, _colorBlock.m_color.rgb());
    colorBrowser->setPalette(pal);
    colorBrowser->installEventFilter(this);
    colorLayout->addWidget(colorBrowser);

    // Alpha (2)
    QSpinBox *alpha = new QSpinBox();
    alpha->setStyleSheet("font: 14px");
    alpha->setFixedSize(50,27);
    alpha->setRange(0,255);
    alpha->setValue(_colorBlock.m_color.alpha());
    alpha->setCursor(Qt::SizeVerCursor);
    alpha->installEventFilter(this);
    colorLayout->addWidget(alpha);
    connect(alpha, SIGNAL(valueChanged(int)), this, SLOT(ColorSpinBoxChanged(int)));

    // Open Bracket (3)
    QLabel* label2 = new QLabel();
    label2->setMinimumSize(22,22);
    label2->setAlignment(Qt::AlignRight);
    label2->setText("(");
    colorLayout->addWidget(label2);

    // Start (4)
    QSpinBox *start = new QSpinBox();
    start->setStyleSheet("font: 14px");
    start->setFixedSize(50,27);
    start->setMinimum(0);
    start->setMaximum(m_subtitles[m_page].size() - 1);
    start->setValue(_colorBlock.m_start);
    start->setCursor(Qt::SizeVerCursor);
    start->installEventFilter(this);
    colorLayout->addWidget(start);
    connect(start, SIGNAL(valueChanged(int)), this, SLOT(ColorSpinBoxChanged(int)));

    // To (5)
    QLabel* label3 = new QLabel();
    label3->setText("to");
    colorLayout->addWidget(label3);

    // End (6)
    QSpinBox *end = new QSpinBox();
    end->setStyleSheet("font: 14px");
    end->setFixedSize(50,27);
    end->setMinimum(0);
    end->setMaximum(m_subtitles[m_page].size() - 1);
    end->setValue(_colorBlock.m_end);
    end->setCursor(Qt::SizeVerCursor);
    end->installEventFilter(this);
    colorLayout->addWidget(end);
    connect(end, SIGNAL(valueChanged(int)), this, SLOT(ColorSpinBoxChanged(int)));

    // Close Bracket (7)
    QLabel* label4 = new QLabel();
    label4->setMinimumSize(22,22);
    label4->setAlignment(Qt::AlignLeft);
    label4->setText(")");
    colorLayout->addWidget(label4);

    // Delete (8)
    QPushButton *del = new QPushButton();
    del->setStyleSheet("font: 14px");
    del->setMaximumSize(20,27);
    del->setText("X");
    colorLayout->addWidget(del);
    connect(del, SIGNAL(clicked()), this, SLOT(ColorDeletePressed()));

    // Create layout
    colorLayout->addStretch();
    ui->VL_Colors->insertLayout(_insertAt, colorLayout);

    for (int i = 0; i < ui->VL_Colors->layout()->count(); i++)
    {
        QLayoutItem* colorLayoutItem = ui->VL_Colors->layout()->itemAt(i);
        QLabel* idLabel = reinterpret_cast<QLabel*>(colorLayoutItem->layout()->itemAt(0)->widget());
        idLabel->setText("Color" + QString::number(i) + ":");

        UpdateStartEndLimits(i);
    }
}

//---------------------------------------------------------------------------
// Add color blocks to the editor
//---------------------------------------------------------------------------
void mstEditor::RemoveColorBlock(int _colorID)
{
    if (_colorID < 0 || _colorID >= m_colorBlocks.size()) return;

    QLayoutItem* colorLayoutItem = ui->VL_Colors->layout()->takeAt(_colorID);
    while (colorLayoutItem->layout()->count() > 0)
    {
        delete colorLayoutItem->layout()->takeAt(0)->widget(); // Remove all widgets inside
    }
    delete colorLayoutItem->widget();                          // Remove layout
    m_colorBlocks.remove(_colorID);

    for (int i = 0; i < ui->VL_Colors->layout()->count(); i++)
    {
        QLayoutItem* colorLayoutItem = ui->VL_Colors->layout()->itemAt(i);
        QLabel* idLabel = reinterpret_cast<QLabel*>(colorLayoutItem->layout()->itemAt(0)->widget());
        idLabel->setText("Color" + QString::number(i) + ":");
    }
}

//---------------------------------------------------------------------------
// Remove all color blocks
//---------------------------------------------------------------------------
void mstEditor::RemoveAllColorBlocks()
{
    while(!m_colorBlocks.empty())
    {
        RemoveColorBlock(0);
    }
}

//---------------------------------------------------------------------------
// Update start and end limits of a color
//---------------------------------------------------------------------------
void mstEditor::UpdateStartEndLimits(int _colorID)
{
    if (m_subtitles[m_page].isEmpty())
    {
        return;
    }

    QLayoutItem* colorLayoutItem = ui->VL_Colors->layout()->itemAt(_colorID);
    QSpinBox* start = reinterpret_cast<QSpinBox*>(colorLayoutItem->layout()->itemAt(4)->widget());
    QSpinBox* end = reinterpret_cast<QSpinBox*>(colorLayoutItem->layout()->itemAt(6)->widget());

    // Clamp it first
    start->setMinimum(0);
    end->setMinimum(0);
    start->setMaximum(m_subtitles[m_page].size() - 1);
    end->setMaximum(m_subtitles[m_page].size() - 1);

    start->setMaximum(end->value());
    int min = 0;
    for (int i = 0; i < m_colorBlocks.size(); i++)
    {
        int otherEnd = m_colorBlocks[i].m_end;
        if (i != _colorID && otherEnd < end->value() && otherEnd >= min)
        {
            min = otherEnd + 1;
        }
    }
    start->setMinimum(qMin(m_subtitles[m_page].size() - 1, min));

    end->setMinimum(start->value());
    int max = m_subtitles[m_page].size() - (ui->CB_Sound->isChecked() ? 2 : 1);
    for (int i = 0; i < m_colorBlocks.size(); i++)
    {
        int otherStart = m_colorBlocks[i].m_start;
        if (i != _colorID && otherStart > start->value() && otherStart <= max)
        {
            max = otherStart - 1;
        }
    }
    end->setMaximum(qMax(0, max));
}

//---------------------------------------------------------------------------
// Signal sent when spin box value has changed
//---------------------------------------------------------------------------
void mstEditor::ColorSpinBoxChanged(int _value)
{
    QWidget* widget = reinterpret_cast<QWidget*>(sender());

    int indexAlpha = -1;
    for (int i = 0; i < ui->VL_Colors->layout()->count(); i++)
    {
        // Look for alpha
        QLayoutItem* colorLayoutItem = ui->VL_Colors->layout()->itemAt(i);
        if (colorLayoutItem->layout()->itemAt(2)->widget() == widget)
        {
            indexAlpha = i;
            break;
        }
    }

    int indexStart = -1;
    int indexEnd = -1;
    for (int i = 0; i < ui->VL_Colors->layout()->count(); i++)
    {
        // Look for start or end
        QLayoutItem* colorLayoutItem = ui->VL_Colors->layout()->itemAt(i);
        if (colorLayoutItem->layout()->itemAt(4)->widget() == widget)
        {
            indexStart = i;
            break;
        }
        else if (colorLayoutItem->layout()->itemAt(6)->widget() == widget)
        {
            indexEnd = i;
            break;
        }
    }

    if (indexAlpha != -1)
    {
        m_colorBlocks[indexAlpha].m_color.setAlpha(_value);
    }
    else if (indexStart != -1)
    {
        m_colorBlocks[indexStart].m_start = _value;
        UpdateStartEndLimits(indexStart);
    }
    else if (indexEnd != -1)
    {
        m_colorBlocks[indexEnd].m_end = _value;
        UpdateStartEndLimits(indexEnd);
    }
    else
    {
        return;
    }

    UpdateSubtitlePreview();
    SetSubtitleEdited(true);
}

//---------------------------------------------------------------------------
// Signal sent when delete button is pressed
//---------------------------------------------------------------------------
void mstEditor::ColorDeletePressed()
{
    QWidget* widget = reinterpret_cast<QWidget*>(sender());
    int index = -1;
    for (int i = 0; i < ui->VL_Colors->layout()->count(); i++)
    {
        QLayoutItem* colorLayoutItem = ui->VL_Colors->layout()->itemAt(i);
        if (colorLayoutItem->layout()->itemAt(8)->widget() == widget)
        {
            index = i;
            break;
        }
    }

    if (index == -1) return;

    RemoveColorBlock(index);
    UpdateSubtitlePreview();
    SetSubtitleEdited(true);
}

//---------------------------------------------------------------------------
// Set apply and reset button enabled
//---------------------------------------------------------------------------
void mstEditor::SetSubtitleEdited(bool _edited)
{
    m_subtitleEdited = _edited;
    ui->PB_Save->setEnabled(_edited);
    ui->PB_Reset->setEnabled(_edited);
    if (_edited && ui->CB_AutoApply->isChecked())
    {
        on_PB_Save_clicked();
    }
}

//---------------------------------------------------------------------------
// What the fuck is this?
//---------------------------------------------------------------------------
void mstEditor::on_PB_TagWhat_clicked()
{
    QMessageBox::information(this, "What is this?", "To add references of buttons, add $ in the subtitle!", QMessageBox::Ok);
}

//---------------------------------------------------------------------------
// Goto previous page
//---------------------------------------------------------------------------
void mstEditor::on_PB_PagePrev_clicked()
{
    if (m_page == 0) return;
    LoadPage(m_page - 1);
}

//---------------------------------------------------------------------------
// Goto next page
//---------------------------------------------------------------------------
void mstEditor::on_PB_PageNext_clicked()
{
    if (m_page >= m_subtitles.size() - 1) return;
    LoadPage(m_page + 1);
}

//---------------------------------------------------------------------------
// Add a next page to the end
//---------------------------------------------------------------------------
void mstEditor::on_PB_PageAdd_clicked()
{
    m_subtitles.push_back("*INSERT SUBTITLE HERE*");
    LoadPage(m_subtitles.size() - 1);
    SetCurrentPageLabel();

    SetSubtitleEdited(true);
}

//---------------------------------------------------------------------------
// Delete current page
//---------------------------------------------------------------------------
void mstEditor::on_PB_PageDelete_clicked()
{
    ui->CB_Sound->setChecked(false);    // Event will handle line edit box
    ui->TE_TextEditor->setText("");     // Event will remove tags and set m_subtitle[m_page]

    m_subtitles.remove(m_page);
    LoadPage(max(m_page - 1, 0));

    SetSubtitleEdited(true);
}

//---------------------------------------------------------------------------
// Replace subtitle
//---------------------------------------------------------------------------
void mstEditor::on_PB_Save_clicked()
{
    if (!m_subtitleEdited) return;

    // If current page has color, add them back in
    InsertColorTagsToCurrentPage(false);

    mst::TextEntry entry;
    entry.m_name = m_name.toStdString();

    for (QString subtitle : m_subtitles)
    {
        if (ui->CB_Russian->isChecked())
        {
            subtitle = ToUnicode(subtitle);
        }
        entry.m_subtitles.push_back(subtitle.toStdWString());
    }

    for (TagPair const& tagPair : m_tags)
    {
        if (tagPair.second == Tag::Sound)
        {
            entry.m_tags.push_back("sound(" + tagPair.first.toStdString() + ")");
        }
        else if (tagPair.second == Tag::RGBA)
        {
            entry.m_tags.push_back("rgba(" + tagPair.first.toStdString() + ")");
        }
        else if (tagPair.second == Tag::Color)
        {
            entry.m_tags.push_back("color");
        }
        else
        {
            entry.m_tags.push_back("picture(" + tagPair.first.toStdString() + ")");
        }
    }

    // Replace entry
    m_mst.ModifyEntry(static_cast<unsigned int>(m_id), entry);

    // Update in Tree View
    TW_AddOrReplaceEntry(entry, m_id);

    // Save and reset button
    m_fileEdited = true;
    SetSubtitleEdited(false);

    // Remove color tags again
    GetColorTagsFromCurrentPage(false);
}

//---------------------------------------------------------------------------
// Reload subtitle
//---------------------------------------------------------------------------
void mstEditor::on_PB_Reset_clicked()
{
    if (DiscardSaveMessage("Reset", "Reset subtitle? (Unsaved changes will be lost!)", false))
    {
        LoadSubtitle(m_id);
    }
}

//---------------------------------------------------------------------------
// Add a new color block
//---------------------------------------------------------------------------
void mstEditor::on_PB_ColorAdd_clicked()
{
    // Check if first subtitle has sound
    int size = m_subtitles[m_page].size() - (ui->CB_Sound->isChecked() ? 1 : 0);

    // Search for empty spots
    QVector<bool> occupied(size, false);
    for (int i = 0; i < m_colorBlocks.size(); i++)
    {
        ColorBlock const& colorBlock = m_colorBlocks[i];
        for (int j = colorBlock.m_start; j <= colorBlock.m_end && j < size; j++)
        {
            occupied[j] = true;
        }
    }

    ColorBlock newColorBlock;
    int unoccupiedIndex = -1;
    for (int i = 0; i < occupied.size(); i++)
    {
        if (!occupied[i])
        {
            unoccupiedIndex = i;
            break;
        }
    }

    if (occupied.isEmpty())
    {
        QMessageBox::warning(this, "Warning", "Cannot add color to an empty subtitle!", QMessageBox::Ok);
        return;
    }
    else if (unoccupiedIndex == -1)
    {
        QMessageBox::warning(this, "Warning", "All characters have been occupied by a color!", QMessageBox::Ok);
        return;
    }

    newColorBlock.m_start = unoccupiedIndex;
    newColorBlock.m_end = unoccupiedIndex;

    // Find where to insert the layout
    int insertAt = 0;
    for (int i = 0; i < m_colorBlocks.size(); i++)
    {
        if (m_colorBlocks[i].m_end < unoccupiedIndex)
        {
            insertAt++;
        }
    }

    m_colorBlocks.insert(insertAt, newColorBlock);
    AddColorBlock(insertAt, newColorBlock);

    UpdateSubtitlePreview();
    SetSubtitleEdited(true);
}

//---------------------------------------------------------------------------
// Editing subtitle
//---------------------------------------------------------------------------
void mstEditor::on_TE_TextEditor_textChanged()
{
    if (!ui->TE_TextEditor->isEnabled()) return;

    bool useSound = ui->CB_Sound->isEnabled() && ui->CB_Sound->isChecked();
    QString currentText = ui->TE_TextEditor->toPlainText();

    if (!m_subtitleHardcoded)
    {
        // Count the number of tags it has before this page
        int pictureTagStart = 0;
        for (int i = 0; i < m_page; i++)
        {
            pictureTagStart += m_subtitles[i].count("$");
        }
        pictureTagStart += (useSound ? 1 : 0);

        // Find the difference of tag count
        QString prevText = m_subtitles[m_page];
        int prevTagCount = prevText.count("$");
        int currentTagCount = currentText.count("$") + (useSound ? 1 : 0);

        // Insert or delete combo boxes
        if (prevTagCount > currentTagCount)
        {
            for (int i = 0; i < (prevTagCount - currentTagCount); i++)
            {
                int index = ui->VL_Tags->layout()->count() - 1;
                m_tags.remove(pictureTagStart + index);
                RemoveButtonComboBox(index);
            }
        }
        else if (currentTagCount > prevTagCount)
        {
            for (int i = 0; i < (currentTagCount - prevTagCount); i++)
            {
                int index = ui->VL_Tags->layout()->count();
                m_tags.insert(pictureTagStart + index, TagPair(m_buttonToString[Button::A], Tag::Picture));
                AddButtonComboBox(index);
            }
        }
    }

    m_subtitles[m_page] = (useSound ? "$" : "") + currentText;

    // If color block range is outside subtitle, remove it
    for (int i = m_colorBlocks.size() - 1; i >= 0; i--)
    {
        if (m_colorBlocks[i].m_end > (m_subtitles[m_page].size() - 1) || m_colorBlocks[i].m_start > (m_subtitles[m_page].size() - 1))
        {
            RemoveColorBlock(i);
        }
    }

    // Update limits of color blocks
    for (int i = 0; i < m_colorBlocks.size(); i++)
    {
        UpdateStartEndLimits(i);
    }

    UpdateSubtitlePreview();
    SetSubtitleEdited(true);
}

//---------------------------------------------------------------------------
// Button combo box changed
//---------------------------------------------------------------------------
void mstEditor::on_CB_ComboBox_currentIndexChanged(int index)
{
    // Count the number of tags it has before this page
    int tagID = 0;
    for (int i = 0; i < m_page; i++)
    {
        tagID += m_subtitles[i].count("$");
    }
    tagID += (ui->CB_Sound->isChecked() ? 1 : 0);

    QComboBox* comboBox = reinterpret_cast<QComboBox*>(sender());
    int tagIndex = -1;
    for (int i = 0; i < ui->VL_Tags->layout()->count(); i++)
    {
        if (ui->VL_Tags->layout()->itemAt(i)->layout()->itemAt(1)->widget() == comboBox)
        {
            tagIndex = i;
            break;
        }
    }

    // tagID found, update tag string
    if (tagIndex != -1)
    {
        tagID += tagIndex;

        m_tags[tagID].first = m_buttonToString[static_cast<Button>(index)];
        UpdateSubtitlePreview();
    }

    SetSubtitleEdited(true);
}

//---------------------------------------------------------------------------
// Use sound or not?
//---------------------------------------------------------------------------
void mstEditor::on_CB_Sound_stateChanged(int state)
{
    if (m_page < 0 || !ui->CB_Sound->isEnabled()) return;

    // Count the number of tags it has before this page
    int tagID = 0;
    for (int i = 0; i < m_page; i++)
    {
        tagID += m_subtitles[i].count("$");
    }

    // Remove or insert tag
    QString& subtitle = m_subtitles[m_page];
    if (state == Qt::Checked)
    {
        subtitle = "$" + subtitle;
        m_tags.insert(tagID, TagPair("", Tag::Sound));
    }
    else
    {
        subtitle = subtitle.mid(1);
        m_tags.remove(tagID);
    }

    // Enable base on state
    ui->LE_Sound->setEnabled(state == Qt::Checked);
    ui->LE_Sound->setText("");

    SetSubtitleEdited(true);
}

//---------------------------------------------------------------------------
// Apply save once if auto apply is checked and text has been edited
//---------------------------------------------------------------------------
void mstEditor::on_CB_AutoApply_clicked(bool checked)
{
    if (checked && m_subtitleEdited)
    {
        on_PB_Save_clicked();
    }
}

//---------------------------------------------------------------------------
// Sound tag has changed by user
//---------------------------------------------------------------------------
void mstEditor::on_LE_Sound_textEdited(const QString &str)
{
    if (m_page < 0 || !ui->LE_Sound->isEnabled()) return;

    // Count the number of tags it has before this page
    int tagID = 0;
    for (int i = 0; i < m_page; i++)
    {
        tagID += m_subtitles[i].count("$");
    }

    // Update tag
    m_tags[tagID].first = str;

    SetSubtitleEdited(true);
}

//---------------------------------------------------------------------------
// Subtitle name has changed by user
//---------------------------------------------------------------------------
void mstEditor::on_LE_SubtitleName_textEdited(const QString &str)
{
    if (m_id < 0 || m_page < 0 || !ui->LE_SubtitleName->isEnabled()) return;

    // Update name
    m_name = str;

    SetSubtitleEdited(true);
}

//---------------------------------------------------------------------------
// Converting string to russian
//---------------------------------------------------------------------------
QString mstEditor::ToRussian(QString const& str)
{
    QString returnStr = str;
    for(QChar& chr : returnStr)
    {
        if (m_unicodeToRussian.contains(chr))
        {
            chr = m_unicodeToRussian[chr];
        }
    }
    return returnStr;
}

//---------------------------------------------------------------------------
// Converting russian back to unicode
//---------------------------------------------------------------------------
QString mstEditor::ToUnicode(QString const& str)
{
    QString returnStr = str;
    for(QChar& chr : returnStr)
    {
        if (m_russianToUnicode.contains(chr))
        {
            chr = m_russianToUnicode[chr];
        }
    }
    return returnStr;
}

//---------------------------------------------------------------------------
// Translate all subtitles to russian in tree view
//---------------------------------------------------------------------------
void mstEditor::on_CB_Russian_clicked(bool checked)
{
    if (checked)
    {
        m_previewLabel->setStyleSheet("font: 26px \"nintendo_NTLG-DB_002\"; color: white;");
    }
    else
    {
        m_previewLabel->setStyleSheet("font: 27px \"FOT-RodinCattleya Pro DB\"; color: white;");
    }

    for(int i = 0; i < ui->TW_TreeWidget->topLevelItemCount(); i++)
    {
        QTreeWidgetItem* item = ui->TW_TreeWidget->topLevelItem(i);
        QString str = item->text(1);
        item->setText(1, checked ? ToRussian(str) : ToUnicode(str));
    }

    bool wasEdited = m_subtitleEdited;

    QString textEdit = ui->TE_TextEditor->toPlainText();
    ui->TE_TextEditor->setText(checked ? ToRussian(textEdit) : ToUnicode(textEdit));

    SetSubtitleEdited(wasEdited);
}

//---------------------------------------------------------------------------
// Save preview as image
//---------------------------------------------------------------------------
void mstEditor::on_PB_SavePreview_clicked()
{
    QString path = "";
    if (!m_pngPath.isEmpty())
    {
        path = m_pngPath;
    }

    if (!ui->LE_SubtitleName->text().isEmpty())
    {
        path += "/" + ui->LE_SubtitleName->text() + ".png";
    }

    QString pngFile = QFileDialog::getSaveFileName(this, tr("Save As .PNG"), path, "PNG File (*.png)");
    if (pngFile == Q_NULLPTR) return;

    QImage bitmap(ui->L_Preview->size(), QImage::Format_ARGB32);
    bitmap.fill(Qt::transparent);
    QPainter painter(&bitmap);
    ui->L_Preview->render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
    bitmap.save(pngFile);

    // Save directory
    QFileInfo info(pngFile);
    m_pngPath = info.dir().absolutePath();
}

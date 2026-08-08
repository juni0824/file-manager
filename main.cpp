// library includes
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <filesystem>
#include <sys/time.h>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

// namespace for filesystem
namespace fs = std::filesystem;

// application class
class MyApp : public wxApp {
public:
// override base class virtuals
    bool OnInit() override;
};

// main frame class
class MyFrame : public wxFrame {
private:
    // Define custom IDs for menu items
    enum {
        ID_RENAME = wxID_HIGHEST + 1,
        ID_NEW_DIR,
        ID_DELETE_FILE,
        ID_REFRESH_FILES
    };
    // UI components
public:
    // Constructor
    MyFrame() : wxFrame(nullptr, wxID_ANY, "File Manager", wxDefaultPosition, wxDefaultSize) {
        wxMenu* fileMenu = new wxMenu;
        fileMenu->Append(wxID_EXIT, "E&xit\tCtrl-Q", "Exit the application");
       
        // Set up menu bar
        wxMenuBar* menuBar = new wxMenuBar;
        SetMenuBar(menuBar);

        // Bind exit event
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { Close(true); }, wxID_EXIT);

        // Set up status bar
        CreateStatusBar();
        SetStatusText("Welcome");

        // Main panel and sizer
        wxPanel* panel = new wxPanel(this);
        wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
        panel->SetSizer(sizer);

        // Create directory bar to search
        dirBar = new wxTextCtrl(panel, wxID_ANY, defaultDir,
                                            wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
        sizer->Add(dirBar, 0, wxEXPAND | wxALL, 5);

        // Bind enter key event for directory bar
        Bind(wxEVT_TEXT_ENTER, [this](wxCommandEvent& event) {
            wxString path = event.GetString();

            // Check if directory exists
            if (fs::exists(std::string(path.mb_str())) && 
                fs::is_directory(std::string(path.mb_str()))) {
                
                // Update default directory and file list
                defaultDir = std::string(path.mb_str());
                updateFile();
                SetStatusText("Directory exists: " + path);
            }
            // if directory does not exist
            else {
                SetStatusText("Directory does not exist: " + path);
            }
        }, dirBar->GetId());

        // Create file list control
        fileList = new wxListCtrl(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                wxLC_REPORT | wxLC_SINGLE_SEL);
        sizer->Add(fileList, 1, wxEXPAND | wxALL, 5);
        // Set up columns for file information
        fileList->InsertColumn(0, "Name", wxLIST_FORMAT_LEFT, 300);
        fileList->InsertColumn(1, "Size", wxLIST_FORMAT_RIGHT, 100);
        fileList->InsertColumn(2, "Type", wxLIST_FORMAT_LEFT, 150);
        fileList->InsertColumn(3, "Modified", wxLIST_FORMAT_RIGHT, 200);


        // Bind double-click event to open files/directories
        fileList->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent& event){
            long i = event.GetIndex();
            std::string path = defaultDir + "/" + std::string(fileList->GetItemText(i));
            // Check if it's a directory
            if(fs::is_directory(path)) {
                defaultDir = path;
                updateFile();
                dirBar->SetValue(wxString(defaultDir));
            }
            // else open file with default application
            else {
                selectedItem();
                wxLaunchDefaultApplication(path);
            }
        });

        // Set up Edit menu and shortcuts
        wxMenu* editMenu = new wxMenu;
        editMenu->Append(wxID_COPY, "&Copy\tCtrl-C");
        editMenu->Append(wxID_CUT, "Cu&t\tCtrl-X");
        editMenu->Append(wxID_PASTE, "&Paste\tCtrl-V");

        // Add Edit menu to menu bar, copy, cut, paste bindings
        menuBar->Append(editMenu, "&Edit");
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { copyItem(); }, wxID_COPY);
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { cutItem(); }, wxID_CUT);
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { pasteItem(); }, wxID_PASTE);

        // Set up Files menu with rename, new directory, delete, refresh
        wxMenu* filesMenu = new wxMenu;
        filesMenu->Append(ID_RENAME, "&Rename");
        filesMenu->Append(ID_NEW_DIR, "&New Directory");
        filesMenu->Append(ID_DELETE_FILE, "&Delete");
        filesMenu->Append(ID_REFRESH_FILES, "&Refresh");
        
        // Add Files menu to menu bar and bind events
        menuBar->Append(filesMenu, "&Files");
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { renameItem(); }, ID_RENAME);
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { newDir(); }, ID_NEW_DIR);
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { deleteItem(); }, ID_DELETE_FILE);
        Bind(wxEVT_MENU, [this](wxCommandEvent&) { refresh(); }, ID_REFRESH_FILES);

        // Initial file list update
        updateFile();
    }

    // Helper function to update file list based on current directory
    private:
    void updateFile() {
        fileList->DeleteAllItems();

        // Iterate through directory entries
        for (const auto& entry : fs::directory_iterator(defaultDir)) {

            // Get file/directory name
            std::string name = entry.path().filename().string();
            long i = fileList->InsertItem(fileList->GetItemCount(), name);

            std::error_code error;

            // Get file status to determine type and size
            auto status = entry.symlink_status(error);
            // if error then set unknowns
            if (error) {
                fileList->SetItem(i, 1, "?");
                fileList->SetItem(i, 2, "Unknown");
                continue;
            }

            // Check if it's a symlink
            if (fs::is_symlink(status)) {
                fileList->SetItem(i, 1, "?");
                fileList->SetItem(i, 2, "Link");
            }

            bool isDir = fs::is_directory(entry.path(), error);
            if (error) {
                fileList->SetItem(i, 1, "?");
                fileList->SetItem(i, 2, "Unknown");
                continue;
            }

            // Set size and type based on whether it's a directory or file
            if (isDir) {
                fileList->SetItem(i, 1, "0");
                fileList->SetItem(i, 2, "Dir");
            }
            else {
                auto size = fs::file_size(entry.path(), error);
                if (error) {
                    fileList->SetItem(i, 1, "?");
                } 
                else {
                    fileList->SetItem(i, 1, std::to_string(size));
                }

                std::string ext = entry.path().extension().string();
                fileList->SetItem(i, 2, ext.empty() ? "File" : ext);
            }

            // Get and format last modified time
            auto modified = entry.last_write_time(error);
            if(error){
                fileList->SetItem(i, 3, "?");
            }
            // convert file_time_type to system_clock::time_point
            else{
                auto sys_clock = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    modified - fs::file_time_type::clock::now()
                    + std::chrono::system_clock::now()
                );
                // Format time to string
                std::time_t time = std::chrono::system_clock::to_time_t(sys_clock);
                std::tm* tm = std::localtime(&time);
                std::ostringstream ss;
                ss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
                std::string modified = ss.str();
                fileList->SetItem(i, 3, modified);
            }
        }
    }

    // helper function to get selected item path
    private:
        std::string selectedItem(){
            // Get selected item index
            long selected = fileList->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            // if no item selected
            if (selected == -1) {
                wxMessageBox("Select an item first");
                return "";
            }
            // Get item name and construct full path
            wxString name = fileList->GetItemText(selected, 0);
            std::string selectedPath = defaultDir + "/" + std::string(name.mb_str());
            return selectedPath;
        }

    // copy item to clipboard
    private:
        void copyItem(){
            // Get selected item path
            std::string path = selectedItem();
            // check if path is empty
            if(path.empty()){
                return;
            }
            // set clipboard and isCut flag
            clipboard = path;
            isCut = false;
            SetStatusText("Copied: " + wxString(clipboard), 0);
        }

    // cut item to clipboard
    private:
        void cutItem(){
            // Get selected item path
            std::string path = selectedItem();
            // check if path is empty
            if(path.empty()){
                return;
            }
            // set clipboard and isCut flag
            clipboard = path;
            isCut = true;
            SetStatusText("Cut: " + wxString(clipboard));
        }

    // paste item from clipboard
    private:
        void pasteItem(){
            // check if clipboard is empty
            if(clipboard.empty()){
                SetStatusText("Clipboard is empty");
                return;
            }
            // construct full path for paste destination
            std::string fullPath = defaultDir + "/" + fs::path(clipboard).filename().string();
            // overwrite confirmation
            if(fs::exists(fullPath)){
                if(wxMessageBox("File exists. Overwrite?", "Confirm", wxYES_NO | wxICON_QUESTION) != wxYES){
                    return;
                }
                // remove existing file or directory
                else{
                    std::error_code error;
                    fs::remove_all(fullPath, error);
                    if(error){
                        SetStatusText("Error overwriting file: " + wxString(error.message()));
                        return;
                    }
                }
            }
            // if is cut then move file
            if(isCut){
                // prevent moving to same location
                if (fs::equivalent(clipboard, fullPath)) {
                    SetStatusText("error, item already in this location");
                    return;
                }

                // move file or directory
                fs::rename(clipboard, fullPath);
                SetStatusText("Moved to: " + wxString(fullPath));
            }
            // else copy file or directory
            else {
                // if directory copy recursively
                if(fs::is_directory(clipboard)){
                    fs::copy(clipboard, fullPath, fs::copy_options::recursive);
                }
                // else copy file
                else{
                    fs::copy(clipboard, fullPath);
                }
            }
            // clear clipboard and update file list
            clipboard.clear();
            isCut = false;
            updateFile();
            SetStatusText("Pasted: " + wxString(fullPath));
        }

    // rename selected item
    private:
        void renameItem(){
            // Get selected item path
            std::string path = selectedItem();
            // check if path is empty
            if(path.empty()){
                return;
            }
            // show text entry dialog for new name
            wxTextEntryDialog text(this, "Enter new name:", "Rename"
                                , fs::path(path).filename().string());
            // on OK, perform rename
            if (text.ShowModal() == wxID_OK) {
                // construct new path
                std::string newPath = defaultDir + "/" + std::string(text.GetValue().mb_str());
                // check if new path already exists
                if (fs::exists(newPath)) {
                    wxMessageBox("A file with this name already exists.", "Error", wxICON_ERROR);
                    return;
                }
                // perform rename and update file list
                fs::rename(path, newPath);
                updateFile();
                SetStatusText("Renamed to: " + wxString(text.GetValue()));
            }
        }

    // create new directory
    private:
        void newDir(){
            // show text entry dialog for directory name
            wxTextEntryDialog text(this, "Enter directory name: ", "New Directory");
            // on OK, create directory, if text is not empty
            if(text.ShowModal() == wxID_OK){
                // create new directory path
                std::string newDirPath = defaultDir + "/" + std::string(text.GetValue().mb_str());
                // check if directory already exists
                if(fs::exists(newDirPath)){
                    wxMessageBox("Directory already exists.", "Error", wxICON_ERROR);
                    return;
                }
                // else create directory and update file list
                fs::create_directory(newDirPath);
                updateFile();
                SetStatusText("Created directory: " + wxString(newDirPath));
            }
        }

    // delete selected item
    private:
        void deleteItem(){
            // Get selected item path
            std::string path = selectedItem();
            // check if path is empty
            if(path.empty()){
                return;
            }
            // if confirmed, delete file or directory
            if(wxMessageBox("Are you sure you want to delete this item?", "Yes No", wxYES_NO | wxICON_QUESTION) == wxYES){
                // remove file or directory recursively
                std::error_code error;
                fs::remove_all(path, error);
                // if error occurs, show message
                if(error){
                    wxMessageBox("Error deleting item: " + wxString(error.message()), "Error", wxICON_ERROR);
                    return;
                }
                // update file list
                updateFile();
                SetStatusText("Deleted: " + wxString(path));
            }

        }
    
    // refresh file list
    private:
        void refresh(){
            updateFile();
            SetStatusText("Refreshed");
        }

    // exit application
    private:
        void Exit(wxCommandEvent& event) {
            Close(true);
        }

    // member variables
    private:
        std::string defaultDir = wxGetHomeDir().ToStdString();
        wxListCtrl* fileList = nullptr;
        wxTextCtrl* dirBar = nullptr;
        std::string clipboard;
        bool isCut;
};

// implement application
wxIMPLEMENT_APP(MyApp);

// application initialization
bool MyApp::OnInit() {
    MyFrame* frame = new MyFrame();
    frame->Show(true);
    return true;
}

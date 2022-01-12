#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <stdint.h>
#include <fstream>

using namespace std;
using namespace std::filesystem;

vector <directory_entry> GetFiles() 
{
    path CurrentPath = current_path();
    vector<directory_entry> FileNames;

    for (const auto& file : directory_iterator(CurrentPath)) 
    {
        path FileName = path(file).filename();
        FileNames.push_back(file);
    }

    return FileNames;
}

bool Is_vb0(directory_entry& File) 
{
    string FileName = path(File).filename().string();
    auto FileNameLenght = FileName.length();

    for (auto i = 0; i < FileNameLenght; i++) 
    {
        if (FileName[i] == '-') 
        {
            if (FileName[i+1] == 'i')
                return false;

            if (FileName.substr(i+1, 3) == "vb0")
                return true;
        }
    }
    cout << path(File).filename().string() << " not valid for merging" << endl;
    return false;
}

bool Merge(directory_entry& vb0_File, directory_entry& vb2_File) 
{
    // Add Stride
    return true;
}

int main()
{
    std::cout << "Hello World!\n";

    auto Files = GetFiles();

    if (Files.size() < 1) 
    {
        cout << "Error: No files in the current folder" << endl;
        Sleep(30000);
        exit(0);
    }     
    
    for (auto i = 0; i < Files.size(); i++) 
    {
        if (Is_vb0(Files[i]) && i+1 < Files.size()) 
        {
            if (Merge(Files[i], Files[i+1])) 
            {
                cout << path(Files[i]).filename() << " and " << path(Files[i+1]).filename() << " merged successfully!" << endl;
                i++;
            }
            else 
            {
                cout << "Unable to merge " << path(Files[i]).filename() << " and " << path(Files[i+1]).filename() << endl;
            }
        }
    }

        
    Sleep(30000);
}
#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <stdint.h>
#include <fstream>
using namespace std;
using namespace std::filesystem;

int32_t INDEXTOSKIP = 7;
vector<string> Attributes { "NORMAL", "TANGENT", "POSITION", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BLENDINDICES", "BLENDINDICES", "BLENDWEIGHT", "BLENDWEIGHT" };
int32_t vb0CurrentLine, vb0TotalLines, vb2CurrentLine, vb2TotalLines;
fstream vb0Stream, vb2Stream;

namespace Indexes
{
    using attidx = pair<string, int32_t>;
    vector<attidx> AttributesIndexes;

    void SetupAttributesIndexes()
    {
        for (const auto Attribute : Attributes)
        {
            bool bAlreadyExists = false;

            for (const auto AttributeIndex : AttributesIndexes)
            {
                if (AttributeIndex.first == Attribute)
                {
                    bAlreadyExists = true;
                }
            }

            if (!bAlreadyExists)
            {
                AttributesIndexes.push_back(make_pair(Attribute, 0));
            }
        }
    }

    int32_t FindAndIncrementIndex(string& Attribute)
    {
        for (auto& x : AttributesIndexes)
        {
            if (x.first == Attribute)
            {
                int32_t Index = x.second;
                x.second++;
                return Index;
            }
        }
    }

    void Reset()
    {
        for (auto& x : AttributesIndexes)
        {
            x.second = 0;
        }
    }
}

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

bool Is_A(directory_entry& File, string Type)
{
    string FileName = path(File).filename().string(); 

    if (FileName.find(Type) != string::npos)
    {
        return true;
    }
    else
    {
        return false;
    }
}

int32_t GetLinesNum(directory_entry File)
{
    string Line;
    int32_t Count = 0;
    fstream Stream;
    Stream.open(File);
    while (getline(Stream, Line))
    {
        Count++;
    }
    Stream.close();
    return Count;
}

void LoopVertexData(FILE* NewFile, fstream& Stream, int32_t& RemainingLines, int32_t& CurrentAttributeIndex, int32_t& CurrentOffset, bool bIs_v0)
{
    string Line;

    if (RemainingLines > 1)
    {
        while (getline(Stream, Line))
        {
            RemainingLines--;

            if (Line.empty())
                break;

            int32_t IndexLenght = Line.find("]") - 4;
            int32_t OffsetStart = Line.find("+") + 1;
            CurrentOffset = bIs_v0 ? stoi(Line.substr(OffsetStart, 3)) : CurrentOffset + 4;
            int32_t IndexToSet = Indexes::FindAndIncrementIndex(Attributes[CurrentAttributeIndex]);
            string Tail = Line.substr(Line.find(":"), Line.length());

            if (IndexToSet == 0)
            {
                fprintf(NewFile, "vb0[%s]+%03i %s%s\n", Line.substr(4, IndexLenght).c_str(), CurrentOffset, Attributes[CurrentAttributeIndex].c_str(), Tail.c_str());
            }
            else
            {
                fprintf(NewFile, "vb0[%s]+%03i %s%i%s\n", Line.substr(4, IndexLenght).c_str(), CurrentOffset, Attributes[CurrentAttributeIndex].c_str(), IndexToSet, Tail.c_str());
            }

            CurrentAttributeIndex++;
        }
    }
}

void MergeVertexData(FILE* NewFile)
{
    int32_t vb0RemainingLines = vb0TotalLines - vb0CurrentLine - 1, vb2RemainingLines = vb2TotalLines - vb2CurrentLine - 1, CurrentAttributeIndex = 0, CurrentOffset = 0;

    while (vb0RemainingLines > 1 || vb2RemainingLines > 1)
    {
        Indexes::Reset();
        LoopVertexData(NewFile, vb0Stream, vb0RemainingLines, CurrentAttributeIndex, CurrentOffset, true);
        LoopVertexData(NewFile, vb2Stream, vb2RemainingLines, CurrentAttributeIndex, CurrentOffset, false);
        CurrentAttributeIndex = 0;
        fprintf(NewFile, "\n");
    }
}

void MergeHeader(FILE* NewFile)
{
    int32_t FirstChunkEndLine = 0, CurrentElementIndex = 0, MaxOffset = 0, Count = 0;
    string vb0Line, vb2line;

    while (getline(vb0Stream, vb0Line))
    {
        getline(vb2Stream, vb2line);
        vb0CurrentLine++; vb2CurrentLine++;

        if (vb0Line.find("stride:") != string::npos)
        {
            int32_t StrideSum = stoi(vb0Line.substr(8, 2)) + stoi(vb2line.substr(8, 2));
            fprintf(NewFile, "stride: %i\n", StrideSum);
            continue;
        }

        if (vb0Line.find(string("element")) != string::npos)
        {
            if (stoi(vb0Line.substr(8, 1)) == INDEXTOSKIP)
            {
                for (int32_t i = 0; i < 8; i++)
                {
                    getline(vb0Stream, vb0Line);
                    getline(vb2Stream, vb2line);
                    vb0CurrentLine++; vb2CurrentLine++;
                }
                fprintf(NewFile, "element[%i]:\n", CurrentElementIndex);
                continue;
            }

            fprintf(NewFile, "element[%i]:\n", CurrentElementIndex);
            continue;
        }

        if (vb0Line.find(string("SemanticName")) != string::npos)
        {
            string AttributeToSet = Attributes[CurrentElementIndex];
            fprintf(NewFile, "  SemanticName: %s\n", AttributeToSet.c_str());
            continue;
        }

        if (vb0Line.find(string("SemanticIndex")) != string::npos)
        {
            string CurrentAttribute = Attributes[CurrentElementIndex];
            int32_t IdxToSet = Indexes::FindAndIncrementIndex(CurrentAttribute);
            fprintf(NewFile, "  SemanticIndex: %i\n", IdxToSet);
            continue;
        }

        if (vb0Line.find(string("InputSlot:")) != string::npos)
        {
            fprintf(NewFile, "  InputSlot: 0\n");
            continue;
        }

        if (vb0Line.find(string("AlignedByteOffset")) != string::npos)
        {
            int32_t CurrentByteOff;
            if (CurrentElementIndex > INDEXTOSKIP - 1)
            {
                MaxOffset += 4;
                CurrentByteOff = MaxOffset;
            }
            else
            {
                CurrentByteOff = stoi(vb0Line.substr(21, vb0Line.length()));
                MaxOffset = MaxOffset >= CurrentByteOff ? MaxOffset : CurrentByteOff;
            }
            CurrentElementIndex++;
            fprintf(NewFile, "  AlignedByteOffset: %i\n", CurrentByteOff);
            continue;
        }

        if (vb0Line.find(string("vertex-data")) != string::npos)
        {
            FirstChunkEndLine = Count;
            fprintf(NewFile, "%s\n", vb0Line.c_str());
            break;
        }

        fprintf(NewFile, "%s\n", vb0Line.c_str());
    }

    vb0CurrentLine++; vb2CurrentLine++;
}

bool Merge(directory_entry& vb0_File, directory_entry& vb2_File)
{
    // Create File
    FILE* NewFile = NULL;
    create_directory("Merged");
    string NewFilePath = "Merged/" + path(vb0_File).filename().string();
    fopen_s(&NewFile, NewFilePath.c_str(), "w+");

    // Open Streams
    vb0Stream.open(vb0_File);
    vb2Stream.open(vb2_File);

    // Get Total Lines
    vb0TotalLines = GetLinesNum(vb0_File);
    vb2TotalLines = GetLinesNum(vb2_File);

    // Merge 
    MergeHeader(NewFile);
    MergeVertexData(NewFile);
    
    // Close File and Streams
    fclose(NewFile);
    vb0Stream.close();
    vb2Stream.close();
    return true;
}

void Setup(directory_entry SetupFile)
{
    fstream SetupStream;
    string Line;
    int32_t Index = 0;
    bool bAttributes = false;
    SetupStream.open(SetupFile);
    Attributes.empty();

    while (getline(SetupStream, Line))
    {
        if (bAttributes)
        {
            Attributes[Index] = Line;
            Index++;
        }

        if (Line.find("INDEXTOSKIP") != string::npos)
        {
            INDEXTOSKIP = stoi(Line.substr(12, 1));
        }

        if (Line.find("ATTRIBUTES") != string::npos)
        {
            bAttributes = true;
        }
    }

    SetupStream.close();
}

int main()
{ 
    vector<directory_entry> Files = GetFiles();
    bool bSetup = false;

    for (auto File : Files)
    {
        if (Is_A(File, "setup") || Is_A(File, "Setup"))
        {
            Setup(File);
            bSetup = true;
            break;
        }
    }

    if (!bSetup)
    {
        cout << "Setup.txt not found, using defaults." << endl;
    }

    Indexes::SetupAttributesIndexes();

    for (auto i = 0; i < Files.size(); i++)
    {
        if (Is_A(Files[i], "vb0") && Is_A(Files[i + 1], "vb2"))
        {
            if (Merge(Files[i], Files[i + 1]))
            {
                cout << path(Files[i]).filename() << " and " << path(Files[i + 1]).filename() << " merged successfully!" << endl; 
                Indexes::Reset();
                vb0CurrentLine = 0; vb0TotalLines = 0; vb2CurrentLine = 0; vb2TotalLines = 0;
                i++;
            }
            else
            {
                cout << "Unable to merge " << path(Files[i]).filename() << " and " << path(Files[i + 1]).filename() << endl;
            }
        }
    }

    cout << "Exiting..." << endl;
    Sleep(30000);
}
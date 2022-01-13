#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <stdint.h>
#include <fstream>

using namespace std;
using namespace std::filesystem;
using StringIndexes = pair<string, int32_t>;
int32_t vb0TotalLines, vb2TotalLines;
fstream vb0Stream, vb2Stream;
StringIndexes AttSemIdx[] = { make_pair("NORMAL", 0), make_pair("TANGENT", 0), make_pair("POSITION", 0), make_pair("TEXCOORD", 0), make_pair("BLENDINDICES", 0), make_pair("BLENDWEIGHT", 0) };
vector<directory_entry> Files;
const string _Stride {"stride: "};
const string _VertexData { "vertex-data" };
const string _Element { "element[" };
const string _SemName { "  SemanticName: " };
const string _SemIdx { "  SemanticIndex: " };
const string _InputSlot { "  InputSlot: " };
const string _ByteOff { "  AlignedByteOffset: " };
const string Attributes[] {"NORMAL", "TANGENT", "POSITION", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "_delete_", "BLENDINDICES", "BLENDINDICES", "BLENDWEIGHT", "BLENDWEIGHT"};

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
    auto FileNameLenght = FileName.length();

    for (auto i = 0; i < FileNameLenght; i++)
    {
        if (FileName[i] == '-')
        {
            if (FileName.substr(i + 1, 3) == Type)
                return true;
        }
    }
    return false;
}

int32_t GetIntValueFromString(string& Line, const string& StringToCut)
{
    int32_t Value;
    int32_t ValueLineLenght = Line.length();
    string ValueString = Line.substr(StringToCut.length(), ValueLineLenght - StringToCut.length());
    stringstream Stream(ValueString);
    Stream >> Value;
    return Value;
}

string GetMergedStrideLine(string vb0StrideLine, string vb2StrideLine)
{
    int32_t vb0Stride = GetIntValueFromString(vb0StrideLine, _Stride);
    int32_t vb2Stride = GetIntValueFromString(vb2StrideLine, _Stride);
    int32_t SumStride = vb0Stride + vb2Stride;
    return _Stride + to_string(SumStride);
}

int32_t GetElementIndex(string& ElementIndexLine)
{
    string ElementIndexString = ElementIndexLine.substr(_Element.length(), ElementIndexLine.size() - 1);
    return stoi(ElementIndexString);
}

int32_t MergeFirstPart(FILE* NewFile)
{
    int32_t FirstChunkEndLine = 0,CurrentElementIndex = 0, TempIdx = 0, MaxOffset = 0, Count = 0;
    string vb0Line, vb2line;
    
    while (getline(vb0Stream, vb0Line))
    {
        getline(vb2Stream, vb2line);
        Count++;

        if (vb0Line.find(string(_VertexData)) != string::npos)
        {
            FirstChunkEndLine = Count;
            fprintf(NewFile, "%s\n", vb0Line.c_str());
            break;
        }

        if (vb0Line.find(string(_Stride)) != string::npos)
        {
            string MergedStrideLine = GetMergedStrideLine(vb0Line, vb2line);
            fprintf(NewFile, "%s\n", MergedStrideLine.c_str());
            continue;
        }

        if (vb0Line.find(string(_Element)) != string::npos)
        {
            TempIdx = GetElementIndex(vb0Line);
            CurrentElementIndex = GetElementIndex(vb0Line);
            if (CurrentElementIndex == 7)
                continue;
            if (CurrentElementIndex > 7)
                TempIdx--;
            fprintf(NewFile, "%s%i]\n", _Element.c_str(), TempIdx);
            continue;
        }

        if (CurrentElementIndex == 7)
            continue;

        if (vb0Line.find(string(_SemName)) != string::npos)
        {
            string AttributeToSet = Attributes[CurrentElementIndex];
            fprintf(NewFile, "%s%s\n", _SemName.c_str(), AttributeToSet.c_str());
            continue;
        }

        if (vb0Line.find(string(_SemIdx)) != string::npos)
        {
            int32_t IdxToSet = 0;
            string CurrentAttribute = Attributes[CurrentElementIndex];
            for (auto& x : AttSemIdx)
            {
                if (x.first == CurrentAttribute)
                {
                    IdxToSet = x.second;
                    x.second++;
                }   
            }
            fprintf(NewFile, "%s%i\n", _SemIdx.c_str(), IdxToSet);
            continue;
        }

        if (vb0Line.find(string(_InputSlot)) != string::npos)
        {
            fprintf(NewFile, "%s0\n", _InputSlot.c_str());
            continue;
        }

        if (vb0Line.find(string(_ByteOff)) != string::npos)
        {
            int32_t CurrentByteOff;
            if (CurrentElementIndex > 7)
            {
                MaxOffset += 4;
                CurrentByteOff = MaxOffset;
            }
            else
            {
                CurrentByteOff = GetIntValueFromString(vb0Line, _ByteOff);
                MaxOffset = MaxOffset >= CurrentByteOff ? MaxOffset : CurrentByteOff;
            }
            fprintf(NewFile, "%s%i\n", _ByteOff.c_str(), CurrentByteOff);
            continue;
        }

        fprintf(NewFile, "%s\n", vb0Line.c_str());
    }
    return FirstChunkEndLine + 1;
}

bool MergeSecondPart(FILE* NewFile, int32_t LineToStart)
{
    string vb0Line, vb2Line;
    int32_t vb0CurrentLine = LineToStart, vb2CurrentLine = LineToStart, vb0RemainingLines = vb0TotalLines - LineToStart - 1, vb2RemainingLines = vb2TotalLines - LineToStart - 1, CurrentIndex = 0, CurrentOffset = 0, CurrentAttributeIndex = 0;

    while (vb0RemainingLines > 1 || vb2RemainingLines > 1)
    {
        for (auto& x : AttSemIdx)
        {
            x.second = 0;
        }

        if (vb0RemainingLines > 1)
        {
            while (getline(vb0Stream, vb0Line))
            {
                vb0CurrentLine++;
                vb0RemainingLines--;

                if (vb0Line.empty())
                {
                    break;
                }

                int32_t StartElementNameIndex = vb0Line.find("ATTRIBUTE");
                int32_t IndexAfterElementName = vb0Line.find(": ");
                int32_t IdxToSet = 0;
                for (auto& x : AttSemIdx)
                {
                    if (x.first == Attributes[CurrentAttributeIndex])
                    {
                        IdxToSet = x.second;
                        x.second++;
                    }
                }

                int32_t StartOffsetIndex = vb0Line.find("+") + 1;
                int32_t EndOffsetIndex = StartElementNameIndex - 1;
                CurrentOffset = stoi(vb0Line.substr(StartOffsetIndex, EndOffsetIndex));

                if (IdxToSet == 0)
                {
                    fprintf(NewFile, "%s%s:%s\n", vb0Line.substr(0, StartElementNameIndex).c_str(), Attributes[CurrentAttributeIndex].c_str(), vb0Line.substr(IndexAfterElementName + 1, vb0Line.length() - 1).c_str());
                }
                else
                {
                    fprintf(NewFile, "%s%s%i:%s\n", vb0Line.substr(0, StartElementNameIndex).c_str(), Attributes[CurrentAttributeIndex].c_str(), IdxToSet, vb0Line.substr(IndexAfterElementName + 1, vb0Line.length() - 1).c_str());
                }

                CurrentAttributeIndex++;

                if (Attributes[CurrentAttributeIndex] == "_delete_")
                    CurrentAttributeIndex++;
            }
        }

        if (vb2RemainingLines > 1)
        {
            while (getline(vb2Stream, vb2Line))
            {
                vb2CurrentLine++;
                vb2RemainingLines--;

                if (vb2Line.empty())
                {
                    break;
                }

                int32_t StartElementNameIndex = vb2Line.find("ATTRIBUTE");
                int32_t IndexAfterElementName = vb2Line.find(": ");
                int32_t IdxToSet = 0;
                for (auto& x : AttSemIdx)
                {
                    if (x.first == Attributes[CurrentAttributeIndex])
                    {
                        IdxToSet = x.second;
                        x.second++;
                    }
                }

                int32_t StartOffsetIndex = vb2Line.find("+") + 1;
                int32_t EndOffsetIndex = StartElementNameIndex - 1;
                CurrentOffset += 4;

                if (IdxToSet == 0)
                {
                    fprintf(NewFile, "vb0%s%03i %s:%s\n", vb2Line.substr(3, StartOffsetIndex - 3).c_str(), CurrentOffset, Attributes[CurrentAttributeIndex].c_str(),
                        vb2Line.substr(IndexAfterElementName + 1, vb2Line.length() - 1).c_str());
                }
                else
                {
                    fprintf(NewFile, "vb0%s%03i %s%i:%s\n", vb2Line.substr(3, StartOffsetIndex - 3).c_str(), CurrentOffset, Attributes[CurrentAttributeIndex].c_str(), IdxToSet,
                        vb2Line.substr(IndexAfterElementName + 1, vb2Line.length() - 1).c_str());
                }

                CurrentAttributeIndex++;

                if (Attributes[CurrentAttributeIndex] == "_delete_")
                    CurrentAttributeIndex++;
            }
            CurrentAttributeIndex = 0;
            fprintf(NewFile, "\n");
        }
    }

    fclose(NewFile);
    return true;
}

int32_t GetLinesNum(fstream& File)
{
    string Line;
    int32_t Count = 0;
    while (getline(File, Line))
    {
        Count++;
    }
    return Count;
}

bool Merge(directory_entry& vb0_File, directory_entry& vb2_File)
{
    FILE* NewFile = NULL;
    create_directory("Merged");
    string NewFilePath = "Merged/" + path(vb0_File).filename().string();
    fopen_s(&NewFile, NewFilePath.c_str(), "w+");
    
    vb0Stream.open(vb0_File);
    vb2Stream.open(vb2_File);
    fstream vb0Clone;
    fstream vb2Clone;
    vb0Clone.open(vb0_File);
    vb2Clone.open(vb2_File);
    vb0TotalLines = GetLinesNum(vb0Clone);
    vb2TotalLines = GetLinesNum(vb2Clone);
    vb0Clone.close();
    vb2Clone.close();
    
    int32_t LineStartSecondPart = MergeFirstPart(NewFile);
    
    if (MergeSecondPart(NewFile, LineStartSecondPart))
    {
        vb0Stream.close();
        vb2Stream.close();
        return true;
    }
    else
    {
        return false;
    }
    
    return true;
}

void ResetGlobals()
{
    for (auto& x : AttSemIdx)
    {
        x.second = 0;
    }
}

int main()
{ 
    Files = GetFiles();

    for (auto i = 0; i < Files.size(); i++)
    {
        if (Is_A(Files[i], "vb0") && Is_A(Files[i + 1], "vb2"))
        {
            if (Merge(Files[i], Files[i + 1]))
            {
                cout << path(Files[i]).filename() << " and " << path(Files[i + 1]).filename() << " merged successfully!" << endl;
                ResetGlobals();
                i++;
            }
            else
            {
                cout << "Unable to merge " << path(Files[i]).filename() << " and " << path(Files[i + 1]).filename() << endl;
            }
        }
    }
    cout << "Exiting..." << endl;
    Sleep(3000);
}
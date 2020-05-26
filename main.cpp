#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QDirIterator>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <zlib.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#define DISP_ID_FIND "#define eDispId_"
#define FIDL_ID_FIND "eDispId_RENAME_"
#define FIDL_ID_FIND_CUSTOM "eDispId_RENAME_Custom_"
#define GROUP_STR "/** Group"
#define GROUP_STR_END " **/"
#define GROUP_STR_FREFIX "McuServiceType::GroupLevel::eDispId_Group_"
#define ENUM_STR_FREFIX "McuServiceType::EnumEventID::"

#define IS_LOG_ENABLE 0
using namespace std;
// compare
// ./DisplayGen -c outputFile image_new.qrc image_old.qrc

struct stDispID {
    int mobis_ID;
    int ivis_ID;
    int grou_label;
};

void fileRead(ifstream& fin, vector<string>& v)
{
    string line;

    while (getline(fin, line))
        v.push_back(line);
}

void serarchGroup(vector<string>& v, map<int, string>& out)
{

    map<int, string> groupMap; // line, groupName
    for (unsigned long i = 0; i < v.size(); i++) {
        /////////////
        /// group ///
        /////////////
        ///

        string word = GROUP_STR;
        int indexGroup = v[i].find(word);
        if (indexGroup != -1) {
            string tempStr = v[i].substr(word.length(), v[i].length() - word.length());

            int endPos = tempStr.find(GROUP_STR_END);
            string tempStr2 = tempStr.substr(0, endPos);
            tempStr2.erase(remove(tempStr2.begin(), tempStr2.end(), ' '), tempStr2.end());

            int replaceIndex = -1;
            replaceIndex = tempStr2.find("-");
            if (replaceIndex != -1) {
                tempStr2.replace(replaceIndex, 1, "_");
            }

            //            cout << "line [" << i << "] value [" << tempStr2 << "] " << endl;
            out[i] = GROUP_STR_FREFIX + tempStr2;
        }
    }
}

void search3_2(vector<string>& v, string& word, map<string, pair<string, string>>& out)
{
    map<int, string> groupMap;
    serarchGroup(v, groupMap);
    //    cout << "serch word [" << word << "] << endl";
    for (unsigned long i = 0; i < v.size(); i++) {
        int index = v[i].find(word);
        if (index != -1) {
            string temp = v[i].substr(word.length(), v[i].length() - word.length());
            int endPos = temp.find("(");

            //            string valueStr = v[i].substr(index + 8, v[i].length() - word.length());

            int valueIndex = v[i].find("eDispId_");
            string valueStr = v[i].substr(valueIndex, v[i].length() - word.length());
            valueStr.erase(remove(valueStr.begin(), valueStr.end(), ' '), valueStr.end());

            string keyStr = temp.substr(0, endPos);
            keyStr.erase(remove(keyStr.begin(), keyStr.end(), ' '), keyStr.end());

            //            cout << "keyStr : " << keyStr << "valueStr : " << valueStr << endl;

            map<int, string>::iterator it;

            pair<string, string> pairData; //IVIS, group

            string preGroup = "McuServiceType::GroupLevel::eDispId_Group_0";

            for (it = groupMap.begin(); it != groupMap.end(); it++) {
                if ((it->first) > i) {
                    //test code

                    pairData = pair<string, string>(valueStr, preGroup);

                    break;
                }

                if (it == --groupMap.end()) {
                    pairData = pair<string, string>(valueStr, preGroup);
                    //                    cout << "Max Value valueStr : " << valueStr << "preGroup : " << preGroup << endl;
                }

                preGroup = it->second;
            }

            out[keyStr] = pairData;
        }
    }
}
void qrcImageNameListUp(vector<string>& v, string& word, map<string, pair<string, int>>& imageNameInfo)
{
    map<string, int> testMap;
    for (unsigned long i = 0; i < v.size(); i++) {
        int index = v[i].find(word);
        if (index != -1) {
            string temp = v[i].substr(0, v[i].length() - word.length());
            int index2 = temp.find_last_of("/");
            if (index2 != -1) {
                string temp2 = temp.substr(index2 + 1, temp.length() - index2);

                if (imageNameInfo.end() != imageNameInfo.find(temp2)) {
                    imageNameInfo[temp2] = pair<string, int>(imageNameInfo[temp2].first + v[i], imageNameInfo[temp2].second + 1);

                    //                    cout << "key : " << temp2 << "\tcount : " << imageNameInfo[temp2].second << " path : " << imageNameInfo[temp2].first << endl;
                } else {
                    imageNameInfo[temp2] = pair<string, int>(v[i], 1);
                }
            }
        }
    }
}

void search5(vector<string>& v, string& endStr, string& word, vector<string>& outV)
{

    for (unsigned long i = 0; i < v.size(); i++) {

        //////////////
        ///  text  ///
        //////////////
        int index = v[i].find(word);
        if (index != -1) {
            string temp = v[i].substr(index, v[i].length() - index + word.length());
            int endPos = temp.find(endStr);
            outV.push_back(temp.substr(0, endPos));
        }
    }
}

void search8(vector<string>& v, string& endStr, string& word, map<string, string>& out)
{

    for (unsigned long i = 0; i < v.size(); i++) {
        int index = v[i].find(word);
        if (index != -1) {

            string valueStr = v[i].substr(index, v[i].find(endStr));
            //            cout << "rear rm text : " << valueStr << endl;
            string keyStr = valueStr.substr(word.length(), valueStr.length() - index);
            //            cout << "keyStr : " << keyStr << endl;

            out[keyStr] = ENUM_STR_FREFIX + valueStr;
        }
    }
}
int main(int argc, char* argv[])
{
    ///////////
    //  arg  //
    ///////////
    /// \brief 입력 받은 진자 읽기
    /// \return
    ///
    QCoreApplication app(argc, argv);
    QString outputFilePath("DisplayIDStructTable.cpp ");
    QString codeFilePath("image_rsc_io.cpp");
    QCommandLineParser parser;

    parser.setApplicationDescription("ResourceGen");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("qrc", "The file to open.");

    QCommandLineOption genOutOption2(QStringList() << "c"
                                                   << "compare ",
        QObject::tr("Compare two files and remove duplicates <output_path>"),
        QObject::tr("output_path"));
    parser.addOption(genOutOption2);

    parser.process(app);

    if (parser.isSet(genOutOption2)) {
        outputFilePath = parser.value(genOutOption2);
    }

    QString dispHeader = "";
    QString dispFidl = "";
    QString qrcFile_new = "";
    QString qrcFile_old = "";

    foreach (const QString& fileName, parser.positionalArguments()) {

        if (0 == fileName.compare("Image_old.qrc")) {
            qrcFile_old = fileName;
        } else if (0 == fileName.compare("Image_new.qrc")) {
            qrcFile_new = fileName;
        }
    }

    qDebug() << "dispHeader : " << dispHeader;
    qDebug() << "dispFidl : " << dispFidl;

    //////////////////////
    /// qrc check  new ///
    //////////////////////
    cout << "==========READ  " << qrcFile_new.toStdString() << "==========\n\n\n\n";
    vector<string> readDataDisp;
    ifstream fin(qrcFile_new.toStdString());
    fileRead(fin, readDataDisp);
    string word = "</file>";
    map<string, pair<string, int>> mapHeaderID;
    qrcImageNameListUp(readDataDisp, word, mapHeaderID);

    map<string, string> fileNameMap;

    map<string, pair<string, int>>::iterator it;
    for (it = mapHeaderID.begin(); it != mapHeaderID.end(); ++it) {
        //        if (it->second.second == 1) {
        fileNameMap[it->first] = it->second.first;
        //        }
    }

    map<string, string>::iterator it2;
    for (it2 = fileNameMap.begin(); it2 != fileNameMap.end(); ++it2) {
        cout << it2->first << endl;
    }

    QString path = QDir::currentPath();
    QString newfile = "new";
    QString oldfile = "old";
    int rsc = -1;
    FILE* fpSrc;
    rsc = open(newfile.toLocal8Bit(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    fpSrc = fopen(newfile.toLocal8Bit(), "w+");

    for (it2 = fileNameMap.begin(); it2 != fileNameMap.end(); ++it2) {
        fprintf(fpSrc, "{%s}\n", it2->first.c_str());
        cout << it2->first << endl;
    }

    close(rsc);
    fclose(fpSrc);

    //////////////////////////////////////////////////

    //////////////////////
    /// qrc check  old ///
    //////////////////////
    cout << "==========READ  " << qrcFile_old.toStdString() << "==========\n\n\n\n";
    vector<string> readDataDisp2;
    ifstream fin2(qrcFile_old.toStdString());
    fileRead(fin2, readDataDisp2);
    string word2 = "</file>";
    map<string, pair<string, int>> mapHeaderID2;
    qrcImageNameListUp(readDataDisp2, word2, mapHeaderID2);

    map<string, string> fileNameMap2;

    //    map<string, pair<string, int>>::iterator it;
    for (it = mapHeaderID2.begin(); it != mapHeaderID2.end(); ++it) {
        //        if (it->second.second == 1) {
        fileNameMap2[it->first] = it->second.first;
        //        }
    }

    //    map<string, string>::iterator it2;
    for (it2 = fileNameMap2.begin(); it2 != fileNameMap2.end(); ++it2) {
        cout << it2->first << endl;
    }

    rsc = open(oldfile.toLocal8Bit(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    fpSrc = fopen(oldfile.toLocal8Bit(), "w+");

    for (it2 = fileNameMap2.begin(); it2 != fileNameMap2.end(); ++it2) {
        fprintf(fpSrc, "{%s}\n", it2->first.c_str());
        cout << it2->first << endl;
    }

    close(rsc);
    fclose(fpSrc);
}

void search12345(vector<string>& v, vector<string>& outV)
{
    string aliasStr;
    string slitWord = "/Image/";
    string endWord = "</file>";

    for (unsigned long i = 0; i < v.size(); i++) {
        int index = v[i].find(slitWord);

        if (-1 != index) {
            string tempStr = v[i].substr(index + slitWord.length(), v[i].length() - (index + +slitWord.length()) - endWord.length());
            string tempStrTail = v[i].substr(index + slitWord.length(), v[i].length() - (index + +slitWord.length()));
            //            cout << tempStr << endl;

            //            <file alias="Combination/TBT_IMG_ARROW_1_L.png">Image_QRC/14.1_Navi_TBT/Image/Combination/TBT_IMG_ARROW_1_L.png</file>
            string tempStr2 = "        <file alias=\"" + tempStr + "\">" + tempStrTail;
            //            cout << tempStr2 << endl;
            outV.push_back(tempStr2);

        } else {
            outV.push_back(v[i]);
        }
    }
}

//int main(int argc, char* argv[])
//{
//    ///////////
//    //  arg  //
//    ///////////
//    /// \brief 입력 받은 진자 읽기
//    /// \return
//    ///
//    QCoreApplication app(argc, argv);
//    QString outputFilePath("DisplayIDStructTable.cpp ");
//    QString codeFilePath("image_rsc_io.cpp");
//    QCommandLineParser parser;

//    parser.setApplicationDescription("ResourceGen");
//    parser.addHelpOption();
//    parser.addVersionOption();
//    parser.addPositionalArgument("qrc", "The file to open.");

//    QCommandLineOption genOutOption2(QStringList() << "c"
//                                                   << "compare ",
//        QObject::tr("Compare two files and remove duplicates <output_path>"),
//        QObject::tr("output_path"));
//    parser.addOption(genOutOption2);

//    parser.process(app);

//    if (parser.isSet(genOutOption2)) {
//        outputFilePath = parser.value(genOutOption2);
//    }

//    QString dispHeader = "";
//    QString dispFidl = "";
//    QString qrcFile_new = "";
//    QString qrcFile_old = "";

//    foreach (const QString& fileName, parser.positionalArguments()) {

//        if (0 == fileName.compare("Image.qrc")) {
//            qrcFile_new = fileName;
//        } else if (0 == fileName.compare("Image_new.qrc")) {
//            qrcFile_new = fileName;
//        }
//    }

//    qDebug() << "dispHeader : " << dispHeader;
//    qDebug() << "dispFidl : " << dispFidl;

//    //////////////////////
//    /// qrc check  new ///
//    //////////////////////
//    cout << "==========READ  " << qrcFile_new.toStdString() << "==========\n\n\n\n";
//    vector<string> readDataDisp;
//    ifstream fin(qrcFile_new.toStdString());
//    fileRead(fin, readDataDisp);

//    vector<string> outputData;
//    search12345(readDataDisp, outputData);

//    ///////////////////
//    /// create file ///
//    ///////////////////
//    QString oldfile = "change.qrc";
//    int rsc = -1;
//    FILE* fpSrc;

//    rsc = open(oldfile.toLocal8Bit(), O_CREAT | O_RDWR | O_TRUNC, 0644);
//    fpSrc = fopen(oldfile.toLocal8Bit(), "w+");

//    for (int i = 0; i < outputData.size(); i++) {
//        fprintf(fpSrc, "%s\n", outputData[i].c_str());
//    }

//    close(rsc);
//    fclose(fpSrc);
//}

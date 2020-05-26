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
// ./DisplayGen *.qrc

void fileRead(ifstream& fin, vector<string>& v)
{
    string line;

    while (getline(fin, line))
        v.push_back(line);
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

        if (0 == fileName.compare("Image.qrc")) {
            qrcFile_new = fileName;
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

    vector<string> outputData;
    search12345(readDataDisp, outputData);

    ///////////////////
    /// create file ///
    ///////////////////
    QString oldfile = "change.qrc";
    int rsc = -1;
    FILE* fpSrc;

    rsc = open(oldfile.toLocal8Bit(), O_CREAT | O_RDWR | O_TRUNC, 0644);
    fpSrc = fopen(oldfile.toLocal8Bit(), "w+");

    for (int i = 0; i < outputData.size(); i++) {
        fprintf(fpSrc, "%s\n", outputData[i].c_str());
    }

    close(rsc);
    fclose(fpSrc);
}

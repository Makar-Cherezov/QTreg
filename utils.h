#ifndef utils_h
#define utils_h

#include <vector>
#include <filesystem>
#include "ui_mainwindow.h"
#include <QDir>
#include "itkImage.h"
#include <fstream>
std::vector<std::string> GetFilesInDir(std::filesystem::path directoryPath); // Старый способ чтения имён файлов
void fillFileNames(std::vector<std::string>* srcFileNames,
                   std::vector<std::string>* refFileNames,
                   Ui::MainWindow* ui); // Новый способ на встроенных стредствах QT
void PrintSlice(itk::Image<float, 3>::Pointer fixedImage); // печать пикселей слайца в файл

#endif

